#ifndef PLAYER_SOLVER_H_
#define PLAYER_SOLVER_H_

#include "player.h"

#include "../solver/solver.h"
#include "../solver/position.h"


class PlayerSolver : public Player {
public:
    int move(Position &pos) override;

private:
    Solver solver = Solver();
};

#endif
