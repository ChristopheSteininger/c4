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
    
    // Find any vertical wins.
    board v_doubles = b & (b << 2);
    if ((v_doubles & (v_doubles << 1)) != 0) {
        return 1;
    }

    // Find any horizontal wins.
    board h_doubles = b & (b << 2 * BOARD_HEIGHT_1);
    if ((h_doubles & (h_doubles << BOARD_HEIGHT_1)) != 0) {
        return 1;
    }
    
    // Find any wins along the positive diagonal.
    board pd_doubles = b & (b << 2 * BOARD_HEIGHT_2);
    if ((pd_doubles & (pd_doubles << BOARD_HEIGHT_2)) != 0) {
        return 1;
    }
    
    // Find any wins along the negative diagonal.
    board nd_doubles = b & (b << 2 * BOARD_HEIGHT);
    return (nd_doubles & (nd_doubles << BOARD_HEIGHT)) != 0;
}


int is_draw(board b0, board b1) {
    board valid_moves = (b0 | b1) + BOTTOM_ROW;
    
    return valid_moves == TOP_ROW;
}


board find_threats(board b0, board b1) {
    assert(!has_won(b0));
    assert(!has_won(b1));

    // Find any vertical threats.
    board v_doubles = b0 & (b0 << 1);
    board v_triples = v_doubles & (v_doubles << 1);
    board v_threats = v_triples << 1;

    // Find any horizontal threats.
    board h_doubles = b0 & (b0 << BOARD_HEIGHT_1);
    board h_triples = h_doubles & (h_doubles << BOARD_HEIGHT_1);
    board h_threats
        = ((b0 >> BOARD_HEIGHT_1) & (h_doubles << BOARD_HEIGHT_1))
        | ((b0 << BOARD_HEIGHT_1) & (h_doubles >> 2 * BOARD_HEIGHT_1))
        | (h_triples << BOARD_HEIGHT_1)
        | (h_triples >> 3 * BOARD_HEIGHT_1);

    // Find any positive diagonal threats.
    board pd_doubles = b0 & (b0 << BOARD_HEIGHT_2);
    board pd_triples = pd_doubles & (pd_doubles << BOARD_HEIGHT_2);
    board pd_threats
        = ((b0 >> BOARD_HEIGHT_2) & (pd_doubles << BOARD_HEIGHT_2))
        | ((b0 << BOARD_HEIGHT_2) & (pd_doubles >> 2 * BOARD_HEIGHT_2))
        | (pd_triples << BOARD_HEIGHT_2)
        | (pd_triples >> 3 * BOARD_HEIGHT_2);
    
    // Find any negative diagonal threats.
    board nd_doubles = b0 & (b0 << BOARD_HEIGHT);
    board nd_triples = nd_doubles & (nd_doubles << BOARD_HEIGHT);
    board nd_threats
        = ((b0 >> BOARD_HEIGHT) & (nd_doubles << BOARD_HEIGHT))
        | ((b0 << BOARD_HEIGHT) & (nd_doubles >> 2 * BOARD_HEIGHT))
        | (nd_triples << BOARD_HEIGHT)
        | (nd_triples >> 3 * BOARD_HEIGHT);

    // Include any threats.
    board all_threats = v_threats | h_threats | pd_threats | nd_threats;

    // Exclude any threats which cannot be played immediately.
    board next_valid_moves = ((b0 | b1) + BOTTOM_ROW) & ~TOP_ROW;
    return all_threats & next_valid_moves;
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
