#ifndef SOLVER_H_
#define SOLVER_H_

#include "settings.h"
#include "position.h"
#include "pool.h"
#include "table.h"


class Solver {
public:
    ~Solver();
    int solve_weak(Position &pos, bool verbose = false);
    int solve_strong(Position &pos, bool verbose = false);

    int get_best_move(Position &pos);
    int get_principal_variation(Position &pos, std::vector<int> &moves);
    int get_num_moves_prediction(Position &pos, int score) const;
    
    const Stats &get_merged_stats() { return pool.get_merged_stats(); }

private:
    // Every worker will make a copy of this table. This will give
    // each thread access to a shared table with thread local stats.
    Table table;

    Pool pool{table};
};


#endif
