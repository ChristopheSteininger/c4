#ifndef POOL_H_
#define POOL_H_

#include <memory>
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

    const Stats &get_merged_stats() const { return merged_stats; };
    void reset_stats() { merged_stats.reset(); }

   private:
    std::vector<std::unique_ptr<Worker>> workers;
    std::shared_ptr<SearchResult> result{};
    std::shared_ptr<Progress> progress;

    // Merged stats contains the combined stats of all calls to Pool::search() since
    // the last call to Pool::reset_stats(). Useful for cases where multiple searches
    // were made on a single position.
    Stats merged_stats;

    void stop_all();
    void wait_all();
    void merge_stats(Stats &search_stats);
};

#endif
