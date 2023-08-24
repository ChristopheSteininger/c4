#ifndef PLAYER_RANDOM_H_
#define PLAYER_RANDOM_H_

#include <random>

#include "player.h"

#include "../solver/position.h"


class PlayerRandom : public Player {
public:
    PlayerRandom();
    int move(Position &pos) override;

private:
    std::mt19937 gen;
    std::uniform_int_distribution<> dis;
};

#endif
