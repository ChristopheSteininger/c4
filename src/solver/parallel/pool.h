#ifndef POOL_H_
#define POOL_H_

#include <memory>
#include <mutex>
#include <thread>
#include <vector>

#include "../position.h"
#include "../table.h"
#include "../util/progress.h"
#include "result.h"
#include "worker.h"

class Pool {
   public:
    Pool(const Table &parent_table, std::shared_ptr<Progress> progress);
    ~Pool();

    int search(const Position &pos, int alpha, int beta);
    void cancel();

    const Stats &get_merged_stats() const { return merged_stats; };
    void reset_stats() { merged_stats.reset(); }

    int get_num_workers() { return workers.size(); }

   private:
    std::vector<std::unique_ptr<Worker>> workers;
    std::shared_ptr<SearchResult> result{};
    std::shared_ptr<Progress> progress;

    // Prevent multiple searches running in parallel on the same thread pool.
    std::mutex mutex;

    // Merged stats contains the combined stats of all calls to Pool::search() since
    // the last call to Pool::reset_stats(). Useful for cases where multiple searches
    // were made on a single position.
    Stats merged_stats;
    int get_score_jitter(double window_step, size_t i);

    void stop_all();
    void wait_all();
    void merge_stats(Stats &search_stats);
};

#endif
