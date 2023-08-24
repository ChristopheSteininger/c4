#include <assert.h>
#include <stdio.h>
#include <locale.h>

#include "settings.h"
#include "position.h"
#include "solver.h"


int main() {
    assert(BOARD_WIDTH * BOARD_HEIGHT_1 < 8 * sizeof(board));
    
    // Allow thousands separator.
    setlocale(LC_NUMERIC, "");

    Position pos = Position();

    Solver solver = Solver();
    solver.solve_verbose(pos);
}
