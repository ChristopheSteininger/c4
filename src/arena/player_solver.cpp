#include <iostream>

#include "player_solver.h"


int PlayerSolver::move(Position &pos) {
    return solver.get_best_move(pos);
}
