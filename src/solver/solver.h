#ifndef SOLVER_H_
#define SOLVER_H_

#include "position.h"
#include "table.h"


class Solver {
public:
    int get_best_move(Position &pos);
    int solve(Position &pos);
    int solve_verbose(Position &pos);

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
};


#endif
