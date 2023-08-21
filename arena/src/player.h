#ifndef PLAYER_H_
#define PLAYER_H_

extern "C" {
    #include "../../solver/src/board.h"
}

class Player {
public:
    virtual ~Player() {};

    virtual int move(board, board) = 0;
};

#endif
