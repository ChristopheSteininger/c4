#include "board.h"

#include <assert.h>
#include <stdio.h>

#include "settings.h"


static const board BOARD_HEIGHT_1 = BOARD_HEIGHT + 1;

static const board COLUMN_MASK = ((board) 1 << BOARD_HEIGHT_1) - 1;

// 1 at the bottom of each column.
static const board BOTTOM_ROW = (((board) 1 << (BOARD_HEIGHT_1 * BOARD_WIDTH)) - 1)
    / ((1 << BOARD_HEIGHT_1) - 1);

// 1 in each column header.
static const board TOP_ROW = BOTTOM_ROW << BOARD_HEIGHT; 


board move(board player, board opponent, int column) {
    assert(is_move_valid(player, opponent, column));
    
    board valid_moves = (player | opponent) + BOTTOM_ROW;
    board mask = COLUMN_MASK << (BOARD_HEIGHT_1 * column);
    
    return player | (valid_moves & mask);
}


int has_won(board b) {
    board vertical_win = b & (b << 1) & (b << 2) & (b << 3);

    board horizontal_win = b
        & (b << BOARD_HEIGHT_1)
        & (b << BOARD_HEIGHT_1 * 2)
        & (b << BOARD_HEIGHT_1 * 3);
    
    board positive_diagonal_win = b
        & (b << (BOARD_HEIGHT_1 + 1))
        & (b << (BOARD_HEIGHT_1 * 2 + 2))
        & (b << (BOARD_HEIGHT_1 * 3 + 3));
    
    board negative_diagonal_win = b
        & (b << (BOARD_HEIGHT_1 - 1))
        & (b << (BOARD_HEIGHT_1 * 2 - 2))
        & (b << (BOARD_HEIGHT_1 * 3 - 3));

    return (vertical_win | horizontal_win | positive_diagonal_win | negative_diagonal_win) != 0;
}


int is_move_valid(board b0, board b1, int column) {
    board next_moves = (b0 | b1) + BOTTOM_ROW;
    board column_mask = COLUMN_MASK << (BOARD_HEIGHT_1 * column);
    
    return (next_moves & column_mask & TOP_ROW) == 0;
}


int has_piece_on(board b, int x, int y) {
    int shift_to_cell = y + x * BOARD_HEIGHT_1;
    return (b >> shift_to_cell) & 1;
}


void printb(board b0, board b1) {
    printf("+");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("-");
    }
    printf("+\n");

    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        printf("|");
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (has_piece_on(b0, x, y)) {
                printf("O");
            } else if (has_piece_on(b1, x, y)) {
                printf("X");
            } else {
                printf(" ");
            }
        }

        printf("|\n");
    }

    printf("+");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("-");
    }
    printf("+\n");
    
    printf(" ");
    for (int x = 0; x < BOARD_WIDTH; x++) {
        printf("%d", x);
    }
    printf("\n");
}
