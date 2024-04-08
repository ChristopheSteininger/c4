#include "progress.h"

#include <cassert>
#include <iomanip>
#include <iostream>

#include "settings.h"

void Progress::started_search(int alpha, int beta) {
    std::unique_lock<std::mutex> lock(mutex);
    
    assert(!search_running);
    search_running = true;

    if (print_progress_enabled) {
        std::cout << "Searching in range [" << alpha << ", " << beta << "] . . ." << std::endl;
    }

    search_start_time = std::chrono::steady_clock::now();
    min_num_moves = BOARD_WIDTH * BOARD_HEIGHT;
}

void Progress::completed_search(int score, const Stats &stats) {
    std::unique_lock<std::mutex> lock(mutex);

    assert(search_running);
    search_running = false;

    if (print_progress_enabled) {
        long long run_time_ms = milliseconds_since_search_start();
        long long nodes_per_ms = stats.get_num_nodes() / std::max(1LL, run_time_ms);

        std::cout << std::fixed << std::setprecision(2) 
                  << "Search took " << (run_time_ms / 1000.0) << " s and explored " << stats.get_num_nodes()
                  << " nodes (" << nodes_per_ms << " nodes per ms)." << std::endl
                  << "Score is " << score << "." << std::endl << std::endl;
    }
}

void Progress::completed_node(int id, int num_moves) {
    // min_num_moves is accessed by other threads, but will only be decreased so no need
    // to aquire a lock if this check is true.
    if (num_moves >= min_num_moves || !search_running) {
        return;
    }

    std::unique_lock<std::mutex> lock(mutex);

    if (num_moves < min_num_moves && search_running) {
        min_num_moves = num_moves;

        long long run_time_ms = milliseconds_since_search_start();
        if (print_progress_enabled && run_time_ms > 1000) {
            std::cout << std::fixed << std::setprecision(2)
                      << "  Thread " << id << " finished a position at depth "
                      << num_moves << " after " << (run_time_ms / 1000.0) << " s." << std::endl;
        }
    }
}

long long Progress::milliseconds_since_search_start() const {
    auto run_time = std::chrono::steady_clock::now() - search_start_time;
    
    return std::chrono::duration_cast<std::chrono::milliseconds>(run_time).count();
}
