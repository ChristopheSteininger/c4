#include <assert.h>
#include <stdio.h>

#include "board.h"
#include "settings.h"


const int BOARD_HEIGHT_1 = BOARD_HEIGHT + 1;
static const int BOARD_HEIGHT_2 = BOARD_HEIGHT + 2;

static const board COLUMN_MASK = ((board) 1 << BOARD_HEIGHT_1) - 1;

const board BOTTOM_ROW = (((board) 1 << (BOARD_HEIGHT_1 * BOARD_WIDTH)) - 1)
    / ((1 << BOARD_HEIGHT_1) - 1);

// 1 in each column header.
static const board TOP_ROW = BOTTOM_ROW << BOARD_HEIGHT;


board move(board player, board opponent, int column) {
    assert(is_board_valid(player));
    assert(is_board_valid(opponent));
    assert(is_move_valid(player, opponent, column));
    
    board valid_moves = (player | opponent) + BOTTOM_ROW;
    board mask = COLUMN_MASK << (BOARD_HEIGHT_1 * column);
    
    return player | (valid_moves & mask);
}


int has_won(board b) {
    assert(is_board_valid(b));
    
    board vertical_2_in_row = b & (b << 2);
    if ((vertical_2_in_row & (vertical_2_in_row << 1)) != 0) {
        return 1;
    }

    board horiztonal_2_in_row = b & (b << BOARD_HEIGHT_1 * 2);
    if ((horiztonal_2_in_row & (horiztonal_2_in_row << BOARD_HEIGHT_1)) != 0) {
        return 1;
    }
    
    board positive_diagonal_2_in_row = b & (b << (BOARD_HEIGHT_2 * 2));
    if ((positive_diagonal_2_in_row & (positive_diagonal_2_in_row << BOARD_HEIGHT_2)) != 0) {
        return 1;
    }
    
    board negative_dialonal_2_in_row = b & (b << (BOARD_HEIGHT * 2));
    return (negative_dialonal_2_in_row & (negative_dialonal_2_in_row << BOARD_HEIGHT)) != 0;
}


int is_draw(board b0, board b1) {
    board valid_moves = (b0 | b1) + BOTTOM_ROW;
    
    return valid_moves == TOP_ROW;
}


int is_move_valid(board b0, board b1, int column) {
    assert(is_board_valid(b0));
    assert(is_board_valid(b1));
    
    board next_moves = (b0 | b1) + BOTTOM_ROW;
    board column_mask = COLUMN_MASK << (BOARD_HEIGHT_1 * column);
    
    return (next_moves & column_mask & TOP_ROW) == 0;
}


int is_board_valid(board b) {
    return (b & TOP_ROW) == 0;
}


int has_piece_on(board b, int x, int y) {
    assert(is_board_valid(b));

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
}
