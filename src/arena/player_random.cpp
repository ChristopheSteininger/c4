#include "player_random.h"

#include <iostream>

PlayerRandom::PlayerRandom() : dis(0, 6) {
    std::random_device rd;
    gen.seed(rd());
}

int PlayerRandom::move(Position &pos) {
    for (int i = 0; i < 1000; i++) {
        int col = dis(gen);

        if (pos.is_move_valid(col)) {
            return col;
        }
    }

    std::cout << "Random player could not find a valid move in this position:" << std::endl;
    pos.printb();

    return 0;
}
