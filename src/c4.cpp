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

static std::string pretty_print_score(Position &pos, int score) {
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
    Position pos;
    Solver solver;

    std::cout << Solver::get_settings_string()
              << strongly_weakly() << " solving:" << std::endl
              << std::endl;
    pos.printb();
    std::cout << std::endl;

    solver.print_progress();

    auto start_time = std::chrono::steady_clock::now();
    int score = (SOLVE_STRONGLY) ? solver.solve_strong(pos) : solver.solve_weak(pos);
    auto run_time = std::chrono::steady_clock::now() - start_time;

    long long run_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(run_time).count();
    const Stats stats = solver.get_merged_stats();

    std::cout.imbue(std::locale(""));
    std::cout << std::fixed << std::setprecision(2) << pretty_print_score(pos, score) << std::endl
              << std::endl
              << "Time to solve        = " << run_time_ms / 1000.0 << " s" << std::endl
              << "Nodes per ms         = " << stats.get_num_nodes() / std::max(1LL, run_time_ms) << std::endl
              << "Nodes:" << std::endl
              << "    Exact            = " << stats.get_num_exact_nodes() << std::endl
              << "    Lower            = " << stats.get_num_lower_nodes() << std::endl
              << "    Upper            = " << stats.get_num_upper_nodes() << std::endl
              << "    Total            = " << stats.get_num_nodes() << std::endl
              << "Table:" << std::endl
              << "    Hit rate         = " << stats.get_hit_rate() * 100 << "%" << std::endl
              << "    Collision rate   = " << stats.get_collision_rate() * 100 << "%" << std::endl
              << "    New write rate   = " << stats.get_new_write_rate() * 100 << "%" << std::endl
              << "    Rewrite rate     = " << stats.get_rewrite_rate() * 100 << "%" << std::endl
              << "    Overwrite rate   = " << stats.get_overwrite_rate() * 100 << "%" << std::endl
              << "Best moves guessed   = " << stats.get_best_move_guess_rate() * 100 << "%" << std::endl
              << "Worst moves guessed  = " << stats.get_worst_move_guess_rate() * 100 << "%" << std::endl;
}
