#ifndef PROGRESS_H_
#define PROGRESS_H_

#include <chrono>
#include <mutex>

#include "stats.h"

class Progress {
   public:
    void print_progress() { print_progress_enabled = true; }

    void started_search(int alpha, int beta, std::chrono::steady_clock::time_point new_search_start_time);
    void completed_node(int num_moves);
    void completed_search(int score, const Stats &stats);

   private:
    bool print_progress_enabled{false};
    std::chrono::steady_clock::time_point search_start_time;

    std::mutex mutex;
    bool search_running{false};
    int min_num_moves;
    int num_positions_at_min;

    long long milliseconds_since_search_start() const;
};

#endif
