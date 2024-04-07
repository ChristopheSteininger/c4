#include <assert.h>
#include <locale.h>

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>

#include "solver/position.h"
#include "solver/settings.h"
#include "solver/solver.h"
#include "solver/table.h"

// Use this bool to switch between providing a strong or weak solution to the chosen position.
static inline constexpr bool SOLVE_STRONGLY = true;

static std::string strongly_weakly() { return (SOLVE_STRONGLY) ? "Strongly" : "Weakly"; }

static std::string pretty_print_score(const Position &pos, int score) {
    std::stringstream result;

    if (SOLVE_STRONGLY) {
        result << "Strong score is " << score;

        int last_move = pos.num_moves() + pos.moves_left(score);
        if (score < 0) {
            result << " (loss on move #" << last_move << ").";
        } else if (score > 0) {
            result << " (win on move #" << last_move << ").";
        } else {
            result << " (draw).";
        }
    } else {
        result << "Weak score is " << score;

        if (score < 0) {
            result << " (loss).";
        } else if (score > 0) {
            result << " (win).";
        } else {
            result << " (draw).";
        }
    }

    return result.str();
}

int main() {
    std::cout.imbue(std::locale(""));

    Position pos;
    Solver solver;

    std::cout << Solver::get_settings_string()
              << strongly_weakly() << " solving:" << std::endl
              << std::endl
              << pos.display_board()
              << std::endl;

    solver.print_progress();

    auto start_time = std::chrono::steady_clock::now();
    int score = (SOLVE_STRONGLY) ? solver.solve_strong(pos) : solver.solve_weak(pos);
    auto run_time = std::chrono::steady_clock::now() - start_time;

    std::cout << pretty_print_score(pos, score) << std::endl
              << std::endl
              << solver.get_merged_stats().display_all_stats(run_time) << std::endl;

    return 0;
}
