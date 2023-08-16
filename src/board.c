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

// 1 in each invalid cell.
static const board INVALID_CELLS = ~VALID_CELLS;


// Helper methods.

static board find_threats_in_direction(board b, int dir) {
    board doubles = b & (b << dir);
    board triples = doubles & (doubles << dir);
    
    return ((b >> dir) & (doubles << dir))
        | ((b << dir) & (doubles >> 2 * dir))
        | (triples << dir)
        | (triples >> 3 * dir);
}


static board too_short(int dir) {
    board pairs = (VALID_CELLS >> dir) & VALID_CELLS;
    board triples = (pairs >> dir) & VALID_CELLS;
    board quads = (triples >> dir) & VALID_CELLS;

    board quads_shifted = quads | (quads << dir);
    board possible_wins = quads_shifted | (quads_shifted << 2 * dir);
    
    return VALID_CELLS & ~possible_wins;
}


static board border_stones_in_direction(int dir) {
    board stones_right_of_border = (VALID_CELLS << dir) & VALID_CELLS;
    board stones_left_of_border = (VALID_CELLS >> dir) & VALID_CELLS;

    board center_stones = stones_right_of_border & stones_left_of_border;

    return ~center_stones;
}


static board dead_stones_in_direction(board b0, board b1, int dir) {   
    board played_positions = b0 | b1;
    board empty_positions = VALID_CELLS & ~played_positions;

    // . = empty
    // | = edge of the board
    // O = player 0
    // X = player 1
    // # = player 0/player 1
    // _ = empty/player 0/player 1
    // ^ = position of the 1s in the mask
    
    // Os and Xs can be swapped in all patterns.

    // Detect the patterns #. and .#
    //                     ^       ^
    board uncovered
        = ((empty_positions >> dir) & played_positions)
        | ((empty_positions << dir) & played_positions);
    
    // Detect the patterns ##. and .##
    //                     ^         ^
    board covered_by_1
        = ((uncovered >> dir) & played_positions)
        | ((uncovered << dir) & played_positions);

    // Detect the patterns #XX. and .XX#
    //                     ^           ^
    board pairs = ((b0 >> dir) & b0) | ((b1 >> dir) & b1);
    board covered_by_pair
        = ((covered_by_1 >> dir) & (pairs >> dir))
        | ((covered_by_1 << dir) & (pairs << 2 * dir));
    
    // Use the previous patterns to find all stones covered by
    // enough other stones that we know these are dead stones.
    board covered_stones = played_positions
        & ~uncovered
        & ~covered_by_1
        & ~covered_by_pair;
    
    // Detect the patterns |___|, |__|, and |_|
    //                      ^^^    ^^        ^
    // These patterns occur at the corners of the board when
    // checking the diagonals. All stones in these positions
    // are dead.
    board excluded_stones = too_short(dir);

    // Detect the patterns O_X and X_O
    //                      ^       ^
    board between
        = ((b0 >> dir) & (b1 << dir))
        | ((b1 >> dir) & (b0 << dir));

    // Detect the patterns |#X_O and O_X#|
    //                      ^           ^
    board pinned
        = border_stones_in_direction(dir)
        & played_positions
        & ((between >> 2 * dir) | (between << 2 * dir));

    return covered_stones | excluded_stones | pinned;
}


static board find_winning_stones_in_direction(board b, int dir) {
    board pairs = b & (b << 2 * dir);
    board quads = pairs & (pairs << dir);

    board winning_pairs = quads | (quads >> dir);

    return winning_pairs | (winning_pairs >> 2 * dir);
}


static board has_won_in_direction(board b, int dir) {
    board pairs = b & (b << 2 * dir);
    
    return pairs & (pairs << dir);
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


int has_won(board b) {
    assert(is_board_valid(b));

    board quads
        = has_won_in_direction(b, 1)
        | has_won_in_direction(b, BOARD_HEIGHT)
        | has_won_in_direction(b, BOARD_HEIGHT_1)
        | has_won_in_direction(b, BOARD_HEIGHT_2);
    
    return quads != 0;
}


int is_draw(board b0, board b1) {
    assert(!has_won(b0));
    assert(!has_won(b1));
    
    board played_positions = b0 | b1;
    
    return played_positions == VALID_CELLS;
}


board find_threats(board player, board opponent) {
    assert(!has_won(player));
    assert(!has_won(opponent));
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


board find_opportunities(board player, board opponent) {
    assert(!has_won(player));
    assert(!has_won(opponent));
    assert(!is_draw(player, opponent));

    // Find any threat, including ones blocked by the opponent.
    board all_threats = find_threats_in_direction(player, 1)
        | find_threats_in_direction(player, BOARD_HEIGHT)
        | find_threats_in_direction(player, BOARD_HEIGHT_1)
        | find_threats_in_direction(player, BOARD_HEIGHT_2);

    board future_threats = all_threats & ~opponent & VALID_CELLS;
    return future_threats;
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
    return dead_stones_in_direction(b0, b1, 1)
        & dead_stones_in_direction(b0, b1, BOARD_HEIGHT)
        & dead_stones_in_direction(b0, b1, BOARD_HEIGHT_1)
        & dead_stones_in_direction(b0, b1, BOARD_HEIGHT_2);
}


board find_winning_stones(board b) {
    return find_winning_stones_in_direction(b, 1)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT_1)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT_2);
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
