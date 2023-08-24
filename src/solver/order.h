#ifndef ORDER_H_
#define ORDER_H_

#include "position.h"

/**
 * Functions to guess the best move.
 */

int order_moves(Position &pos, int *moves, board non_losing_moves, int table_move);

#endif
