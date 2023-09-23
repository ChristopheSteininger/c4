#include "search.h"

#include <algorithm>
#include <cassert>

#include "Tracy.hpp"
#include "position.h"
#include "settings.h"
#include "table.h"

static constexpr int INF_SCORE = 10000;

static int get_node_type(int value, int alpha, int beta) {
    if (value <= alpha) {
        return TYPE_UPPER;
    }

    if (value >= beta) {
        return TYPE_LOWER;
    }

    return TYPE_EXACT;
}

static int count_bits(board b) {
    int result;
    for (result = 0; b; result++) {
        b &= b - 1;
    }

    return result;
}

static void rotate_moves(int *moves, int num_moves, int offset, bool has_table_move) {
    int *first_move = moves;
    int num_rotate_moves = num_moves;

    // Don't rotate table moves.
    if (has_table_move) {
        first_move++;
        num_rotate_moves--;
    }

    if (num_rotate_moves > 1) {
        std::rotate(first_move, first_move + (offset % num_rotate_moves), first_move + num_rotate_moves);
    }
}

static void sort_moves(Node *children, int num_moves, int *moves, int offset, int table_move) {
    assert(num_moves > 0);

    // A move from the table always goes first.
    if (table_move != -1) {
        children[table_move].dynamic_score = 1000;
    }

    // Sort moves according to score, high to low.
    std::sort(moves, moves + num_moves,
              [&children](int a, int b) { return children[a].dynamic_score > children[b].dynamic_score; });

    // Rotate any non table moves to help threads desync.
    if (offset != 0) {
        rotate_moves(moves, num_moves, offset, table_move != -1);
    }
}

static float heuristic(Position &pos, board threats, int col) {
    int num_threats = count_bits(threats);
    int num_next_threats = count_bits(pos.find_next_turn_threats(threats));
    int num_next_next_threats = count_bits(pos.find_next_next_turn_threats(threats));
    float center_score = (float)std::min(col, BOARD_WIDTH - col - 1) / BOARD_WIDTH;

    return 1.0 * num_next_threats + 0.5 * num_next_next_threats + 0.3 * num_threats + 0.1 * center_score;
}

inline static void invert(int &alpha, int &beta) {
    alpha = -alpha;
    beta = -beta;
}

// Create our own copy of the transposition table. This table will use the same
// underlying storage as parent_table so this thread can benefit from the work
// other threads have saved in the table.
Search::Search(const Table &parent_table, const std::shared_ptr<Stats> stats)
    : table(parent_table, stats), stats(stats) {}

void Search::start() { stop_search = false; }

void Search::stop() { stop_search = true; }

int Search::search(Position &pos, int alpha, int beta, int move_offset) {
    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());
    assert(!pos.wins_this_move(pos.find_player_threats()));

    Node child(pos);
    if (static_search(child, -1, alpha, beta) || alpha >= beta) {
        return alpha;
    }

    if (pos.is_same_player(child.pos)) {
        return negamax(child, alpha, beta, move_offset);
    } else {
        return -negamax(child, -beta, -alpha, move_offset);
    }
}

int Search::negamax(Node &node, int alpha, int beta, int move_offset) {
    ZoneScoped;

    assert(alpha < beta);
    assert(!node.pos.has_player_won());
    assert(!node.pos.has_opponent_won());
    assert(!node.pos.is_draw());
    assert(!node.pos.wins_this_move(node.pos.find_player_threats()));

    stats->new_node();

    // If another thread found the result we are looking for,
    // immediately return.
    if (stop_search) {
        return SEARCH_STOPPED;
    }

    int original_alpha = alpha;
    int original_beta = beta;

    // Prefetch the position's entry.
    if (!node.did_lookup) {
        node.hash = node.pos.hash(node.is_mirrored);
    }
    table.prefetch(node.hash);

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!node.pos.can_player_win()) {
        beta = std::min(beta, 0);
    }

    // This function will never be called on a position that can be statically evaluated, so we
    // know it is not possible to win or lose in the next two turns, so tighten bounds.
    alpha = std::max(alpha, node.pos.score_loss(2));
    beta = std::min(beta, node.pos.score_win(2));
    if (alpha >= beta) {
        return beta;
    }

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = node.pos.find_opponent_threats();
    board non_losing_moves = node.pos.find_non_losing_moves(opponent_threats);

    int value = -INF_SCORE;

    int num_moves = 0;
    int moves[BOARD_WIDTH];
    Node children[BOARD_WIDTH];

    // Next, we will test each move if it can be statically evaluated (i.e. only
    // playing forced moves will lead to a forced win, loss, or draw). Moves that
    // are statically evaluated will not be recursed into, and can be used to
    // tighten search bounds.
    //
    // Moves which cannot be statically evaluated will instead be assigned a score
    // which is a guess of how good the move is. Moves with the highest score will
    // be searched first.
    int b = -INF_SCORE;
    for (int col = 0; col < BOARD_WIDTH; col++) {
        if (node.pos.is_non_losing_move(non_losing_moves, col)) {
            children[col] = Node(node.pos);
            int child_alpha = alpha;
            int child_beta = beta;

            invert(child_alpha, child_beta);
            children[col].pos.move(col);
            bool is_static = static_search(children[col], col, child_beta, child_alpha);
            invert(child_alpha, child_beta);

            alpha = std::max(alpha, child_alpha);
            value = std::max(value, child_alpha);
            b = std::max(b, child_beta);

            if (alpha >= beta) {
                return alpha;
            }

            if (!is_static) {
                moves[num_moves] = col;
                num_moves++;
            }
        }
    }

    // At this point we know it is not possible to win in the next four turns, so tighten
    // bounds further.
    beta = std::min(beta, node.pos.score_win(4));
    beta = std::min(beta, b);
    if (alpha >= beta) {
        return beta;
    }

    // If every move was statically evaluated, then there is nothing more to do.
    if (num_moves == 0) {
        return value;
    }

    // Check if this state has already been seen.
    if (!node.did_lookup) {
        int lookup_type, lookup_value;
        table.get(node.hash, node.is_mirrored, node.table_move, lookup_type, lookup_value);
        if (lookup_type == TYPE_EXACT) {
            return lookup_value;
        } else if (lookup_type == TYPE_LOWER) {
            alpha = std::max(alpha, lookup_value);
        } else if (lookup_type == TYPE_UPPER) {
            beta = std::min(beta, lookup_value);
        }
        if (alpha >= beta) {
            return lookup_value;
        }
    }

    // Sort moves according to score.
    sort_moves(children, num_moves, moves, move_offset, node.table_move);

    // If none of the above checks pass, then this is an internal node and we must
    // evaluate the child nodes to determine the score of this node.
    int best_recursion_value = -INF_SCORE, best_move_index = -1, best_move_col = -1;
    for (int i = 0; i < num_moves && alpha < beta; i++) {
        int col = moves[i];

        int child_move_offset = move_offset / BOARD_WIDTH;
        if (node.table_move != -1 && i == 0) {
            // Table moves do not respect the move offset, so pass it onto the child.
            child_move_offset = move_offset;
        }

        int child_score = node.pos.is_same_player(children[col].pos)
                              ? negamax(children[col], alpha, beta, child_move_offset)
                              : -negamax(children[col], -beta, -alpha, child_move_offset);

        // If the child aborted the search, propagate the signal upwards.
        if (abs(child_score) == -SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        if (child_score > best_recursion_value) {
            best_move_index = i;
            best_move_col = col;
        }

        value = std::max(value, child_score);
        alpha = std::max(alpha, child_score);
        best_recursion_value = std::max(best_recursion_value, child_score);
    }

    assert(best_recursion_value != -INF_SCORE);
    assert(best_move_col != -1);
    assert(best_move_index != -1);
    assert(alpha >= value);
    assert(value > -INF_SCORE);

    // Store the result in the transposition table.
    int type = get_node_type(value, original_alpha, original_beta);
    table.put(node.hash, node.is_mirrored, best_move_col, type, value);

    // Update statistics.
    stats->node_type(type);
    if (best_move_index == 0) {
        stats->best_move_guessed();
    } else if (best_move_index == num_moves - 1) {
        // Oops.
        stats->worst_move_guessed();
    }

    return value;
}

bool Search::static_search(Node &node, int col, int &alpha, int &beta) {
    ZoneScoped;

    assert(alpha < beta);
    assert(!node.pos.has_player_won());
    assert(!node.pos.has_opponent_won());
    assert(!node.pos.is_draw());
    assert(!node.pos.wins_this_move(node.pos.find_player_threats()));

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!node.pos.can_player_win()) {
        beta = std::min(beta, 0);
        if (alpha >= beta) {
            return beta;
        }
    }

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = node.pos.find_opponent_threats();
    board non_losing_moves = node.pos.find_non_losing_moves(opponent_threats);

    // If the player can only move below the opponents threats, the player will lose.
    if (non_losing_moves == 0) {
        alpha = node.pos.score_loss();
        beta = alpha;
        return true;
    }

    // Check if the opponent could win next move.
    board opponent_wins = node.pos.wins_this_move(opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            alpha = node.pos.score_loss();
            beta = alpha;
            return true;
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            alpha = node.pos.score_loss();
            beta = alpha;
            return true;
        }
    }

    // At this point we know it is not possible to win or lose in the next two turns, so tighten bounds.
    alpha = std::max(alpha, node.pos.score_loss(2));
    beta = std::min(beta, node.pos.score_win(2));
    if (alpha >= beta) {
        return true;
    }

    board useful_threats;
    if (col != -1) {
        board player_threats = node.pos.find_player_threats();
        useful_threats = node.pos.find_useful_threats(opponent_threats, player_threats);
    }

    // Check if we have a forced move and if so, statically evaluate it.
    board forced_move = get_forced_move(node.pos, opponent_wins, non_losing_moves);
    if (forced_move) {
        invert(alpha, beta);
        node.pos.move(forced_move);
        bool is_static = static_search(node, -1, beta, alpha);
        invert(alpha, beta);

        if (is_static) {
            return true;
        }
    }

    // If we do not have a forced move then this position cannot be statically evaluated.
    // Do a table lookup to see if we can tighten search bounds.
    else if (node.pos.get_ply() < ENHANCED_TABLE_CUTOFF_PLIES) {
        node.did_lookup = true;
        node.hash = node.pos.hash(node.is_mirrored);

        // Check if this state has already been seen.
        int table_move, lookup_type, lookup_value;
        bool lookup_success = table.get(node.hash, node.is_mirrored, table_move, lookup_type, lookup_value);
        if (lookup_success) {
            if (lookup_type == TYPE_EXACT) {
                alpha = lookup_value;
                beta = lookup_value;
                return true;
            } else if (lookup_type == TYPE_LOWER) {
                alpha = std::max(alpha, lookup_value);
            } else if (lookup_type == TYPE_UPPER) {
                beta = std::min(beta, lookup_value);
            }
            if (alpha >= beta) {
                return true;
            }

            node.table_move = table_move;
        }
    }

    // At this point we know the move is complex and cannot be statically evaluated
    // so use a heuristic to guess the value of the move.
    if (col != -1) {
        node.dynamic_score = heuristic(node.pos, useful_threats, col);
    }

    return false;
}

board Search::get_forced_move(Position &pos, board opponent_wins, board non_losing_moves) {
    // A move is forced if the opponent could win next turn.
    if (opponent_wins) {
        assert((opponent_wins & (opponent_wins - 1)) == 0);
        assert((opponent_wins & non_losing_moves) == opponent_wins);
        return opponent_wins;
    }

    // A move is also forced if the player has only one move which does not lose immediately.
    if ((non_losing_moves & (non_losing_moves - 1)) == 0) {
        return non_losing_moves;
    }

    return 0;
}
