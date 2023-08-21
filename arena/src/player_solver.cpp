#include <iostream>

#include "player_solver.h"

extern "C" {
    #include "../../solver/src/solver.h"
    #include "../../solver/src/table.h"
}


PlayerSolver::PlayerSolver() {
    if (!allocate_table()) {
        throw std::runtime_error("Failed to allocate memory for transposition table");
    }
}


PlayerSolver::~PlayerSolver() {
    free_table();
}


int PlayerSolver::move(board player, board opponent) {
    return get_best_move(player, opponent);
}
