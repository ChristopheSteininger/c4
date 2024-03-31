#ifndef SOLVER_H_
#define SOLVER_H_

#include <string>

#include "pool.h"
#include "position.h"
#include "settings.h"
#include "table.h"

class Solver {
   public:
    ~Solver();

    int solve_weak(const Position &pos);
    int solve_strong(const Position &pos);
    int solve(const Position &pos, int lower, int upper);

    int get_best_move(const Position &pos);
    int get_principal_variation(const Position &pos, std::vector<int> &moves);

    const Stats &get_merged_stats() const { return pool.get_merged_stats(); }

    void clear_state();

    static std::string get_settings_string();

   private:
    // Every worker will make a copy of this table. This will give
    // each thread access to a shared table with thread local stats.
    Table table{};

    Pool pool{table};
};

#endif
