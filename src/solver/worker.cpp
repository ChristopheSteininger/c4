#include "worker.h"

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "position.h"
#include "os.h"

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
    std::unique_lock<std::mutex> lock(mutex);

    while (!found.load()) {
        cond.wait(lock);
    }

    assert(score != SEARCH_STOPPED);
    return score;
}

Worker::Worker(int id, const Table &parent_table, std::shared_ptr<SearchResult> result,
               const std::shared_ptr<Progress> &progress) {
    this->id = id;
    this->result = std::move(result);
    this->stats = std::make_shared<Stats>();
    this->search = std::make_unique<Search>(id, parent_table, stats, progress);

    // Start the thread, which will go to sleep until a position is submitted.
    this->thread = std::thread(&Worker::work, this);
    set_thread_affinity(thread, id);
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

void Worker::start(const Position &new_pos, int new_alpha, int new_beta, int new_score_jitter) {
    assert(new_alpha < new_beta);
    assert(new_score_jitter >= 0);

    mutex.lock();

    // We should never try to start a search while another search is already
    // running.
    assert(!is_searching);
    assert(!is_exiting);

    // Position is not thread safe, so we must make our own copy.
    pos = Position(new_pos);
    alpha = new_alpha;
    beta = new_beta;
    score_jitter = new_score_jitter;

    // Tells the thread to start searching the given position as soon
    // as we wake it up.
    is_searching = true;
    search->start();

    mutex.unlock();
    cond.notify_all();
}

void Worker::wait() {
    std::unique_lock<std::mutex> lock(mutex);

    // Block until the thread is done searching and
    // went back to sleep.
    while (is_searching) {
        cond.wait(lock);
    }
}

void Worker::stop() {
    if (is_searching) {
        search->stop();
    }
}

void Worker::reset_stats() {
    stats->reset();
}

void Worker::print_thread_stats() {
    auto now = std::chrono::steady_clock::now();

    auto active_time_us = std::chrono::duration_cast<std::chrono::microseconds>(active_time);
    auto total_time_us = std::chrono::duration_cast<std::chrono::microseconds>(now - start_time);

    double utilisation = active_time_us.count() * 100.0 / total_time_us.count();

    std::cout << std::left << std::setw(5) << id
        << std::right << std::setw(9) << std::fixed << std::setprecision(2) << utilisation << "%"
        << std::right << std::setw(10) << solutions_found
        << std::endl;
}

void Worker::work() {
    std::unique_lock<std::mutex> lock(mutex);
    start_time = std::chrono::steady_clock::now();

    while (!is_exiting) {
        // Sleep until we have something to do.
        while (!is_searching && !is_exiting) {
            cond.wait(lock);
        }

        // We have a new position to search.
        if (is_searching) {
            auto search_start = std::chrono::steady_clock::now();
            int score = search->search(pos, alpha, beta, score_jitter);
            active_time += std::chrono::steady_clock::now() - search_start;

            is_searching = false;

            // Tell the main thread we've solved the position.
            if (abs(score) != SEARCH_STOPPED) {
                bool was_first = result->notify_result(score);

                if (was_first) {
                    solutions_found++;
                }
            }
        }

        cond.notify_all();
    }
}
