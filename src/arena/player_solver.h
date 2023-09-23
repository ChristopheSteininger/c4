#ifndef PLAYER_SOLVER_H_
#define PLAYER_SOLVER_H_

#include "../solver/position.h"
#include "../solver/solver.h"
#include "player.h"

class PlayerSolver : public Player {
   public:
    int move(Position &pos) override;

   private:
    Solver solver = Solver();
};

#endif
