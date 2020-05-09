#include "settings.h"
#include "board.h"
#include "hashing.h"


static board min(board a, board b) {
    if (a < b) {
        return a;
    }

    return b;
}


board hash_state(board b0, board b1) {
    // Find any stones which cannot impact the rest of the game and assume
    // player 0 played these stones.
    board dead_stones = find_dead_stones(b0, b1);

    // The hash is a 1 on all positions played by player 0, and a 1 on top
    // of each column. This hash uniquely identifies the state.
    board column_headers = (b0 | b1 | dead_stones) + BOTTOM_ROW;
    board hash = b0 | column_headers;
    
    // Return the same hash for mirrored states.
    return min(hash, mirror(hash));
}