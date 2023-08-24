#include <iostream>
#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "solver.h"
#include "settings.h"
#include "position.h"
#include "table.h"
#include "order.h"


static const int MAX_SCORE = (BOARD_WIDTH * BOARD_HEIGHT + 1) / 2;
static const int MIN_SCORE = -BOARD_WIDTH * BOARD_HEIGHT / 2;
static const int INFINITY = 10000;


static int max(int a, int b) {
    return (a > b) ? a : b;
}


static int min(int a, int b) {
    return (a < b) ? a : b;
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


static int score_loss(Position &pos) {
    return (-BOARD_WIDTH * BOARD_HEIGHT + pos.num_moves()) / 2;
}


static int score_win(Position &pos) {
    return (BOARD_WIDTH * BOARD_HEIGHT + 1 + pos.num_moves()) / 2;
}


int Solver::negamax(Position &pos, int alpha, int beta) {
    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());

    stat_num_nodes++;

    int original_alpha = alpha;
    int original_beta = beta;

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!pos.can_player_win()) {
        beta = min(beta, 0);
    }
    if (alpha >= beta) {
        return 0;
    }

    // The minimum score possible increases each turn.
    alpha = max(alpha, score_loss(pos));
    if (alpha >= beta) {
        return alpha;
    }

    // If the player can only move below the opponents threats, the player will lose.
    board opponent_threats = pos.find_opponent_threats();
    board non_losing_moves = pos.find_non_losing_moves(opponent_threats);
    if (non_losing_moves == 0) {
        return score_loss(pos);
    }

    // Check if the opponent could win next move.
    board opponent_wins = pos.wins_this_move(opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            return score_loss(pos);
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            return score_loss(pos);
        }

        // Otherwise, the opponent has only one threat, and the player must block the threat.
        else {
            board before_move = pos.move(opponent_wins);
            int score = -negamax(pos, -beta, -alpha);
            pos.unmove(before_move);

            return score;
        }
    }

    // Check if this state has already been seen.
    int lookup_move, lookup_type, lookup_value;
    int lookup_success = table.get(pos, lookup_move, lookup_type, lookup_value);
    lookup_value += MIN_SCORE;

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
    int value = -INFINITY;
    int best_move_index, best_move_col;

    int moves[BOARD_WIDTH];
    int num_moves = order_moves(pos, moves, non_losing_moves, lookup_success ? lookup_move : BOARD_WIDTH);

    int i;
    for (i = 0; i < num_moves && alpha < beta; i++) {
        int col = moves[i];
        board before_move = pos.move(col);
        int child_score = -negamax(pos, -beta, -alpha);
        pos.unmove(before_move);

        if (child_score > value) {
            value = child_score;
            best_move_index = i;
            best_move_col = col;
        }

        alpha = max(child_score, alpha);
    }

    assert(value != -INFINITY);

    // Store the result in the transposition table.
    int type = get_node_type(value, original_alpha, original_beta);
    table.put(pos, best_move_col, type, value + -MIN_SCORE);

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

int Solver::solve(Position &pos, int alpha, int beta) {
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
    if (pos.has_player_won()
            || pos.wins_this_move(pos.find_player_threats())) {
        return score_win(pos);
    }
    if (pos.has_opponent_won()) {
        return score_loss(pos);
    }
    if (pos.is_draw()) {
        return 0;
    }

    alpha = max(alpha, score_loss(pos));
    beta = min(beta, score_win(pos));

    int a = 0;
    int b = 1;

    while (a > alpha || b < beta) {
        negamax(pos, a, b);

        a--;
        b++;
    }

    return negamax(pos, alpha, beta);
}

int Solver::get_best_move(Position &pos) {
    int score = solve_weak(pos);

    for (int col = 0; col < BOARD_WIDTH; col++) {
        if (pos.is_move_valid(col)) {
            board before_move = pos.move(col);
            int child_score = -solve_weak(pos);
            pos.unmove(before_move);

            if (child_score >= score) {
                return col;
            }
        }
    }
    
    assert(0);

    return 0;
}


int Solver::solve_weak(Position &pos) {
    int result = solve(pos, -1, 1);

    if (result > 0) {
        return 1;
    } else if (result < 0) {
        return -1;
    } else {
        return 0;
    }
}

int Solver::solve_strong(Position &pos) {
    return solve(pos, MIN_SCORE, MAX_SCORE);
}

int Solver::solve_verbose(Position &pos) {
    printf("Using a %d x %d board and %.2f GB table.\n",
        BOARD_WIDTH, BOARD_HEIGHT, table.get_size_in_gigabytes());

    printf("Solving:\n");
    pos.printb();

    unsigned long start_time = clock();
    int score = solve_strong(pos);
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
    printf("    Hit rate         = %6.2f%%\n", table.get_hit_rate() * 100);
    printf("    Collision rate   = %6.2f%%\n", table.get_collision_rate() * 100);
    printf("    Density          = %6.2f%%\n", table.get_density() * 100);
    printf("    Rewrite rate     = %6.2f%%\n", table.get_rewrite_rate() * 100);
    printf("    Overwrite rate   = %6.2f%%\n", table.get_overwrite_rate() * 100);
    printf("Best moves guessed   = %6.2f%%\n", (double) get_num_best_moves_guessed() * 100 / get_num_interior_nodes());
    printf("Moves checked        = %6.2f%%\n", get_moves_checked_rate() * 100);

    return score;
}


unsigned long Solver::get_num_nodes() const {
    return stat_num_nodes;
}


unsigned long Solver::get_num_exact_nodes() const {
    return stat_num_exact_nodes;
}


unsigned long Solver::get_num_lower_nodes() const {
    return stat_num_lower_nodes;
}


unsigned long Solver::get_num_upper_nodes() const {
    return stat_num_upper_nodes;
}


unsigned long Solver::get_num_best_moves_guessed() const {
    return stat_num_best_moves_guessed;
}


unsigned long Solver::get_num_interior_nodes() const {
    return stat_num_interior_nodes;
}


double Solver::get_moves_checked_rate() const {
    return (double) stat_num_moves_checked / stat_num_child_nodes;
}
