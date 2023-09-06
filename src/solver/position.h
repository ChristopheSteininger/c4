#ifndef BOARD_H_
#define BOARD_H_

#include <cstdint>
#include <algorithm>

#include "settings.h"


/**
 * Functions to manipulate the board.
 **/

// A number wide enough to store one bit for each cell on the board and the column headers.
#if (BOARD_HEIGHT_1 * BOARD_WIDTH > 64)
typedef __uint128_t board;
#else
typedef uint64_t board;
#endif
static_assert(BOARD_WIDTH * BOARD_HEIGHT <= 8 * sizeof(board));


inline static constexpr int score_win_at(const int ply) {
    return (BOARD_WIDTH * BOARD_HEIGHT - ply + 1) / 2;
}

inline static constexpr int score_loss_at(const int ply) {
    return -score_win_at(ply + 1);
}


class Position {
public:
    // Returns the first board after a move on top of the given column.
    board move(int col);

    // Returns the first board after making a move in the given bit.
    board move(board mask);

    // Undo the last move, given the result of the last call to move().
    void unmove(board before_move);

    // Returns the number of moves played.
    int num_moves() const;

    // Return true only if the game over conditions are met.
    bool has_player_won() const;
    bool has_opponent_won() const;
    bool is_draw() const;
    bool is_game_over() const;

    // Returns true only if either of the players does not have enough
    // space left to win the game.
    bool can_player_win() const;
    bool can_opponent_win() const;

    // Returns a 1 in any cell in which the player threatens a win,
    // even if the threat cannot be played this turn.
    board find_player_threats() const;
    board find_opponent_threats() const;
    board find_odd_even_threats(board threats) const;

    // Returns a 1 in any cell in which either player can win this move.
    board wins_this_move(board threats) const;

    // Returns a 1 in any cell in which the current playe can move without loosing next turn.
    board find_non_losing_moves(board opponent_threats) const;

    // Removes a single move for a mask of valid moves.
    board clear_move(board moves_mask, int move) const;

    // Returns true only if the current player is allowed to play the given move.
    bool is_move_valid(int col) const;

    // Returns true only if the current player can play this move without loosing next turn.
    bool is_non_losing_move(board opponent_threats, int col) const;

    // Returns the score if the game were won/lost after the given number of turns.
    inline int score_win(int turns = 0) const { return score_win_at(ply + turns); }
    inline int score_loss(int turns = 0) const { return score_loss_at(ply + turns); }

    // Return a hash guaranteed to be unique to the position.
    board hash(bool &is_mirrored) const;

    // Print the game to the console.
    void printb() const;
    void print_mask(board b0, board b1) const;

    // Only used for testing. Returns true only if every dead stone
    // found cannot impact the rest of the game.
    bool are_dead_stones_valid() const;

    // The score of winning or losing as early as possible.
    // The earliest possible win is on the 7th move.
    static const int MAX_SCORE = score_win_at(7);
    static const int MIN_SCORE = score_loss_at(7);

private:
    // The current and next players position.
    board b0 = 0;
    board b1 = 0;

    int ply = 0;

    // Returns the input board reflected along the middle column.
    board mirror(board hash) const;

    // Returns a 1 in any cell which has no impact on the game.
    board find_dead_stones() const;

    // Returns true only if the board has valid column headers.
    bool is_board_valid() const;
};


#endif
