/*

This program will generate random valid games of Connect 4.
Used to generate games for testing or benchmarking performance.

*/

#include <iostream>
#include <random>

#include "solver/position.h"
#include "solver/solver.h"


static inline constexpr int NUM_GAMES = 1000;

// Range of moves left until the end of the game if all moves are optimal. Min is inclusive, max is exclusive.
static inline constexpr int MIN_MOVES_LEFT = 3;
static inline constexpr int MAX_MOVES_LEFT = 14;

// Range of moves play in each generated game. Min is inclusive, max is exclusive.
static inline constexpr int MIN_MOVES = 29;
static inline constexpr int MAX_MOVES = BOARD_WIDTH * BOARD_HEIGHT - MIN_MOVES_LEFT + 1;


static_assert(MIN_MOVES_LEFT < MAX_MOVES_LEFT);

static_assert(0 < MIN_MOVES);
static_assert(MIN_MOVES < MAX_MOVES);
static_assert(MAX_MOVES <= 1 + BOARD_WIDTH * BOARD_HEIGHT - MIN_MOVES_LEFT);


static std::mt19937 engine{0};
static std::uniform_int_distribution<uint16_t> dist{};

static Solver solver{};


static void print_game(const Position &pos, int moves[], int score) {
    int moves_left = pos.score_to_last_move(score) - pos.num_moves();

    assert(MIN_MOVES <= pos.num_moves() && pos.num_moves() < MAX_MOVES);
    assert(MIN_MOVES_LEFT <= moves_left && moves_left < MAX_MOVES_LEFT);
    assert(!pos.is_game_over());

    for (int i = 0; i < pos.num_moves(); i++) {
        assert(0 <= moves[i] && moves[i] < BOARD_WIDTH);

        std::cout << moves[i];
    }

    std::cout << " " << score << std::endl;
}

static bool is_game_close_to_end(const Position &pos) {
    if (pos.num_moves() < 7) {
        return false;
    }

    int loss = pos.score_loss();
    int win = pos.score_win();

    // Do not play any move where we can force a win in the next turn.
    // Similarly, do not play any move where the opponent can force a win
    // in one turn.
    return solver.solve(pos, win - 1, win) >= win || solver.solve(pos, loss, loss + 1) <= loss;
}

static int get_random_move(Position &pos) {
    int possible_moves[BOARD_WIDTH];
    int num_valid_moves = 0;
    
    for (int i = 0; i < BOARD_WIDTH; i++) {
        if (pos.is_move_valid(i)) {
            board before_move = pos.move(i);

            if (!is_game_close_to_end(pos)) {
                possible_moves[num_valid_moves] = i;
                num_valid_moves++;
            }

            pos.unmove(before_move);
        }
    }

    if (num_valid_moves == 0) {
        return -1;
    }

    int move_index = dist(engine) % num_valid_moves;
    return possible_moves[move_index];
}

static bool try_random_game(int num_moves) {
    Position pos{};
    int moves[BOARD_WIDTH * BOARD_HEIGHT];

    for (int i = 0; i < num_moves; i++) {
        int move = get_random_move(pos);
        
        // Cannot keep playing if we could not find a move to play.
        if (move == -1) {
            return false;
        }

        pos.move(move);
        moves[i] = move;
    }

    // Check that the game has the right level of complexity.
    int score = solver.solve_strong(pos);
    int moves_left = pos.score_to_last_move(score) - num_moves;

    if (MIN_MOVES_LEFT <= moves_left && moves_left < MAX_MOVES_LEFT) {
        print_game(pos, moves, score);
        return true;
    }

    return false;
}

int main() {
    std::cout.imbue(std::locale(""));
    std::cout << solver.get_settings_string()
              << std::endl
              << "Generating " << NUM_GAMES << " random games:" << std::endl;

    int game_lengths[BOARD_WIDTH * BOARD_HEIGHT]{};

    for (int i = 0; i < NUM_GAMES; i++) {
        int num_moves = (dist(engine) % (MAX_MOVES - MIN_MOVES)) + MIN_MOVES;
        
        while (!try_random_game(num_moves));
        game_lengths[num_moves]++;
    }

    std::cout << "Done." << std::endl
              << std::endl
              << "Generated games by length:" << std::endl;
    
    for (int i = MIN_MOVES; i < MAX_MOVES; i++) {
        std::cout << i << ": " << game_lengths[i] << std::endl;
    }

    return 0;
}
