#include "worker.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "Tracy.hpp"
#include "position.h"

void SearchResult::reset() {
    std::unique_lock<std::mutex> lock(mutex);

    score = SEARCH_STOPPED;
    found.store(false);
}

bool SearchResult::notify_result(int result) {
    std::unique_lock<std::mutex> lock(mutex);

    // Do nothing if another thread already found the solution.
    if (found.load()) {
        return false;
    }

    this->score = result;
    found.store(true);

    lock.unlock();
    cond.notify_all();

    return true;
}

int SearchResult::wait_for_result() {
    ZoneScoped;

    std::unique_lock<std::mutex> lock(mutex);

    while (!found.load()) {
        cond.wait(lock);
    }

    assert(score != SEARCH_STOPPED);
    return score;
}

Worker::Worker(int id, const Table &parent_table, std::shared_ptr<SearchResult> result) {
    this->id = id;
    this->result = result;
    this->stats = std::make_shared<Stats>();
    this->search = std::make_unique<Search>(parent_table, stats);

    // Start the thread, which will go to sleep until a position is submitted.
    this->thread = std::thread(&Worker::work, this);
}

Worker::~Worker() {
    mutex.lock();

    assert(!is_searching);
    assert(!is_exiting);

    is_exiting = true;

    mutex.unlock();
    cond.notify_all();

    if (thread.joinable()) {
        thread.join();
    }
}

void Worker::start(const Position &new_pos, int new_alpha, int new_beta,
        int new_window, int new_step, int new_move_offset) {
    ZoneScoped;

    assert(new_alpha <= new_beta);
    assert(new_step > 0);
    assert(new_move_offset >= 0);

    mutex.lock();

    // We should never try to start a search while another search is already
    // running.
    assert(!is_searching);
    assert(!is_exiting);

    // Position is not thread safe, so we must make our own copy.
    pos = Position(new_pos);
    alpha = new_alpha;
    beta = new_beta;
    window = new_window;
    step = new_step;
    move_offset = new_move_offset;

    // Tells the thread to start searching the given position as soon
    // as we wake it up.
    is_searching = true;
    search->start();

    mutex.unlock();
    cond.notify_all();
}

void Worker::wait() {
    ZoneScoped;

    std::unique_lock<std::mutex> lock(mutex);

    // Block until the thread is done searching and
    // went back to sleep.
    while (is_searching) {
        cond.wait(lock);
    }
}

void Worker::stop() {
    ZoneScoped;

    if (is_searching) {
        search->stop();
    }
}

void Worker::reset_stats() {
    stats->reset();
}

void Worker::print_thread_stats() {
    auto now = std::chrono::high_resolution_clock::now();

    auto active_time_us = std::chrono::duration_cast<std::chrono::microseconds>(active_time);
    auto total_time_us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);

    double utilisation = active_time_us.count() * 100.0 / total_time_us.count();

    printf("%-5d %9.2f%% %10d\n", id, utilisation, solutions_found);
}

void Worker::work() {
    ZoneScoped;

    std::unique_lock<std::mutex> lock(mutex);
    start_time = std::chrono::high_resolution_clock::now();

    while (!is_exiting) {
        // Sleep until we have something to do.
        while (!is_searching && !is_exiting) {
            cond.wait(lock);
        }

        // We have a new position to search.
        if (is_searching) {
            auto search_start = std::chrono::high_resolution_clock::now();
            int score = run_search();
            active_time += std::chrono::high_resolution_clock::now() - search_start;

            is_searching = false;

            // Tell the main thread we've solved the position.
            if (score != SEARCH_STOPPED) {
                bool was_first = result->notify_result(score);

                if (was_first) {
                    solutions_found++;
                }
            }
        }

        cond.notify_all();
    }
}

int Worker::run_search() {
    ZoneScoped;

    int a = window;
    int b = window + 1;

    while (true) {
        int score = search->search(pos, a, b, move_offset);

        if (abs(score) == SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        // If the result is within the bounds, then the result is exact and we can exit.
        // Or if the true score is outside [alpha, beta] return the score.
        if ((a < score && score < b) || (score <= a && a <= alpha) || (score >= b && b >= beta)) {
            return score;
        }

        // Increase bounds for the next search.
        if (score <= a) {
            a = std::max(alpha, a - step);
        } else {
            b = std::min(beta, b + step);
        }
    }
}
