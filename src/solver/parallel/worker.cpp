#include "worker.h"

#include <atomic>
#include <cassert>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <memory>
#include <mutex>
#include <string>
#include <thread>

#include "../position.h"
#include "../util/os.h"

Worker::Worker(int id, const Table &parent_table, std::shared_ptr<SearchResult> result,
               std::shared_ptr<Progress> progress) {
    this->id = id;
    this->result = std::move(result);
    this->stats = std::make_shared<Stats>();
    this->search = std::make_unique<Search>(id, parent_table, stats, std::move(progress));

    // Start the thread, which will go to sleep until a position is submitted.
    this->thread = std::thread(&Worker::work, this);
    set_thread_affinity(thread, id);
}

Worker::~Worker() {
    std::unique_lock<std::mutex> lock(mutex);

    assert(!is_searching);
    assert(!is_exiting);

    is_exiting = true;

    lock.unlock();
    cond.notify_all();

    if (thread.joinable()) {
        thread.join();
    }
}

void Worker::start(const Position &new_pos, int new_alpha, int new_beta, int new_score_jitter) {
    assert(new_alpha < new_beta);
    assert(new_score_jitter >= 0);
    
    std::unique_lock<std::mutex> lock(mutex);

    // We should never try to start a search while another search is already
    // running.
    assert(!is_searching);
    assert(!is_exiting);

    // We are starting a new search, so reset all stats.
    stats->reset();

    // Position is not thread safe, so we must make our own copy.
    pos = Position(new_pos);
    alpha = new_alpha;
    beta = new_beta;
    score_jitter = new_score_jitter;

    // Tells the thread to start searching the given position as soon
    // as we wake it up.
    is_searching = true;
    search->start();

    lock.unlock();
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

void Worker::work() {
    std::unique_lock<std::mutex> lock(mutex);

    while (!is_exiting) {
        // Sleep until we have something to do.
        while (!is_searching && !is_exiting) {
            cond.wait(lock);
        }

        // We have a new position to search.
        if (is_searching) {
            int score = search->search(pos, alpha, beta, score_jitter);
            is_searching = false;

            // Tell the main thread we've solved the position.
            if (abs(score) != SEARCH_STOPPED) {
                result->notify_result(score);
            }
        }

        cond.notify_all();
    }
}
