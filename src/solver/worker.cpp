#include <thread>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cassert>
#include <string>
#include <chrono>
#include <cstdio>

#include "Tracy.hpp"

#include "worker.h"


void SearchResult::reset() {
    std::unique_lock<std::mutex> lock(mutex);

    score = SEARCH_STOPPED;
    found.store(false);
}


bool SearchResult::notify_result(int score) {
    std::unique_lock<std::mutex> lock(mutex);

    // Do nothing if another thread already found the solution.
    if (found.load()) {
        return false;
    }

    this->score = score;
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


void Worker::start(const Position &pos, int alpha, int beta, int window, int step, int move_offset) {
    ZoneScoped;

    assert(alpha <= beta);
    assert(step > 0);
    assert(move_offset >= 0);

    mutex.lock();

    // We should never try to start a search while another search is already
    // running.
    assert(!is_searching);
    assert(!is_exiting);

    // Tells the thread to start searching the given position as soon
    // as we wake it up.
    this->is_searching = true;
    this->search->start();

    // Position is not thread safe, so we must make our own copy.
    this->pos = Position(pos);
    this->alpha = alpha;
    this->beta = beta;
    this->window = window;
    this->step = step;
    this->move_offset = move_offset;

    mutex.unlock();
    cond.notify_one();
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


void Worker::print_thread_stats() {
    auto now = std::chrono::high_resolution_clock::now();

    auto active_time_us = std::chrono::duration_cast<std::chrono::microseconds>(active_time);
    auto total_time_us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);

    double utilisation = active_time_us.count() * 100.0 / total_time_us.count();

    printf("%-5d %'9.2f%% %'10d\n",
        id, utilisation, solutions_found);
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
            if (abs(score) != -SEARCH_STOPPED) {
                bool was_first = result->notify_result(score);

                if (was_first) {
                    solutions_found++;
                }
            }
        }

        cond.notify_one();
    }
}


int Worker::run_search() {
    ZoneScoped;

    int a = window;
    int b = window + 1;

    while (true) {
        int score = search->search(pos, a, b, move_offset);

        if (score == SEARCH_STOPPED) {
            return SEARCH_STOPPED;
        }

        // If the result is within the bounds, then the result is exact and we can exit.
        // Or if the true score is outside [alpha, beta] return the score.
        if ((a < score && score < b)
                || (score <= a && a <= alpha)
                || (score >= b && b >= beta)) {
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
