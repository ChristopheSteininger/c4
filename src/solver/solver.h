#ifndef SOLVER_H_
#define SOLVER_H_

#include <vector>

#include "position.h"
#include "table.h"


class Solver {
public:
    int solve_weak(Position &pos, bool verbose = false);
    int solve_strong(Position &pos, bool verbose = false);

    int get_best_move(Position &pos);
    int get_principal_variation(Position &pos, std::vector<int> &moves);
    int get_num_moves_prediction(Position &pos, int score) const;

    void reset_stats();

    double get_table_hit_rate() const;
    double get_table_collision_rate() const;
    double get_table_density() const;
    double get_table_rewrite_rate() const;
    double get_table_overwrite_rate() const;
    unsigned long get_num_nodes() const;
    unsigned long get_num_exact_nodes() const;
    unsigned long get_num_lower_nodes() const;
    unsigned long get_num_upper_nodes() const;
    unsigned long get_num_best_moves_guessed() const;
    unsigned long get_num_interior_nodes() const;
    double get_moves_checked_rate() const;

private:
    Table table = Table();

    unsigned long stat_num_nodes;
    unsigned long stat_num_child_nodes;
    unsigned long stat_num_exact_nodes;
    unsigned long stat_num_lower_nodes;
    unsigned long stat_num_upper_nodes;
    unsigned long stat_num_moves_checked;
    unsigned long stat_num_interior_nodes;
    unsigned long stat_num_best_moves_guessed;

    int negamax_entry(Position &pos, int alpha, int beta);
    int negamax(Position &pos, int alpha, int beta);
    int solve(Position &pos, int alpha, int beta, bool verbose = false);

    void print_pv_update(Position &pos, std::vector<int> &prev_pv, std::vector<int> &curr_pv);
};


#endif
