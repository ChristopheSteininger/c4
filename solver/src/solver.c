#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "solver.h"
#include "settings.h"
#include "board.h"
#include "table.h"
#include "order.h"


unsigned long stat_num_nodes;
unsigned long stat_num_child_nodes;
unsigned long stat_num_exact_nodes;
unsigned long stat_num_lower_nodes;
unsigned long stat_num_upper_nodes;
unsigned long stat_num_moves_checked;
unsigned long stat_num_interior_nodes;
unsigned long stat_num_best_moves_guessed;


static int max(int a, int b) {
    if (a > b) {
        return a;
    }

    return b;
}


static int min(int a, int b) {
    if (a < b) {
        return a;
    }

    return b;
}


static int get_node_type(int value, int alpha, int beta) {
    if (value <= alpha) {
        return TYPE_UPPER;
    }

    if (value >= beta) {
        return TYPE_LOWER;
    }

    return TYPE_EXACT;
}


static int negamax(const board player, const board opponent, int alpha, int beta) {
    assert(alpha < beta);
    
    stat_num_nodes++;

    int original_alpha = alpha;

    // If there are too few empty spaces left on the board for either player to win, then
    // adjust the bounds of the search. If neither player can win then the game is guaranteed
    // to end in a draw.
    board empty_positions = VALID_CELLS & ~(player | opponent);
    if (!has_won(opponent | empty_positions)) {
        alpha = max(alpha, 0);
    }
    if (!has_won(player | empty_positions)) {
        beta = min(beta, 0);
    }
    if (alpha >= beta) {
        return 0;
    }

    // If the player can only move below the opponents threats, the player will lose.
    board opponent_threats = find_threats(opponent, player);
    board non_losing_moves = find_non_losing_moves(player, opponent, opponent_threats);
    if (non_losing_moves == 0) {
        return -1;
    }

    // Check if the opponent could win next move.
    board opponent_wins = wins_this_move(opponent, player, opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            return -1;
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            return -1;
        }

        // Otherwise, the opponent has only one threat, and the player must block the threat.
        else {
            return -negamax(opponent, player | opponent_wins, -beta, -alpha);
        }
    }

    // Check if this state has already been seen.
    int lookup_move, lookup_type, lookup_value;
    int lookup_success = table_lookup(player, opponent, &lookup_move, &lookup_type, &lookup_value);

    if (lookup_success) {
        if (lookup_type == TYPE_EXACT) {
            return lookup_value;
        }
        
        else if (lookup_type == TYPE_LOWER) {
            alpha = max(alpha, lookup_value);
        }
        
        else if (lookup_type == TYPE_UPPER) {
            beta = min(beta, lookup_value);
        }

        if (alpha >= beta) {
            return lookup_value;
        }
    }

    // If none of the above checks pass, then this is an internal node and we must
    // evaluate the child nodes to determine the score of this node.
    int value = -1000;
    int best_move_index, best_move_col;

    int moves[BOARD_WIDTH];
    int num_moves = order_moves(player, opponent, moves, non_losing_moves, lookup_success ? lookup_move : BOARD_WIDTH);

    int i;
    for (i = 0; i < num_moves && alpha < beta; i++) {
        int col = moves[i];
        board after_move = move(player, opponent, col);

        int child_score = -negamax(opponent, after_move, -beta, -alpha);

        if (child_score > value) {
            value = child_score;
            best_move_index = i;
            best_move_col = col;
        }

        alpha = max(child_score, alpha);
    }

    assert(value != -1000);

    // Store the result in the transposition table.
    int type = get_node_type(value, original_alpha, beta);
    table_store(player, opponent, best_move_col, type, value);

    // Best move look ups from the table should always be correct.
    if (type == TYPE_UPPER && lookup_success) {
        assert(best_move_index == 0);
    }

    // Update statistics.
    stat_num_interior_nodes++;
    stat_num_child_nodes += num_moves;
    stat_num_moves_checked += i;

    if (best_move_index == 0) {
        stat_num_best_moves_guessed++;
    }

    if (type == TYPE_EXACT) {
        stat_num_exact_nodes++;
    } else if (type == TYPE_LOWER) {
        stat_num_lower_nodes++;
    } else if (type == TYPE_UPPER) {
        stat_num_upper_nodes++;
    }

    return value;
}


int get_best_move(board player, board opponent) {
    int score = solve(player, opponent);

    for (int col = 0; col < BOARD_WIDTH; col++) {
        if (is_move_valid(player, opponent, col)) {
            board after_move = move(player, opponent, col);
            int child_score = -solve(opponent, after_move);

            if (child_score >= score) {
                return col;
            }
        }
    }
    
    assert(0);

    return 0;
}


int solve(board b0, board b1) {
    stat_num_nodes = 0;
    stat_num_child_nodes = 0;
    stat_num_exact_nodes = 0;
    stat_num_lower_nodes = 0;
    stat_num_upper_nodes = 0;
    stat_num_moves_checked = 0;
    stat_num_interior_nodes = 0;
    stat_num_best_moves_guessed = 0;

    // There is no point in check simple conditions like win in one move
    // during search so handle these here.
    if (has_won(b0)) {
        return 1;
    }
    if (has_won(b1)) {
        return -1;
    }
    if (wins_this_move(b0, b1, find_threats(b0, b1))) {
        return 1;
    }
    if (is_draw(b0, b1)) {
        return 0;
    }

    int result = negamax(b0, b1, -1, 0);

    if (result == 0) {
        return negamax(b0, b1, 0, 1);
    }

    return result;
}


int solve_verbose(board b0, board b1) {
    printf("Solving:\n");
    printb(b0, b1);

    unsigned long start_time = clock();
    int score = solve(b0, b1);
    double run_time_sec = (clock() - start_time) / (double) CLOCKS_PER_SEC;

    printf("\n");
    printf("Score is %d\n", score);

    printf("\n");
    printf("Time to solve        = %'.0f s\n", run_time_sec);
    printf("Nodes per ms         = %'.0f\n", get_num_nodes() / run_time_sec / 1000);
    printf("Nodes:\n");
    printf("    Exact            = %'lu\n", get_num_exact_nodes());
    printf("    Lower            = %'lu\n", get_num_lower_nodes());
    printf("    Upper            = %'lu\n", get_num_upper_nodes());
    printf("    Total            = %'lu\n", get_num_nodes());
    printf("Table:\n");
    printf("    Hit rate         = %6.2f%%\n", get_table_hit_rate() * 100);
    printf("    Collision rate   = %6.2f%%\n", get_table_collision_rate() * 100);
    printf("    Density          = %6.2f%%\n", get_table_density() * 100);
    printf("    Rewrite rate     = %6.2f%%\n", get_table_rewrite_rate() * 100);
    printf("    Overwrite rate   = %6.2f%%\n", get_table_overwrite_rate() * 100);
    printf("Best moves guessed   = %6.2f%%\n", (double) get_num_best_moves_guessed() * 100 / get_num_interior_nodes());
    printf("Moves checked        = %6.2f%%\n", get_moves_checked_rate() * 100);

    return score;
}


unsigned long get_num_nodes() {
    return stat_num_nodes;
}


unsigned long get_num_exact_nodes() {
    return stat_num_exact_nodes;
}


unsigned long get_num_lower_nodes() {
    return stat_num_lower_nodes;
}


unsigned long get_num_upper_nodes() {
    return stat_num_upper_nodes;
}


unsigned long get_num_best_moves_guessed() {
    return stat_num_best_moves_guessed;
}


unsigned long get_num_interior_nodes() {
    return stat_num_interior_nodes;
}


double get_moves_checked_rate() {
    return (double) stat_num_moves_checked / stat_num_child_nodes;
}
