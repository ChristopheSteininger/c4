#include <cassert>
#include <algorithm>

#include "Tracy.hpp"

#include "search.h"
#include "settings.h"
#include "position.h"
#include "table.h"
#include "order.h"


static const int MIN_SCORE = -BOARD_WIDTH * BOARD_HEIGHT / 2;
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


int Search::negamax(Position &pos, int alpha, int beta, int move_offset) {
    ZoneScoped;

    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());
    assert(!pos.wins_this_move(pos.find_player_threats()));

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

    // Find the opponents threats, and any moves directly below a threat.
    // These moves will not be played.
    board opponent_threats = pos.find_opponent_threats();
    board non_losing_moves = pos.find_non_losing_moves(opponent_threats);

    // If the player can only move below the opponents threats, the player will lose.
    if (non_losing_moves == 0) {
        return pos.score_loss();
    }

    // Check if the opponent could win next move.
    board opponent_wins = pos.wins_this_move(opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            return pos.score_loss();
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            return pos.score_loss();
        }
    }

    // At this point we know it is not possible to win or lose next turn,
    // so tighten bounds.
    alpha = std::max(alpha, pos.score_loss(2));
    if (alpha >= beta) {
        return alpha;
    }
    beta = std::min(beta, pos.score_win(2));
    if (alpha >= beta) {
        return beta;
    }

    // If the opponent has only one threat, then the player must block the threat.
    if (opponent_wins) {
        board before_move = pos.move(opponent_wins);
        int score = -negamax(pos, -beta, -alpha, move_offset);
        pos.unmove(before_move);

        // If the child aborted the search, propagate the signal upwards.
        if (score == -SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        return score;
    }

    // Check if this state has already been seen.
    int table_move = -1, lookup_type, lookup_value;

    bool lookup_success = table.get(hash, is_mirrored, table_move, lookup_type, lookup_value);
    lookup_value += MIN_SCORE;

    if (lookup_success) {
        if (lookup_type == TYPE_EXACT) {
            return lookup_value;
        }

        else if (lookup_type == TYPE_LOWER) {
            alpha = std::max(alpha, lookup_value);
        }

        else if (lookup_type == TYPE_UPPER) {
            beta = std::min(beta, lookup_value);
        }

        if (alpha >= beta) {
            return lookup_value;
        }
    }

    // If none of the above checks pass, then this is an internal node and we must
    // evaluate the child nodes to determine the score of this node.
    int value = -INF_SCORE;
    int best_move_index, best_move_col;

    int moves[BOARD_WIDTH];
    int num_moves = order_moves(pos, moves, non_losing_moves, table_move, move_offset);

    int i;
    for (i = 0; i < num_moves && alpha < beta; i++) {
        int col = moves[i];

        // Table moves do not respect the move offset, so pass it onto the child.
        int child_move_offset = 0;
        if (table_move != -1 && i == 0) {
            child_move_offset = move_offset;
        }

        board before_move = pos.move(col);
        int child_score = -negamax(pos, -beta, -alpha, child_move_offset);
        pos.unmove(before_move);

        // If the child aborted the search, propagate the signal upwards.
        if (child_score == -SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        if (child_score > value) {
            value = child_score;
            best_move_index = i;
            best_move_col = col;
        }

        alpha = std::max(child_score, alpha);
    }

    assert(value != -INF_SCORE);

    // Store the result in the transposition table.
    int type = get_node_type(value, original_alpha, original_beta);
    table.put(hash, is_mirrored, best_move_col, type, value + -MIN_SCORE);

    // Update statistics.
    stats->node_type(type);
    if (best_move_index == 0) {
        stats->best_move_guessed();
    }

    return value;
}
