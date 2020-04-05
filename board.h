#ifndef BOARD_H_
#define BOARD_H_

/**
 * Functions to manipulate the board.
 **/


// A number wide enough to store one bit for each cell on the board and the column headers.
typedef __uint128_t board;


extern const int BOARD_HEIGHT_1;

// 1 at the bottom of each column.
extern const board BOTTOM_ROW;


// Returns the first board after a move on top of the given column.
board move(board, board, int);


// Returns true only if the player has won the game.
int has_won(board);


// Returns true only the players have drawn the game.
int is_draw(board, board);


// Returns a 1 in any cell in which the first player threatens a win.
board find_threats(board, board);


// Returns true only if a player is allowed to play the given move.
int is_move_valid(board, board, int);


// Returns true only if the board has valid column headers.
int is_board_valid(board);


// Returns true only if the player has a piece on the cell.
int has_piece_on(board, int, int);


// Print the game to the console.
void printb(board, board);


#endif
