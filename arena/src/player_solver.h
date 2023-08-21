#ifndef PLAYER_SOLVER_H_
#define PLAYER_SOLVER_H_

#include "player.h"


class PlayerSolver : public Player {
public:
    PlayerSolver();
    ~PlayerSolver();

    int move(board, board) override;
};

#endif
