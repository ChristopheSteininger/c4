#ifndef SOLVER_H_
#define SOLVER_H_

#include "board.h"


int get_best_move(board, board);


int solve(board, board);


int solve_verbose(board, board);


unsigned long get_num_nodes();


unsigned long get_num_exact_nodes();


unsigned long get_num_lower_nodes();


unsigned long get_num_upper_nodes();


unsigned long get_num_best_moves_guessed();


unsigned long get_num_interior_nodes();


double get_moves_checked_rate();

#endif
