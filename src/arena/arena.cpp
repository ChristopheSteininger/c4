#include <iostream>
#include <cassert>

#include "arena.h"


Arena::Arena(Player *p0, Player *p1) {
    this->p0 = p0;
    this->p1 = p1;
}


void Arena::play(int games) {
    for (int i = 0; i < games; i++) {
        Position pos = Position();

        pos.move(3);
        pos.move(3);
        pos.move(3);
        pos.move(3);
        
        int result = play_game(pos, p0, p1);

        total_games++;
        if (result == 1) {
            p0_wins++;
        } else if (result == -1) {
            p1_wins++;
        } else if (result == 0) {
            draws++;
        } else {
            assert(0);
        }

        printf("\r\tPlayer 1: Win rate = %3.1f%% (%5d), loss rate = %3.1f%% (%5d), draw rate = %3.1f%% (%5d), games = %5d",
            p0_wins * 100.0 / total_games,
            p0_wins,
            p1_wins * 100.0 / total_games,
            p1_wins,
            draws * 100.0 / total_games,
            draws,
            total_games
        );
        fflush(stdout);
    }

    std::cout << std::endl;
}


int Arena::play_game(Position &pos, Player *cur, Player *next) {
    int col = cur->move(pos);

    if (!pos.is_move_valid(col)) {
        std::cout << "Player tried to make an invalid move at column " << col << " at this position:" << std::endl;
        pos.printb();

        return -1;
    }

    pos.move(col);

    if (pos.has_opponent_won()) {
        return 1;
    }

    if (pos.is_draw()) {
        return 0;
    }

    return -play_game(pos, next, cur);
}
