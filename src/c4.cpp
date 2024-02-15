#include <assert.h>
#include <locale.h>
#include <stdio.h>

#include <chrono>
#include <iostream>

#include "solver/position.h"
#include "solver/settings.h"
#include "solver/solver.h"
#include "solver/table.h"

std::string pretty_print_score(Solver &solver, Position &pos, int score) {
    std::stringstream result;
    
    if (score < 0) {
        result << " (loss on move " << solver.get_num_moves_prediction(pos, score) << ").";
    } else if (score > 0) {
        result << " (win on move " << solver.get_num_moves_prediction(pos, score) << ").";
    } else {
        result << " (draw).";
    }

    return result.str();
}

int main() {
    Position pos;
    Solver solver;

    std::cout << "Using a " << BOARD_WIDTH << " x " << BOARD_HEIGHT << " board, a "
              << Table::get_table_size() << " table, and "
              << NUM_THREADS << " threads." << std::endl
              << "Solving:" << std::endl;
    pos.printb();
    std::cout << std::endl;

    auto start_time = std::chrono::high_resolution_clock::now();
    int score = solver.solve_strong(pos);
    auto run_time = std::chrono::high_resolution_clock::now() - start_time;

    long long run_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(run_time).count();
    const Stats stats = solver.get_merged_stats();

    std::cout.imbue(std::locale(""));
    std::cout << std::fixed << std::setprecision(2) << std::endl
              << "Score is " << score << pretty_print_score(solver, pos, score) << std::endl
              << std::endl
              << "Time to solve        = " << run_time_ms / 1000.0 << " s" << std::endl
              << "Nodes per ms         = " << stats.get_num_nodes() / (double)run_time_ms << std::endl
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
              << "Best moves guessed   = " << (double)stats.get_num_best_moves_guessed() * 100 / stats.get_num_interior_nodes() << "%" << std::endl
              << "Worst moves guessed  = " << (double)stats.get_num_worst_moves_guessed() * 100 / stats.get_num_interior_nodes() << "%" << std::endl;
}
