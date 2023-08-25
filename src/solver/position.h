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


class Position {
public:
    // Returns the first board after a move on top of the given column.
    board move(int col);

    // Returns the first board after making a move in the given bit.
    board move(board mask);

    // Undo the last move, given the result of the last call to move().
    void unmove(board before_move);

    int num_moves();

    // Returns true only if the current player will win or lose the game.
    bool has_player_won();
    bool has_opponent_won();

    // Returns true only if the players have drawn the game.
    bool is_draw();

    bool is_game_over();

    // Returns true only if either of the players does not have enough
    // space left to win the game.
    bool can_player_win();
    bool can_opponent_win();

    // Returns a 1 in any cell in which the player threatens a win,
    // even if the threat cannot be played this turn.
    board find_player_threats();
    board find_opponent_threats();
    board find_odd_even_threats(board threats);

    // Returns a 1 in any cell in which either player can win this move.
    board wins_this_move(board threats);

    // Returns a 1 in any cell in which the current playe can move without loosing next turn.
    board find_non_losing_moves(board opponent_threats);

    // Returns true only if the current player is allowed to play the given move.
    bool is_move_valid(int col);

    // Returns true only if the current player can play this move without loosing next turn.
    bool is_non_losing_move(board opponent_threats, int col);

    // Return a hash guaranteed to be unique to the position.
    board hash(bool &is_mirrored);

    // Print the game to the console.
    void printb();
    void print_mask(board b0, board b1);

    // Only used for testing. Returns true only if every dead stone
    // found cannot impact the rest of the game.
    bool are_dead_stones_valid();

private:
    // The current and next players position.
    board b0 = 0;
    board b1 = 0;

    int ply = 0;

    // Returns the input board reflected along the middle column.
    board mirror(board hash);

    // Returns a 1 in any cell which has no impact on the game.
    board find_dead_stones();

    // Returns true only if the board has valid column headers.
    bool is_board_valid();
};


#endif
