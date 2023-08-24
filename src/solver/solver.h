#ifndef SOLVER_H_
#define SOLVER_H_

#include "position.h"
#include "table.h"


class Solver {
public:
    int solve_weak(Position &pos, bool verbose = false);
    int solve_strong(Position &pos, bool verbose = false);
    int solve_verbose(Position &pos);

    int get_best_move(Position &pos);
    int get_principal_variation(Position &pos, int *moves);

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

    int negamax(Position &pos, int alpha, int beta);
    int solve(Position &pos, int alpha, int beta, bool verbose = false);

    void print_pv_update(Position &pos, int *pv0, int *pv1, int *pv0_size, int *pv1_size, int &cur_pv);
};


#endif
