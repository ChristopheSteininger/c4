/*

Use this program to solve a single Connect 4 position.
Program will print the outcome of the game if both players play perfectly,
and print all collected search statistics.

*/

#include <iostream>
#include <sstream>

#include "solver/position.h"
#include "solver/settings.h"
#include "solver/solver.h"

// Use this bool to switch between providing a strong or weak solution
// to the chosen position.
//   * Weak solution: Will find if either player can force a win or if the game
//     will be a draw after perfect play.
//
//   * Strong solution: Gives the weak solution plus the move on which the
//     game will end assuming perfect play.
//     Slower than a weak solution.
static inline constexpr bool SOLVE_STRONGLY = true;

static std::string pretty_print_score(const Position &pos, int score) {
    std::stringstream result;

    if (SOLVE_STRONGLY) {
        result << "Final strong score is " << score;

        int last_move = pos.num_moves() + pos.moves_left(score);
        if (score < 0) {
            result << " (second player will win on move #" << last_move << ").";
        } else if (score > 0) {
            result << " (first player will win on move #" << last_move << ").";
        } else {
            result << " (draw).";
        }
    } else {
        result << "Final weak score is " << score;

        if (score < 0) {
            result << " (second player will win).";
        } else if (score > 0) {
            result << " (first player will win).";
        } else {
            result << " (draw).";
        }
    }

    return result.str();
}

int main() {
    std::cout.imbue(std::locale(""));

    Position pos{};
    Solver solver{};

    std::cout << solver.get_settings_string()
              << (SOLVE_STRONGLY ? "Strongly" : "Weakly") << " solving:" << std::endl
              << std::endl
              << pos.display_board()
              << std::endl;

    solver.print_progress();
    int score = SOLVE_STRONGLY
        ? solver.solve_strong(pos)
        : solver.solve_weak(pos);

    std::cout << "Search completed!" << std::endl
              << pretty_print_score(pos, score) << std::endl
              << std::endl
              << solver.get_merged_stats().display_all_stats() << std::endl;

    // Prevent console closing immediately after finishing on Windows.
    std::cout << "Press enter to exit." << std::endl;
    std::cin.get();

    return 0;
}
