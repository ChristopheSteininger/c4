#ifndef PLAYER_H_
#define PLAYER_H_

#include "../solver/position.h"

class Player {
public:
    virtual ~Player() {};

    virtual int move(Position &pos) = 0;
};

#endif
