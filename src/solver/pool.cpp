#include <cassert>
#include <algorithm>
#include <vector>
#include <cstdio>

#include "pool.h"
#include "table.h"
#include "settings.h"


Pool::Pool(const Table &parent_table) {
    result = std::make_shared<SearchResult>();

    for (int i = 0; i < NUM_THREADS; i++) {
        workers.push_back(std::make_unique<Worker>(i, parent_table, result));
    }
}


Pool::~Pool() {
    // Ensure each worker is idle so the threads can be joined.
    stop_all();
    wait_all();
}


int Pool::search(Position &pos, int alpha, int beta) {
    // No worker should still be running at this point.
    wait_all();

    result->reset();
    merged_stats.reset();

    int min = std::max(alpha, pos.score_loss());
    int max = std::min(beta, pos.score_win());

    // Spread out the workers over the input bounds.
    double window = min;
    double step = (double) (max - min) / workers.size();
    for (int i = 0; i < workers.size() && window < max; i++) {
        workers[i]->start(pos, min, max, (int) window, i);
        window += step;
    }

    // Block until any of the workers find the solution.
    int score = result->wait_for_result();

    // No need for the other workers to do anything else.
    stop_all();

    // Update stats by merging together all child stats.
    merge_stats();

    return score;
}


void Pool::print_pool_stats() const {
    printf("%-5s %10s %10s\n", "ID", "Active", "First");
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
