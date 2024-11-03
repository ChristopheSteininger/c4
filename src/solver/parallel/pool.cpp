#include "pool.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <vector>

#include "../settings.h"
#include "../table.h"

// Search returning this value means the search was cancelled.
inline constexpr int SEARCH_CANCELLED = 1001;

static int get_score_jitter(double window_step, size_t i) {
    // clang-format off
    if (window_step < 0.1) {
        return (i % 4) * 10000
            + (i % 5) * 1000
            + (i % 6) * 100
            + (i % 7) * 10
            + (i % 8);
    }

    if (window_step < 1.0) {
        return (i % 2) * 100
            + (i % 3) * 10
            + (i % 4);
    }

    return (i % 3) * 10
        + (i % 5);
    // clang-format off
}

Pool::Pool(const Table &parent_table, std::shared_ptr<Progress> progress) {
    this->result = std::make_shared<SearchResult>();
    for (int i = 0; i < NUM_THREADS; i++) {
        workers.push_back(std::make_unique<Worker>(i, parent_table, result, progress));
    }

    this->progress = std::move(progress);
}

Pool::~Pool() {
    // Ensure each worker is idle so the threads can be joined.
    stop_all();
    wait_all();
}

int Pool::search(const Position &pos, int alpha, int beta) {
    assert(alpha < beta);

    assert(pos.score_loss() <= alpha);
    assert(Position::MIN_SCORE <= alpha);
    assert(beta <= pos.score_win());
    assert(beta <= Position::MAX_SCORE);

    assert(!pos.is_game_over());
    assert(!pos.wins_this_move(pos.find_player_threats()));

    // Do not allow more than one search to run.
    std::unique_lock<std::mutex> lock(mutex);

    result->reset();

    // Start the clock.
    std::chrono::steady_clock::time_point search_start_time = std::chrono::steady_clock::now();
    progress->started_search(alpha, beta, search_start_time);

    // Pass the new position to the workers and start searching.
    for (size_t i = 0; i < workers.size(); i++) {
        int score_jitter = get_score_jitter((double) (beta - alpha) / workers.size(), i);

        workers[i]->start(pos, alpha, beta, score_jitter);
    }

    // Block until any of the workers find the solution.
    int score = result->wait_for_result();

    // No need for the other workers to do anything else.
    stop_all();
    wait_all();

    // Update stats by merging together all worker stats.
    Stats search_stats;
    search_stats.completed_search(search_start_time);
    merge_stats(search_stats);

    progress->completed_search(score, search_stats);

    return score;
}

void Pool::cancel() {
    result->notify_result(SEARCH_CANCELLED);
}

void Pool::stop_all() {
    for (const std::unique_ptr<Worker> &worker : workers) {
        worker->stop();
    }
}

void Pool::wait_all() {
    for (const std::unique_ptr<Worker> &worker : workers) {
        worker->wait();
    }
}

void Pool::merge_stats(Stats &search_stats) {
    // Merge all worker stats to form final stats of only the previous search.
    for (const std::unique_ptr<Worker> &worker : workers) {
        search_stats.merge(*worker->get_stats());
    }

    // Update merged stats with stats of the previous search.
    merged_stats.merge(search_stats);
}
