#include <assert.h>
#include <stdio.h>

#include "board.h"


const int BOARD_WIDTH = 7;
const int BOARD_HEIGHT = 6;


int main() {
    assert(BOARD_WIDTH * (BOARD_HEIGHT + 1) < 8 * sizeof(board));
    
    board b0 = 0;
    board b1 = 0;

    b0 = move(b0, b1, 4);
    b1 = move(b1, b0, 4);

    printb(b0, b1);
}
