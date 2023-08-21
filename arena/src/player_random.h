#ifndef PLAYER_RANDOM_H_
#define PLAYER_RANDOM_H_

#include <random>

#include "player.h"


class PlayerRandom : public Player {
public:
    PlayerRandom();
    int move(board, board) override;

private:
    std::mt19937 gen;
    std::uniform_int_distribution<> dis;
};

#endif
