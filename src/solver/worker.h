#ifndef WORKER_H_
#define WORKER_H_

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>

#include "position.h"
#include "search.h"
#include "stats.h"

// A thread safe wrapper for the result of a search.
class SearchResult {
   public:
    void reset();
    bool notify_result(int result);
    int wait_for_result();

   private:
    int score{SEARCH_STOPPED};
    std::atomic<bool> found{false};

    std::mutex mutex;
    std::condition_variable cond;
};

class Worker {
   public:
    Worker(int id, const Table &parent_table, const std::shared_ptr<SearchResult> result);
    ~Worker();

    void start(const Position &new_pos, int new_alpha, int new_beta, int new_move_offset);
    void wait();
    void stop();

    const Stats *get_stats() const { return stats.get(); }
    void reset_stats();

    void print_thread_stats();

   private:
    int id;
    std::thread thread;

    // Result is used to wake the main thread when we found the solution, and is shared
    // across all workers.
    std::shared_ptr<SearchResult> result;

    // Stats tracks the performance of the search on a single thread and is shared
    // only with other objects on the same thread.
    std::shared_ptr<Stats> stats;

    int solutions_found{0};

    // The object which is responsible for the single threaded search of a position.
    std::unique_ptr<Search> search;

    // These locks guard the shared search data. Both the main thread and the worker
    // thread use the following data.
    std::mutex mutex;
    std::condition_variable cond;

    bool is_searching{false};
    bool is_exiting{false};

    Position pos;
    int alpha;
    int beta;
    int score_jitter;
    // End shared search data.

    // Used to measure time the worker is active
    // vs time waiting for work.
    std::chrono::steady_clock::time_point start_time;
    std::chrono::steady_clock::duration active_time{};

    void work();
};

#endif
