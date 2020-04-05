#include "settings.h"
#include "board.h"
#include "hashing.h"


board hash_state(board b0, board b1) {
    board column_headers = (b0 | b1) + BOTTOM_ROW;

    board hash = b0 | column_headers;

    board mirrored_hash = 0;
    for (int col = 0; col <= (BOARD_WIDTH - 1) / 2; col++) {
        int shift = (BOARD_WIDTH - 2 * col - 1) * BOARD_HEIGHT_1;
        
        board left_mask = COLUMN_MASK << (col * BOARD_HEIGHT_1);
        board right_mask = COLUMN_MASK << ((BOARD_WIDTH - col - 1) * BOARD_HEIGHT_1);
        
        mirrored_hash |= (hash & left_mask) << shift;
        mirrored_hash |= (hash & right_mask) >> shift;
    }

    if (mirrored_hash < hash) {
        return mirrored_hash;
    }
    
    return hash;
}
