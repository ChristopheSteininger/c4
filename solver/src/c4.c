#include <assert.h>
#include <stdio.h>
#include <locale.h>

#include "settings.h"
#include "board.h"
#include "solver.h"
#include "table.h"


int main() {
    assert(BOARD_WIDTH * BOARD_HEIGHT_1 < 8 * sizeof(board));
    
    // Allow thousands separator.
    setlocale(LC_NUMERIC, "");
    
    board b0 = 0;
    board b1 = 0;

    if (!allocate_table()) {
        printf("Failed to allocate memory for transposition table");
        return -1;
    }
    
    printf("Using a %d x %d board and %.2f GB table.\n",
        BOARD_WIDTH, BOARD_HEIGHT, get_table_size_in_gigabytes());

    solve_verbose(b0, b1);

    free_table();
}
