#include "progress.h"

#include <cassert>
#include <iomanip>
#include <iostream>

#include "../settings.h"

void Progress::started_search(int alpha, int beta, std::chrono::steady_clock::time_point new_search_start_time) {
    std::unique_lock<std::mutex> lock(mutex);
    
    assert(!search_running);
    search_running = true;

    search_start_time = new_search_start_time;
    min_num_moves = BOARD_WIDTH * BOARD_HEIGHT;
    num_positions_at_min = 0;

    if (print_progress_enabled) {
        std::cout << "Searching in range [" << alpha << ", " << beta << "] . . ." << std::endl;
    }
}

void Progress::completed_node(int num_moves) {
    if (num_moves > min_num_moves || !search_running) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex);
    
    // Check again now that we have the lock if this thread improved or matched the min depth.
    if (num_moves > min_num_moves || !search_running) {
        return;
    }

    if (num_moves < min_num_moves) {
        min_num_moves = num_moves;
        num_positions_at_min = 1;
    } else {
        num_positions_at_min++;
    }

    // Only print an update if console output enabled, and enough time has passed to solve all trival positions.
    long long run_time_ms = milliseconds_since_search_start();
    if (print_progress_enabled && run_time_ms > 1000) {
        if (num_positions_at_min == 1) {
            std::cout << std::endl;
        } else {
            std::cout << "\r";
        }

        std::cout << std::fixed << std::setprecision(2)
                  << "  Solved " << num_positions_at_min << " positions with " << num_moves
                  << " moves after " << (run_time_ms / 1000.0) << " s."
                  << std::flush;
    }
}

void Progress::completed_search(int score, const Stats &stats) {
    std::unique_lock<std::mutex> lock(mutex);

    assert(search_running);
    search_running = false;

    if (print_progress_enabled) {
        std::cout << std::fixed << std::setprecision(2)
                  << std::endl
                  << "Search took " << (stats.get_search_time_ms() / 1000.0) << " s and explored "
                  << stats.get_num_nodes() << " nodes (" << stats.get_nodes_per_ms() << " nodes per ms)." << std::endl
                  << "Score is " << score << "." << std::endl
                  << std::endl;
    }
}

long long Progress::milliseconds_since_search_start() const {
    auto run_time = std::chrono::steady_clock::now() - search_start_time;
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(run_time).count();
}
