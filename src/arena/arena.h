#ifndef ARENA_H_
#define ARENA_H_

#include "player.h"
#include "../solver/position.h"


class Arena {
public:
    Arena(Player *, Player *);

    void play(int);

private:
    Player *p0;
    Player *p1;

    int p0_wins = 0;
    int p1_wins = 0;
    int draws = 0;

    int total_games = 0;

    int p0_calc_millis = 0;
    int p1_calc_millis = 0;

    int play_game(Position &pos, Player *cur, Player *next);
};


#endif
