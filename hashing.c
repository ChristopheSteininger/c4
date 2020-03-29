#include "board.h"
#include "hashing.h"


board hash_state(board b0, board b1) {
    board column_headers = (b0 | b1) + BOTTOM_ROW;
    
    return b0 | column_headers;
}
