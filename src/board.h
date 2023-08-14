#ifndef BOARD_H_
#define BOARD_H_

#include <stdlib.h>

/**
 * Functions to manipulate the board.
 **/


// The height of the board + 1.
extern const int BOARD_HEIGHT_1;

// A number wide enough to store one bit for each cell on the board and the column headers.
#if (BOARD_HEIGHT_1 * BOARD_WIDTH > 64)
typedef __uint128_t board;
#else
typedef uint64_t board;
#endif

// 1 at each playable position of the first column.
extern const board FIRST_COLUMN;

// 1 at the playable position of the first column, plus the first column header.
extern const board FIRST_COLUMN_1;

// 1 at the bottom of each column.
extern const board BOTTOM_ROW;

// 1 on each playable posiition;
extern const board VALID_CELLS;


// Returns the first board after a move on top of the given column.
board move(board, board, int);


// Returns true only if the player has won the game.
int has_won(board);


// Returns true only the players have drawn the game.
int is_draw(board, board);


// Returns a 1 in any cell in which the first player threatens a win.
board find_threats(board, board);


board find_opportunities(board, board);


// Returns true only if a player is allowed to play the given move.
int is_move_valid(board, board, int);


// Returns the number of child states.
int get_num_valid_moves(board, board);


// Returns true only if the board has valid column headers.
int is_board_valid(board);


// Returns the input board reflected along the middle column.
board mirror(board);


// Returns a 1 in any cell which has no impact on the game.
board find_dead_stones(board, board);


// Returns a 1 in any which is part of a 4 in a row.
board find_winning_stones(board);


// Returns true only if the player has a piece on the cell.
int has_piece_on(board, int, int);


// Print the game to the console.
void printb(board, board);


#endif
