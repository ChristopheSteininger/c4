#ifndef SOLVER_H_
#define SOLVER_H_

#include <string>
#include <memory>

#include "parallel/pool.h"
#include "position.h"
#include "progress.h"
#include "settings.h"
#include "table.h"

class Solver {
   public:
    Solver();
    Solver(const Solver &solver);

    int solve_weak(const Position &pos);
    int solve_strong(const Position &pos);
    int solve(const Position &pos, int lower, int upper);

    int get_best_move(const Position &pos, int score);
    int get_principal_variation(const Position &pos, std::vector<int> &moves);

    const Stats &get_merged_stats() const { return pool.get_merged_stats(); }
    void clear_state();

    void print_progress() { progress->print_progress(); }

    static std::string get_settings_string();

   private:
    std::shared_ptr<Progress> progress{std::make_shared<Progress>()};
    
    // Every worker will make a copy of this table. This will give
    // each thread access to a shared table with thread local stats.
    Table table;

    Pool pool;
};

#endif
