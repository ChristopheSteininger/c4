#include <assert.h>
#include <stdio.h>
#include <locale.h>
#include <time.h>

#include "settings.h"
#include "position.h"
#include "solver.h"


int main() {
    assert(BOARD_WIDTH * BOARD_HEIGHT_1 < 8 * sizeof(board));
    
    // Allow thousands separator.
    setlocale(LC_NUMERIC, "");

    Position pos;
    Solver solver;

    printf("Using a %d x %d board and %.2f GB table.\n\n",
        BOARD_WIDTH, BOARD_HEIGHT, solver.get_table_size_in_gigabytes());
    printf("Solving:\n");
    pos.printb();

    unsigned long start_time = clock();
    int score = solver.solve_strong(pos, true);
    double run_time_sec = (clock() - start_time) / (double) CLOCKS_PER_SEC;

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
    printf("Time to solve        = %'.0f s\n", run_time_sec);
    printf("Nodes per ms         = %'.0f\n", solver.get_num_nodes() / run_time_sec / 1000);
    printf("Nodes:\n");
    printf("    Exact            = %'lu\n", solver.get_num_exact_nodes());
    printf("    Lower            = %'lu\n", solver.get_num_lower_nodes());
    printf("    Upper            = %'lu\n", solver.get_num_upper_nodes());
    printf("    Total            = %'lu\n", solver.get_num_nodes());
    printf("Table:\n");
    printf("    Hit rate         = %6.2f%%\n", solver.get_table_hit_rate() * 100);
    printf("    Collision rate   = %6.2f%%\n", solver.get_table_collision_rate() * 100);
    printf("    Density          = %6.2f%%\n", solver.get_table_density() * 100);
    printf("    Rewrite rate     = %6.2f%%\n", solver.get_table_rewrite_rate() * 100);
    printf("    Overwrite rate   = %6.2f%%\n", solver.get_table_overwrite_rate() * 100);
    printf("Best moves guessed   = %6.2f%%\n", (double) solver.get_num_best_moves_guessed() * 100 / solver.get_num_interior_nodes());
    printf("Moves checked        = %6.2f%%\n", solver.get_moves_checked_rate() * 100);
}
