#include <assert.h>
#include <iostream>
#include <stdio.h>
#include <locale.h>
#include <chrono>

#include "settings.h"
#include "position.h"
#include "solver.h"
#include "table.h"


int main() {
    assert(BOARD_WIDTH * BOARD_HEIGHT_1 < 8 * sizeof(board));
    
    // Allow thousands separator.
    setlocale(LC_NUMERIC, "");

    Position pos;
    Solver solver;

    std::cout << "Using a " << BOARD_WIDTH << " x " << BOARD_HEIGHT << " board, a "
        << get_table_size() << " table, and " << NUM_THREADS << " threads." << std::endl;
    printf("Solving:\n");
    pos.printb();

    auto start_time = std::chrono::high_resolution_clock::now();
    int score = solver.solve_strong(pos, true);
    auto run_time = std::chrono::high_resolution_clock::now() - start_time;

    long long run_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(run_time).count();
    const Stats stats = solver.get_merged_stats();

    printf("\n");
    printf("Score is %d ", score);
    if (score < 0) {
        printf("(loss on move %d).\n", solver.get_num_moves_prediction(pos, score));
    } else if (score > 0) {
        printf("(win on move %d).\n", solver.get_num_moves_prediction(pos, score));
    } else {
        printf("(draw).\n");
    }

    printf("\n");
    printf("Time to solve        = %'.2f s\n", run_time_ms / 1000.0);
    printf("Nodes per ms         = %'.0f\n", stats.get_num_nodes() / (double) run_time_ms);
    printf("Nodes:\n");
    printf("    Exact            = %'lu\n", stats.get_num_exact_nodes());
    printf("    Lower            = %'lu\n", stats.get_num_lower_nodes());
    printf("    Upper            = %'lu\n", stats.get_num_upper_nodes());
    printf("    Total            = %'lu\n", stats.get_num_nodes());
    printf("Table:\n");
    printf("    Hit rate         = %6.2f%%\n", stats.get_hit_rate() * 100);
    printf("    Collision rate   = %6.2f%%\n", stats.get_collision_rate() * 100);
    printf("    Density          = %6.2f%%\n", stats.get_density() * 100);
    printf("    Rewrite rate     = %6.2f%%\n", stats.get_rewrite_rate() * 100);
    printf("    Overwrite rate   = %6.2f%%\n", stats.get_overwrite_rate() * 100);
    printf("Best moves guessed   = %6.2f%%\n", (double) stats.get_num_best_moves_guessed() * 100 / stats.get_num_interior_nodes());
    printf("Worst moves guessed  = %6.2f%%\n", (double) stats.get_num_worst_moves_guessed() * 100 / stats.get_num_interior_nodes());
}
