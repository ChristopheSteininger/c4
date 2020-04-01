#include "board.h"
#include "hashing.h"


board hash_state(board b0, board b1) {
    board column_headers = (b0 | b1) + BOTTOM_ROW;
    
    // // Return the same hash for states where the players' stones are swapped.
    // if (b0 < b1) {
    //     return b0 | column_headers;
    // }

    return b1 | column_headers;
}
