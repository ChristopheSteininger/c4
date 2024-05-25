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
static inline constexpr int MIN_MOVES = 6;
static inline constexpr int MAX_MOVES = 14;


static_assert(MIN_MOVES_LEFT < MAX_MOVES_LEFT);

static_assert(0 < MIN_MOVES);
static_assert(MIN_MOVES < MAX_MOVES);
static_assert(MAX_MOVES <= 1 + BOARD_WIDTH * BOARD_HEIGHT - MIN_MOVES_LEFT);


static std::mt19937 engine{0};
static std::uniform_int_distribution<uint16_t> dist{};


static bool sanity_test(const Position& pos, int score) {
    if (pos.num_moves() < MIN_MOVES || pos.num_moves() >= MAX_MOVES) {
        std::cerr << "Error: search returned a position with incorrect number of moves played: "
            << pos.num_moves() << "." << std::endl;
        return false;
    }

    if (pos.moves_left(score) < MIN_MOVES_LEFT || pos.moves_left(score) >= MAX_MOVES_LEFT) {
        std::cerr << "Error: search returned a position with incorrect number of moves left: "
            << pos.moves_left(score) << "." << std::endl;
        return false;
    }

    return true;
}

static void print_game(const Position &pos, int moves[], int score) {
    // Clear the current line to print the generated game.
    std::cout << "\33[2K\r";

    for (int i = 0; i < pos.num_moves(); i++) {
        std::cout << moves[i];
    }

    std::cout << " " << score << std::endl;
}

static bool is_game_over_in_n_moves(Solver &solver, const Position &pos, int min_moves, int max_moves) {
    int min_move_score = pos.score_win(min_moves);
    int max_move_score = pos.score_win(max_moves);

    // MAX_SCORE represents the fewest number of moves possible to win a game (7 moves).
    // It is not possible for the game to be over in fewer moves than this.
    // Similarly, MIN_SCORE represents the few number of moves possible to lose a game.
    if (max_move_score >= Position::MAX_SCORE || Position::MIN_SCORE >= -max_move_score) {
        return false;
    }

    // Return true if the current player will win within `min_moves` to `max_moves`.
    int win_score = solver.solve(pos, max_move_score, min_move_score);
    if (max_move_score < win_score && win_score < min_move_score) {
        return true;
    }

    // Return true if the current player will lose within `min_moves` to `max_moves`.
    int loss_score = solver.solve(pos, -min_move_score, -max_move_score);
    return (-min_move_score < loss_score && loss_score < -max_move_score);
}

static int get_random_move(Solver &solver, Position &pos) {
    int possible_moves[BOARD_WIDTH];
    int num_valid_moves = 0;
    
    for (int i = 0; i < BOARD_WIDTH; i++) {
        if (pos.is_move_valid(i)) {
            board before_move = pos.move(i);

            // Do not play any move where we can force a win in the next turn.
            // Similarly, do not play any move where the opponent can force a win
            // in one turn.
            if (!is_game_over_in_n_moves(solver, pos, 0, 2)) {
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

static bool try_random_game(Solver &solver, int num_moves, int remaining_moves[]) {
    Position pos{};
    int moves[BOARD_WIDTH * BOARD_HEIGHT];

    for (int i = 0; i < num_moves; i++) {
        int move = get_random_move(solver, pos);

        // Cannot keep playing if we could not find a move to play.
        if (move == -1) {
            return false;
        }

        pos.move(move);
        moves[i] = move;
    }

    // Check that the game has the right level of complexity.
    if (is_game_over_in_n_moves(solver, pos, MIN_MOVES_LEFT, MAX_MOVES_LEFT - 1)) {
        int score = solver.solve_strong(pos);

        if (sanity_test(pos, score)) {
            remaining_moves[pos.moves_left(score)]++;
            print_game(pos, moves, score);

            return true;
        }
    }

    return false;
}

int main() {
    std::cout.imbue(std::locale(""));

    Solver solver{};
    int game_lengths[BOARD_WIDTH * BOARD_HEIGHT]{};
    int remaining_moves[BOARD_WIDTH * BOARD_HEIGHT]{};

    std::cout << Solver::get_settings_string()
              << std::endl
              << "Searching for games with " << MIN_MOVES << " <= moves played < " << MAX_MOVES << ", and "
              << MIN_MOVES_LEFT << " <= moves left < " << MAX_MOVES_LEFT << "." << std::endl
              << "Generating " << NUM_GAMES << " random games:" << std::endl;

    for (int i = 0; i < NUM_GAMES; i++) {
        int num_moves = (dist(engine) % (MAX_MOVES - MIN_MOVES)) + MIN_MOVES;
        
        for (int j = 0; !try_random_game(solver, num_moves, remaining_moves); j++) {
            std::cout << "\rGenerating game #" << (i + 1) << " with " << num_moves << " moves. Attempt #" << (j + 1);
        }

        game_lengths[num_moves]++;
    }

    std::cout << "Done." << std::endl
              << std::endl
              << "Generated games by length:" << std::endl;
    
    for (int i = MIN_MOVES; i < MAX_MOVES; i++) {
        std::cout << i << ": " << game_lengths[i] << std::endl;
    }

    std::cout << std::endl << "Generated games by moves left:" << std::endl;
    for (int i = MIN_MOVES_LEFT; i < MAX_MOVES_LEFT; i++) {
        std::cout << i << ": " << remaining_moves[i] << std::endl;
    }

    // Prevent console closing immediately after finishing on Windows.
    std::cout << "Press enter to exit." << std::endl;
    std::cin.get();

    return 0;
}
