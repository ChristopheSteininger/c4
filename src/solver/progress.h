#ifndef PROGRESS_H_
#define PROGRESS_H_

#include <chrono>
#include <mutex>

#include "stats.h"

class Progress {
   public:
    void print_progress() { print_progress_enabled = true; }

    void started_search(int alpha, int beta);
    void completed_search(int score, const Stats &stats);
    void completed_node(int id, int num_moves);

   private:
    bool print_progress_enabled{false};

    std::chrono::steady_clock::time_point search_start_time;

    std::mutex mutex;
    volatile bool search_running{false};
    volatile int min_num_moves;

    long long milliseconds_since_search_start() const;
};

#endif
