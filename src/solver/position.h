#ifndef BOARD_H_
#define BOARD_H_

#include <string>

#include "settings.h"
#include "types.h"

// Wins are scored higher if fewer moves were played. The minimum win score
// of +1 occurs when a player wins on their last move. The maximum score
// occurs if a player wins on their first move.
inline static constexpr int score_win_at(const int num_moves) noexcept {
    return 1 + (BOARD_WIDTH * BOARD_HEIGHT - num_moves) / 2;
}

class Position {
   public:
    // Returns the first board after a move on top of the given column.
    board move(int col) noexcept;

    // Returns the first board after making a move in the given bit.
    board move(board mask) noexcept;

    // Undo the last move, given the result of the last call to move().
    void unmove(board before_move) noexcept;

    // Undo the last move, given the column on which the last move was played.
    // Slower than unmove(board).
    void unmove(int last_col) noexcept;

    // Returns the number of moves played.
    inline int num_moves() const noexcept { return moves_played; };

    // Return true only if the game over conditions are met.
    bool has_player_won() const noexcept;
    bool has_opponent_won() const noexcept;
    bool is_draw() const noexcept;
    bool is_game_over() const noexcept;

    // Returns true only if either of the players does not have enough
    // space left to win the game.
    bool can_player_win() const noexcept;
    bool can_opponent_win() const noexcept;

    // Returns a 1 in any cell in which the player threatens a win,
    // even if the threat cannot be played this turn.
    board find_player_threats() const noexcept;
    board find_opponent_threats() const noexcept;

    board find_next_turn_threats(board threats) const noexcept;
    board find_next_next_turn_threats(board threats) const noexcept;

    // A threat above an opponent's threat is useless and will never win the game.
    inline board find_useful_threats(board player_threats, board opponent_threats) const noexcept {
        return player_threats & ~(opponent_threats << 1);
    }

    // Returns a 1 in any cell in which either player can win this move.
    board wins_this_move(board threats) const noexcept;

    // Returns a 1 in any cell in which the current player can move without loosing next turn.
    board find_non_losing_moves(board opponent_threats) const noexcept;

    // Checks if the next player can win the game next turn, regardless the current player's next move.
    bool is_forced_loss_next_turn(board opponent_wins, board non_losing_moves) const noexcept;

    // If the current player has only one move which does not loss immediately, returns that move.
    // Otherwise returns 0.
    board find_forced_move(board opponent_wins, board non_losing_moves) const noexcept;

    // See if the opponent could force a win by playing only on even cells. If so return an upper
    // bound on the score of the position.
    int upper_bound_from_evens_strategy() const noexcept;

    // Returns true only if the current player is allowed to play the given move.
    bool is_move_valid(int col) const noexcept;

    // Returns true only if the current player can play this move without loosing next turn.
    bool is_non_losing_move(board opponent_threats, int col) const noexcept;

    // Returns the score if the game were won/lost after the given number of turns.
    int score_win(int moves_until_win = 1) const noexcept;
    int score_loss(int moves_until_loss = 2) const noexcept;

    // Decode a score into number of remaining moves if both players are optimal.
    // The inverse of the `score_win` and `score_loss` functions.
    int moves_left(int score) const noexcept;

    inline bool is_same_player(const Position &other) const noexcept {
        return (moves_played & 1) == (other.moves_played & 1);
    }

    // Return a hash guaranteed to be unique to the position.
    board hash(bool &is_mirrored) const noexcept;

    // Print the game to the console.
    void print() const noexcept;
    void print_mask(board b0, board b1) const noexcept;
    std::string display_board() const noexcept;
    std::string display_mask(board b0, board b1) const noexcept;
    
    // Returns -1 if the given cell has been taken by the first player,
    // 1 if the cell has been taken by the second player, and 0 if empty.
    int get_player(int row, int col);

    // Only used for testing. Returns true only if every dead stone
    // found cannot impact the rest of the game.
    bool are_dead_stones_valid() const noexcept;

    // The score of winning or losing as early as possible.
    // The earliest possible win is on the 7th move.
    static constexpr int MAX_SCORE = score_win_at(7);
    static constexpr int MIN_SCORE = -MAX_SCORE;

    // Allow colors to be switched off if not displaying correctly.
#ifdef NO_COLOR_OUTPUT
    static constexpr const char *P0_STONE = "O";
    static constexpr const char *P1_STONE = "X";
#else
    static constexpr const char *P0_STONE = "\x1B[31mO\033[0m";
    static constexpr const char *P1_STONE = "\x1B[33mX\033[0m";
#endif

   private:
    // The current and next players position.
    board b0{0};
    board b1{0};

    int moves_played{0};

    // Returns the input board reflected along the middle column.
    board mirror(board hash) const noexcept;

    // Returns a 1 in any cell which has no impact on the game.
    board find_dead_stones() const noexcept;

    // Returns true only if the board has valid column headers.
    bool is_board_valid() const noexcept;
};

#endif
