#ifndef SOLVER_H_
#define SOLVER_H_

#include "board.h"


int solve(board, board);


int solve_verbose(board, board);


unsigned long get_num_nodes();


double get_best_moves_guessed_rate();


double get_moves_checked_rate();

#endif
