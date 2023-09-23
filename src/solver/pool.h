#ifndef POOL_H_
#define POOL_H_

#include <memory>
#include <thread>
#include <vector>

#include "position.h"
#include "table.h"
#include "worker.h"

class Pool {
   public:
    Pool(const Table &parent_table);
    ~Pool();

    int search(Position &pos, int alpha, int beta);

    const Stats &get_merged_stats() const { return merged_stats; };
    void print_pool_stats() const;

   private:
    std::vector<std::unique_ptr<Worker>> workers;
    std::shared_ptr<SearchResult> result{};

    Stats merged_stats;

    void wait_all();
    void stop_all();
    void merge_stats();
};

#endif
