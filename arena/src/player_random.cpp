#include <iostream>

#include "player_random.h"

extern "C" {
    #include "../../solver/src/board.h"
}


PlayerRandom::PlayerRandom()
        : dis(0, 6)  {
    std::random_device rd;
    gen.seed(rd());
}


int PlayerRandom::move(board player, board opponent) {
    for (int i = 0; i < 1000; i++) {
        int col = dis(gen);

        if (is_move_valid(player, opponent, col)) {
            return col;
        }
    }
    
    std::cout << "Random player could not find a valid move in this position:" << std::endl;
    printb(player, opponent);

    return 0;
}
