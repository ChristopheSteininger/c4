#include <iostream>
#include <stdio.h>
#include <assert.h>

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


static int score_loss(Position &pos, int turns) {
    return (-BOARD_WIDTH * BOARD_HEIGHT + turns + pos.num_moves()) / 2;
}


static int score_win(Position &pos, int turns) {
    return (BOARD_WIDTH * BOARD_HEIGHT + 1 - turns - pos.num_moves()) / 2;
}


static int get_any_move(Position &pos) {
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());

    for (int i = 0; i < BOARD_WIDTH; i++) {
        if (pos.is_move_valid(i)) {
            return i;
        }
    }

    assert(0);
    return -1;
}


static int get_move_from_mask(board b) {
    assert(b != 0);

    for (int i = 0; i < BOARD_WIDTH; i++) {
        board mask = FIRST_COLUMN << (i * BOARD_HEIGHT_1);
        if (b & mask) {
            return i;
        }
    }

    assert(0);
    return -1;
}


static bool arrays_equal(int *a, int *b, int length) {
    for (int i = 0; i < 0; i++) {
        if (a[i] != b[i]) {
            return false;
        }
    }

    return true;
}


int Solver::negamax_entry(Position &pos, int alpha, int beta) {
    if (pos.has_opponent_won()) {
        return -score_loss(pos, -1);
    }

    if (pos.has_player_won()) {
        return -score_win(pos, -1);
    }
    
    if (pos.is_draw()) {
        return 0;
    }

    // If the player can win this move, then end the game.
    board player_threats = pos.find_player_threats();
    if (pos.wins_this_move(player_threats)) {
        return score_win(pos, 0);
    }

    return negamax(pos, alpha, beta);
}


int Solver::negamax(Position &pos, int alpha, int beta) {
    assert(alpha < beta);
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());

    stat_num_nodes++;

    int original_alpha = alpha;
    int original_beta = beta;

    // If there are too few empty spaces left on the board for the player to win, then the best
    // score possible is a draw.
    if (!pos.can_player_win()) {
        beta = min(beta, 0);
    }
    if (!pos.can_opponent_win()) {
        alpha = max(alpha, 0);
    }
    if (alpha >= beta) {
        return 0;
    }

    // The minimum score possible increases each turn.
    alpha = max(alpha, score_loss(pos, -2));
    if (alpha >= beta) {
        return alpha;
    }

    // If the player can only move below the opponents threats, the player will lose.
    board opponent_threats = pos.find_opponent_threats();
    board non_losing_moves = pos.find_non_losing_moves(opponent_threats);
    if (non_losing_moves == 0) {
        return score_loss(pos, 0);
    }

    // Check if the opponent could win next move.
    board opponent_wins = pos.wins_this_move(opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            return score_loss(pos, 0);
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            return score_loss(pos, 0);
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
    bool lookup_success = table.get(pos, lookup_move, lookup_type, lookup_value);
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

int Solver::get_best_move(Position &pos) {
    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());

    // This method uses the results written to the t-table by the negamax function
    // to find the best move. However, the table does not store trival positions which
    // can be solved by static analysis. For these positions we need find the best
    // move ourselves.

    // If neither player will be able to win, moves don't matter anymore.
    if (!pos.can_player_win() && !pos.can_opponent_win()) {
        return get_any_move(pos);
    }

    // If the player can win this move, then end the game.
    board player_threats = pos.find_player_threats();
    board player_wins = pos.wins_this_move(player_threats);
    if (player_wins) {
        return get_move_from_mask(player_wins);
    }

    // If the player can only move below the opponents threats, the player will lose.
    board opponent_threats = pos.find_opponent_threats();
    board non_losing_moves = pos.find_non_losing_moves(opponent_threats);
    if (non_losing_moves == 0) {
        return get_any_move(pos);
    }

    // Check if the opponent could win next move.
    board opponent_wins = pos.wins_this_move(opponent_threats);
    if (opponent_wins) {
        // If the opponent has multiple threats, then the game is lost.
        if (opponent_wins & (opponent_wins - 1)) {
            return get_any_move(pos);
        }

        // If the opponent has two threats on top of each other, then the game is also lost.
        if (!(opponent_wins & non_losing_moves)) {
            return get_any_move(pos);
        }

        // Otherwise, the opponent has only one threat, and the player must block the threat.
        else {
            return get_move_from_mask(opponent_wins);
        }
    }

    // Otherwise this is a complex position and will be stored in the table.
    int best_move, type, value;
    
    bool lookup_success = table.get(pos, best_move, type, value);
    if (lookup_success && type == TYPE_EXACT) {
        return best_move;
    }

    // The results are occasionally overrwritten. If so start another search which
    // will write the best move into the table before returning.
    int score = negamax_entry(pos, -INFINITY, INFINITY);

    lookup_success = table.get(pos, best_move, type, value);
    if (lookup_success && type == TYPE_EXACT) {
        return best_move;
    }

    // If we still have a miss, then try each move until we find a move which
    // gives the same score as the position.
    for (int move = 0; move < BOARD_WIDTH; move++) {
        if (pos.is_move_valid(move)) {
            board before_move = pos.move(move);
            int child_score = -negamax_entry(pos, -INFINITY, INFINITY);
            pos.unmove(before_move);

            if (child_score == score) {
                return move;
            }
        }
    }


    // This point should never be reached.
    std::cout << "Error: could not get a best move. In this position:" << std::endl;
    pos.printb();

    assert(0);
    return -1;
}


int Solver::solve(Position &pos, int alpha, int beta, bool verbose) {
    // There is no point in check simple conditions like win in one move
    // during search so handle these here.
    if (pos.has_player_won()
            || pos.wins_this_move(pos.find_player_threats())) {
        return score_win(pos, 0);
    }
    if (pos.has_opponent_won()) {
        return score_loss(pos, 0);
    }
    if (pos.is_draw()) {
        return 0;
    }

    alpha = max(alpha, score_loss(pos, 0));
    beta = min(beta, score_win(pos, 0));

    int a = 0;
    int b = 1;

    int cur_pv = 0;
    int pv0_size, pv1_size;
    int pv0[BOARD_WIDTH * BOARD_HEIGHT];
    int pv1[BOARD_WIDTH * BOARD_HEIGHT];
    pv1[0] = -1;  // To ensure first iteration prints PV.

    while (a > alpha || b < beta) {
        int result = negamax(pos, a, b);

        if (verbose) {
            std::cout << "Completed search in [" << a << ", " << b << "]. " << "Score is " << result << ". ";
            print_pv_update(pos, pv0, pv1, &pv0_size, &pv1_size, cur_pv);
        }

        a--;
        b++;
    }

    return negamax(pos, alpha, beta);
}


int Solver::get_principal_variation(Position &pos, int *moves) {
    Position pv = Position(pos);

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        if (pv.is_game_over()) {
            return i;
        }

        int best_move = get_best_move(pv);

        moves[i] = best_move;
        pv.move(best_move);
    }

    assert(0);
    return -1;
}


int Solver::get_num_moves_prediction(Position &pos, int score) const {
    // Run the calculation from the perspective of the first player.
    if (pos.num_moves() & 1) {
        score *= -1;
    }

    if (score > 0) {
        return BOARD_WIDTH * BOARD_HEIGHT - (score * 2) + 1;
    } else if (score < 0) {
        return BOARD_WIDTH * BOARD_HEIGHT + ((score + 1) * 2);
    } else {
        return BOARD_WIDTH * BOARD_HEIGHT;
    }
}


int Solver::solve_weak(Position &pos, bool verbose) {
    int result = solve(pos, -1, 1, verbose);

    if (result > 0) {
        return 1;
    } else if (result < 0) {
        return -1;
    } else {
        return 0;
    }
}


int Solver::solve_strong(Position &pos, bool verbose) {
    return solve(pos, MIN_SCORE, MAX_SCORE, verbose);
}


void Solver::print_pv_update(Position &pos, int *pv0, int *pv1, int *pv0_size, int *pv1_size, int &cur_pv) {
    int *pv = pv0;
    int *pv_size = pv0_size;

    if (cur_pv == 1) {
        pv = pv1;
        pv_size = pv1_size;
    }
    cur_pv = 1 - cur_pv;

    *pv_size = get_principal_variation(pos, pv);

    if (*pv0_size != *pv1_size || !arrays_equal(pv0, pv1, *pv_size)) {
        std::cout << "Principal variation is:" << std::endl;
        for (int i = 0; i < *pv_size; i++) {
            std::cout << pv[i] << " ";
        }
        std::cout << std::endl << std::endl;
    } else {
        std::cout << "Principal variation is unchanged." << std::endl;
    }
}


void Solver::reset_stats() {
    stat_num_nodes = 0;
    stat_num_child_nodes = 0;
    stat_num_exact_nodes = 0;
    stat_num_lower_nodes = 0;
    stat_num_upper_nodes = 0;
    stat_num_moves_checked = 0;
    stat_num_interior_nodes = 0;
    stat_num_best_moves_guessed = 0;
}


double Solver::get_table_size_in_gigabytes() const {
    return table.get_size_in_gigabytes();
}


double Solver::get_table_hit_rate() const {
    return table.get_hit_rate();
}


double Solver::get_table_collision_rate() const {
    return table.get_collision_rate();
}


double Solver::get_table_density() const {
    return table.get_density();
}


double Solver::get_table_rewrite_rate() const {
    return table.get_rewrite_rate();
}


double Solver::get_table_overwrite_rate() const {
    return table.get_overwrite_rate();
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
