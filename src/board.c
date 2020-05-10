#include <assert.h>
#include <stdio.h>

#include "board.h"
#include "settings.h"


const int BOARD_HEIGHT_1 = BOARD_HEIGHT + 1;
static const int BOARD_HEIGHT_2 = BOARD_HEIGHT + 2;

const board FIRST_COLUMN = ((board) 1 << BOARD_HEIGHT) - 1;

const board FIRST_COLUMN_1 = ((board) 1 << BOARD_HEIGHT_1) - 1;

const board BOTTOM_ROW = (((board) 1 << (BOARD_HEIGHT_1 * BOARD_WIDTH)) - 1)
    / ((1 << BOARD_HEIGHT_1) - 1);

// 1 in each column header.
static const board COLUMN_HEADERS = BOTTOM_ROW << BOARD_HEIGHT;

// 1 in each valid cell.
const board VALID_CELLS = COLUMN_HEADERS - BOTTOM_ROW;


// Helper methods.

static board find_threats_in_direction(board b, int dir) {
    board doubles = b & (b << dir);
    board triples = doubles & (doubles << dir);
    
    return ((b >> dir) & (doubles << dir))
        | ((b << dir) & (doubles >> 2 * dir))
        | (triples << dir)
        | (triples >> 3 * dir);
}


static board covered_stones_in_direction(board b0, board b1, int dir) {
    board played_positions = b0 | b1;
    board empty_positions = VALID_CELLS & ~played_positions;

    board stones_1_below_empty = (empty_positions >> dir) & played_positions;
    board stones_1_above_empty = (empty_positions << dir) & played_positions;
    
    board stones_2_below_empty = stones_1_below_empty >> dir;
    board stones_2_above_empty = stones_1_above_empty << dir;

    board pairs = ((b0 >> dir) & b0) | ((b1 >> dir) & b1);
    board below_pair = (empty_positions >> 3 * dir) & (pairs >> dir);
    board above_pair = (empty_positions << 3 * dir) & (pairs << 2 * dir);
    
    return played_positions
        & ~stones_1_above_empty
        & ~stones_1_below_empty
        & ~stones_2_above_empty
        & ~stones_2_below_empty
        & ~above_pair
        & ~below_pair;
}


static board find_winning_stones_in_direction(board b, int dir) {
    board doubles = b & (b << 2 * dir);
    
    return doubles & (doubles << dir);
}


// Library methods.

board move(board player, board opponent, int column) {
    assert(is_board_valid(player));
    assert(is_board_valid(opponent));
    assert(is_move_valid(player, opponent, column));
    
    board valid_moves = (player | opponent) + BOTTOM_ROW;
    board mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * column);
    
    return player | (valid_moves & mask);
}


board find_winning_stones(board b) {
    assert(is_board_valid(b));
    
    return find_winning_stones_in_direction(b, 1)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT_1)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT_2);
}


int is_draw(board b0, board b1) {
    assert(!find_winning_stones(b0));
    assert(!find_winning_stones(b1));
    
    board played_positions = b0 | b1;
    
    return played_positions == VALID_CELLS;
}


board find_threats(board player, board opponent) {
    assert(!find_winning_stones(player));
    assert(!find_winning_stones(opponent));
    assert(!is_draw(player, opponent));

    // Find any threat, including ones blocked by the opponent.
    board all_threats = find_threats_in_direction(player, 1)
        | find_threats_in_direction(player, BOARD_HEIGHT)
        | find_threats_in_direction(player, BOARD_HEIGHT_1)
        | find_threats_in_direction(player, BOARD_HEIGHT_2);

    // Exclude any threats which cannot be played immediately.
    board next_valid_moves = ((player | opponent) + BOTTOM_ROW) & ~COLUMN_HEADERS;
    return all_threats & next_valid_moves;
}


int is_move_valid(board b0, board b1, int column) {
    assert(is_board_valid(b0));
    assert(is_board_valid(b1));
    
    board moves_played = b0 | b1;
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * column);
    
    return (moves_played & move_mask) != move_mask;
}


int get_num_valid_moves(board b0, board b1) {
    assert(is_board_valid(b0));
    assert(is_board_valid(b1));

    board next_moves = (b0 | b1) + BOTTOM_ROW;
    board valid_moves = next_moves & ~COLUMN_HEADERS;

    int num_valid_moves = 0;
    for (int col = 0; col < BOARD_WIDTH; col++) {
        board mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);
        if (valid_moves & mask) {
            num_valid_moves++;
        }
    }

    return num_valid_moves;
}


int is_board_valid(board b) {
    return !(b & ~VALID_CELLS);
}


board mirror(board b) {
    board mirror = 0;
    
    for (int col = 0; col <= (BOARD_WIDTH - 1) / 2; col++) {
        int shift = (BOARD_WIDTH - 2 * col - 1) * BOARD_HEIGHT_1;
        
        board left_mask = FIRST_COLUMN_1 << (col * BOARD_HEIGHT_1);
        board right_mask = FIRST_COLUMN_1 << ((BOARD_WIDTH - col - 1) * BOARD_HEIGHT_1);
        
        mirror |= (b & left_mask) << shift;
        mirror |= (b & right_mask) >> shift;
    }

    return mirror;
}


board find_dead_stones(board b0, board b1) {
    return covered_stones_in_direction(b0, b1, 1)
        & covered_stones_in_direction(b0, b1, BOARD_HEIGHT)
        & covered_stones_in_direction(b0, b1, BOARD_HEIGHT_1)
        & covered_stones_in_direction(b0, b1, BOARD_HEIGHT_2);
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
                printf(".");
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
