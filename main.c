#include <assert.h>
#include <stdio.h>
#include <locale.h>

#include "settings.h"
#include "board.h"
#include "solver.h"


int main() {
    assert(BOARD_WIDTH * BOARD_HEIGHT_1 < 8 * sizeof(board));
    
    // Allow thousands separator.
    setlocale(LC_NUMERIC, "");
    
    board b0 = 0;
    board b1 = 0;

    printf("Using a %d x %d board.\n", BOARD_WIDTH, BOARD_HEIGHT);

    solve_verbose(b0, b1);
}
