#ifndef SOLVER_H_
#define SOLVER_H_

#include <vector>

#include "pool.h"
#include "position.h"
#include "settings.h"
#include "table.h"

class Solver {
   public:
    ~Solver();
    int solve_weak(Position &pos);
    int solve_strong(Position &pos);

    int get_best_move(Position &pos);
    int get_principal_variation(Position &pos, std::vector<int> &moves);

    void get_all_samples(std::vector<Sample> &all_samples) const { pool.get_all_samples(all_samples); };
    const Stats &get_merged_stats() { return pool.get_merged_stats(); }

    void clear_state();

   private:
    // Every worker will make a copy of this table. This will give
    // each thread access to a shared table with thread local stats.
    Table table{};

    Pool pool{table};
};

#endif
