#include <iostream>
#include <cassert>

#include "arena.h"

extern "C" {
    #include "../../solver/src/board.h"
}


Arena::Arena(Player *p0, Player *p1) {
    this->p0 = p0;
    this->p1 = p1;
}


void Arena::play(int games) {
    board b0 = 0;
    board b1 = 0;

    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    for (int i = 0; i < games; i++) {
        int result = play_game(b0, b1, p0, p1);

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


int Arena::play_game(board b0, board b1, Player *cur, Player *next) {
    int col = cur->move(b0, b1);

    if (!is_move_valid(b0, b1, col)) {
        std::cout << "Player tried to make an invalid move at column " << col << " at this position:" << std::endl;
        printb(b0, b1);

        return -1;
    }

    board after_move = move(b0, b1, col);

    if (has_won(after_move)) {
        return 1;
    }

    if (is_draw(after_move, b1)) {
        return 0;
    }

    return -play_game(b1, after_move, next, cur);
}
