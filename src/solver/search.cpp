#include <cassert>
#include <algorithm>

#include "Tracy.hpp"

#include "search.h"
#include "settings.h"
#include "position.h"
#include "table.h"
#include "order.h"


static const int INF_SCORE = 10000;

const int SEARCH_STOPPED = -1000;


static int get_node_type(int value, int alpha, int beta) {
    if (value <= alpha) {
        return TYPE_UPPER;
    }

    if (value >= beta) {
        return TYPE_LOWER;
    }

    return TYPE_EXACT;
}


// Create our own copy of the transposition table. This table will use the same
// underlying storage as parent_table so this thread can benefit from the work
// other threads have saved in the table.
Search::Search(const Table &parent_table, const std::shared_ptr<Stats> stats)
    : table(parent_table, stats), stats(stats) {
}

void Search::start() {
    stop_search = false;
}


void Search::stop() {
    stop_search = true;
}


int Search::search(Position &pos, int alpha, int beta, int move_offset) {
    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());
    assert(!pos.wins_this_move(pos.find_player_threats()));

    int static_score;
    if (static_evaluation(pos, static_score)) {
        return static_score;
    }

    return negamax(pos, alpha, beta, move_offset);
}


int Search::negamax(Position &pos, int alpha, int beta, int move_offset) {
    ZoneScoped;

#ifndef NDEBUG
    int dummy;
#endif

    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());
    assert(!pos.wins_this_move(pos.find_player_threats()));
    assert(!static_evaluation(pos, dummy));

    stats->new_node();

    // If another thread found the result we are looking for,
    // immediately return.
    if (stop_search) {
        return SEARCH_STOPPED;
    }

    int original_alpha = alpha;
    int original_beta = beta;

    // Prefetch the position's entry.
    bool is_mirrored;
    board hash = pos.hash(is_mirrored);
    table.prefetch(hash);

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!pos.can_player_win()) {
        beta = std::min(beta, 0);
    }
    if (alpha >= beta) {
        return 0;
    }

    // At this point we know it is not possible to win or lose in the next two turns,
    // so tighten bounds.
    alpha = std::max(alpha, pos.score_loss(2));
    if (alpha >= beta) {
        return alpha;
    }
    beta = std::min(beta, pos.score_win(2));
    if (alpha >= beta) {
        return beta;
    }

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = pos.find_opponent_threats();
    board opponent_wins = pos.wins_this_move(opponent_threats);
    board non_losing_moves = pos.find_non_losing_moves(opponent_threats);

    int value = -INF_SCORE;

    // Return the result of the forced move if we have one.
    board forced_move = get_forced_move(pos, opponent_wins, non_losing_moves);
    if (forced_move) {
        board before_move = pos.move(forced_move);
        value = -negamax(pos, -beta, -alpha, move_offset);
        pos.unmove(before_move);

        // If the child aborted the search, propagate the signal upwards.
        if (value == -SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        return value;
    }

    // Check each move if it can be statically evaluated by only playing forced
    // moves. These results can be used to tighten search bounds and reduce the
    // number of moves to be searched.
    board recursion_moves = non_losing_moves;
    for (int col = 0; col < BOARD_WIDTH; col++) {
        if (pos.is_non_losing_move(non_losing_moves, col)) {
            int static_score;

            board before_move = pos.move(col);
            bool is_static = static_evaluation(pos, static_score);
            pos.unmove(before_move);

            if (is_static) {
                recursion_moves = pos.clear_move(recursion_moves, col);

                value = std::max(value, -static_score);
                alpha = std::max(alpha, -static_score);

                if (alpha >= beta) {
                    return alpha;
                }
            }
        }
    }

    // At this point we know it is not possible to win in the next four turns, so tighten
    // bounds further.
    beta = std::min(beta, pos.score_win(4));
    if (alpha >= beta) {
        return beta;
    }

    // If every move was statically evaluated, then there is nothing more to do.
    if (recursion_moves == 0) {
        return value;
    }

    // If only a single move was not statically evaluated, then play that move and return.
    if ((recursion_moves & (recursion_moves - 1)) == 0) {
        board before_move = pos.move(recursion_moves);
        int score = -negamax(pos, -beta, -alpha, move_offset);
        pos.unmove(before_move);

        // If the child aborted the search, propagate the signal upwards.
        if (score == -SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        return std::max(score, value);
    }

    // Check if this state has already been seen.
    int table_move, lookup_type, lookup_value;
    table.get(hash, is_mirrored, table_move, lookup_type, lookup_value);
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

    // If none of the above checks pass, then this is an internal node and we must
    // evaluate the child nodes to determine the score of this node.
    int best_recursion_value = -INF_SCORE, best_move_index = -1, best_move_col = -1;

    int moves[BOARD_WIDTH];
    int num_moves = order_moves(pos, moves, recursion_moves, table_move, move_offset % BOARD_WIDTH);

    for (int i = 0; i < num_moves && alpha < beta; i++) {
        int col = moves[i];

        int child_move_offset = move_offset / BOARD_WIDTH;
        if (table_move != -1 && i == 0) {
            // Table moves do not respect the move offset, so pass it onto the child.
            child_move_offset = move_offset;
        }

        board before_move = pos.move(col);
        int child_score = -negamax(pos, -beta, -alpha, child_move_offset);
        pos.unmove(before_move);

        // If the child aborted the search, propagate the signal upwards.
        if (child_score == -SEARCH_STOPPED) {
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

    // Store the result in the transposition table.
    int type = get_node_type(value, original_alpha, original_beta);
    table.put(hash, is_mirrored, best_move_col, type, value);

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


bool Search::static_evaluation(Position &pos, int &static_score) {
    if (pos.is_draw()) {
        return 0;
    }

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = pos.find_opponent_threats();
    board non_losing_moves = pos.find_non_losing_moves(opponent_threats);

    // If the player can only move below the opponents threats, the player will lose.
    if (non_losing_moves == 0) {
        static_score = pos.score_loss();
        return true;
    }

    // Check if the opponent could win next move.
    board opponent_wins = pos.wins_this_move(opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            static_score = pos.score_loss();
            return true;
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            static_score = pos.score_loss();
            return true;
        }
    }

    // Check if we have a forced move and, if so, statically evaluate it.
    board forced_move = get_forced_move(pos, opponent_wins, non_losing_moves);
    if (forced_move) {
        board before_move = pos.move(forced_move);
        bool has_score = static_evaluation(pos, static_score);
        pos.unmove(before_move);

        if (has_score) {
            static_score *= -1;
            return true;
        }
    }

    return false;
}


board Search::get_forced_move(Position &pos, board opponent_wins, board non_losing_moves) {
    // A move is forced if the opponent could win next turn, or if the player has
    // only one move which does not lose immediately.
    if (opponent_wins) {
        // assert(opponent_wins == non_losing_moves);
        return opponent_wins;
    }

    if ((non_losing_moves & (non_losing_moves - 1)) == 0) {
        assert(opponent_wins == 0 || opponent_wins == non_losing_moves);
        return non_losing_moves;
    }

    return 0;
}
