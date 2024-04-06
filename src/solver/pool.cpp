#include "pool.h"

#include <algorithm>
#include <cassert>
#include <iostream>
#include <iomanip>
#include <vector>

#include "settings.h"
#include "table.h"

Pool::Pool(const Table &parent_table, const std::shared_ptr<Progress> &progress) {
    result = std::make_shared<SearchResult>();

    for (int i = 0; i < NUM_THREADS; i++) {
        workers.push_back(std::make_unique<Worker>(i, parent_table, result, progress));
    }
}

Pool::~Pool() {
    // Ensure each worker is idle so the threads can be joined.
    stop_all();
    wait_all();
}

static int get_score_jitter(double window_step, size_t i) {
    // clang-format off
   if (window_step < 0.1) {
        return (i % 3) * BOARD_WIDTH * BOARD_WIDTH * BOARD_WIDTH * BOARD_WIDTH
            + (i % 4) * BOARD_WIDTH * BOARD_WIDTH * BOARD_WIDTH
            + (i % 5) * BOARD_WIDTH * BOARD_WIDTH
            + (i % 6) * BOARD_WIDTH
            + (i % 7);
    }

    if (window_step < 1.0) {
        return (i % 2) * BOARD_WIDTH * BOARD_WIDTH
            + (i % 3) * BOARD_WIDTH
            + (i % 4);
    }

    return (i % 3) * BOARD_WIDTH + (i % 5);
    // clang-format off
}

int Pool::search(const Position &pos, int alpha, int beta) {
    assert(alpha < beta);

    assert(pos.score_loss() <= alpha);
    assert(Position::MIN_SCORE <= alpha);
    assert(beta <= pos.score_win());
    assert(beta <= Position::MAX_SCORE);

    assert(!pos.is_game_over());
    assert(!pos.wins_this_move(pos.find_player_threats()));

    // No worker should still be running at this point.
    wait_all();

    result->reset();
    merged_stats.reset();

    // Pass the new position to the workers and start searching.
    for (size_t i = 0; i < workers.size(); i++) {
        int score_jitter = get_score_jitter((double) (beta - alpha) / workers.size(), i);

        workers[i]->start(pos, alpha, beta, score_jitter);
    }

    // Block until any of the workers find the solution.
    int score = result->wait_for_result();

    // No need for the other workers to do anything else.
    stop_all();

    // Update stats by merging together all child stats.
    merge_stats();

    return score;
}

void Pool::reset_stats() {
    for (const std::unique_ptr<Worker> &worker : workers) {
        worker->reset_stats();
    }
}

void Pool::print_pool_stats() const {
    std::cout << std::left << std::setw(5) << "ID"
        << std::right << std::setw(10) << "Active"
        << std::right << std::setw(10) << "First"
        << std::endl;
    for (const std::unique_ptr<Worker> &worker : workers) {
        worker->print_thread_stats();
    }
}

void Pool::wait_all() {
    for (const std::unique_ptr<Worker> &worker : workers) {
        worker->wait();
    }
}

void Pool::stop_all() {
    for (const std::unique_ptr<Worker> &worker : workers) {
        worker->stop();
    }
}

void Pool::merge_stats() {
    for (const std::unique_ptr<Worker> &worker : workers) {
        merged_stats.merge(*worker->get_stats());
    }
}
