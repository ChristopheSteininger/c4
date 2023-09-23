#include "player_solver.h"

#include <iostream>

int PlayerSolver::move(Position &pos) { return solver.get_best_move(pos); }
