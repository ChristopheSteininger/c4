#include "progress.h"

#include <cassert>
#include <iostream>

#include "settings.h"

void Progress::started_search(int alpha, int beta) {
    assert(!search_running);

    if (print_progress_enabled) {
        std::cout << "Started a search in range [" << alpha << ", " << beta << "]." << std::endl;
    }

    search_start_time = std::chrono::steady_clock::now();
    search_running = true;
    min_num_moves = BOARD_WIDTH * BOARD_HEIGHT;
}

void Progress::completed_search(int score) {
    assert(search_running);

    std::unique_lock<std::mutex> lock(mutex);

    search_running = false;

    if (print_progress_enabled) {
        std::cout << "Finished search in " << seconds_since_search_start() << " s. Result is " << score << "." << std::endl
                  << std::endl;
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

        if (print_progress_enabled && seconds_since_search_start() > 1) {
            std::cout << "    Thread " << id << " finished a position at depth " << num_moves << " after "
                      << seconds_since_search_start() << " s." << std::endl;
        }
    }
}

long long Progress::seconds_since_search_start() {
    auto run_time = std::chrono::steady_clock::now() - search_start_time;
    
    return std::chrono::duration_cast<std::chrono::seconds>(run_time).count();
}
