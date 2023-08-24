#include <cassert>
#include <iostream>

#include "position.h"
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


static board min(board a, board b) {
    return (a < b) ? a : b;
}

static board find_threats_in_direction(board b, int dir) {
    board doubles = b & (b << dir);
    board triples = doubles & (doubles << dir);
    
    return ((b >> dir) & (doubles << dir))
        | ((b << dir) & (doubles >> 2 * dir))
        | (triples << dir)
        | (triples >> 3 * dir);
}


static board find_threats(board b) {
    return find_threats_in_direction(b, 1)
        | find_threats_in_direction(b, BOARD_HEIGHT)
        | find_threats_in_direction(b, BOARD_HEIGHT_1)
        | find_threats_in_direction(b, BOARD_HEIGHT_2);
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


// Returns a 1 in any cell which is part of a 4 in a row.
static board find_winning_stones(board b) {
    return find_winning_stones_in_direction(b, 1)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT_1)
        | find_winning_stones_in_direction(b, BOARD_HEIGHT_2);
}


static board has_won_in_direction(board b, int dir) {
    board pairs = b & (b << 2 * dir);
    
    return pairs & (pairs << dir);
}


static board has_won(board b) {
    return has_won_in_direction(b, 1)
        | has_won_in_direction(b, BOARD_HEIGHT)
        | has_won_in_direction(b, BOARD_HEIGHT_1)
        | has_won_in_direction(b, BOARD_HEIGHT_2);
}


// Public functions.

board Position::move(int col) {
    assert(is_board_valid());
    assert(is_move_valid(col));
    assert(0 <= col && col < BOARD_WIDTH);

    board valid_moves = (b0 | b1) + BOTTOM_ROW;
    board mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    board before_move = b0;
    board after_move = b0 | (valid_moves & mask);

    b0 = b1;
    b1 = after_move;

    ply++;

    assert(is_board_valid());

    return before_move;
}


board Position::move(board mask) {
    assert(is_board_valid());
    assert(!(mask & (mask - 1)));
    assert(!(mask & b0));
    assert(!(mask & b1));
    assert(mask & ((b0 | b1) + BOTTOM_ROW));

    board before_move = b0;

    b0 = b1;
    b1 = before_move | mask;
    
    ply++;

    assert(is_board_valid());

    return before_move;
}


void Position::unmove(board before_move) {
    assert(is_board_valid());
    
    b1 = b0;
    b0 = before_move;

    ply--;

    assert(is_board_valid());
}


int Position::num_moves() {
    return ply;
}


bool Position::has_player_won() {
    return has_won(b0) != 0;
}


bool Position::has_opponent_won() {
    return has_won(b1) != 0;
}


bool Position::is_draw() {
    assert(!has_player_won());
    assert(!has_opponent_won());

    return (b0 | b1) == VALID_CELLS;
}


bool Position::can_player_win() {
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    return has_won(b0 | empty_positions);
}


bool Position::can_opponent_win() {
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    return has_won(b1 | empty_positions);
}


board Position::find_player_threats() {
    assert(!has_player_won());
    assert(!has_opponent_won());
    assert(!is_draw());

    // Exclude any threats which the opponent already blocked.
    return find_threats(b0) & ~b1 & VALID_CELLS;
}


board Position::find_opponent_threats() {
    assert(!has_player_won());
    assert(!has_opponent_won());
    assert(!is_draw());

    // Exclude any threats which the opponent already blocked.
    return find_threats(b1) & ~b0 & VALID_CELLS;
}


board Position::find_odd_even_threats(board threats) {
    if (ply & 1) {
        return 0;
    } else {
        return (threats & BOTTOM_ROW)
            | (threats & (BOTTOM_ROW << 2))
            | (threats & (BOTTOM_ROW << 4));
    }
}


board Position::wins_this_move(board threats) {
    board next_valid_moves = (b0 | b1) + BOTTOM_ROW;

    // Exclude any threat which cannot be played immediately.
    return threats & next_valid_moves;
}


board Position::find_non_losing_moves(board opponent_threats) {
    board below_threats = opponent_threats >> 1;
    board valid_moves = (b0 | b1) + BOTTOM_ROW;

    return valid_moves & ~below_threats & VALID_CELLS;
}


bool Position::is_move_valid(int col) {
    assert(0 <= col && col < BOARD_WIDTH);

    board moves_played = b0 | b1;
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    return (moves_played & move_mask) != move_mask;
}


bool Position::is_non_losing_move(board non_losing_moves, int col) {
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    return is_move_valid(col) && (move_mask & non_losing_moves);
}


board Position::hash(bool &is_mirrored) {
    // Find any stones which cannot impact the rest of the game and assume
    // player 0 played these stones. This prevents these stones from
    // influencing the hash.
    board dead_stones = find_dead_stones();

    // The hash is a 1 on all positions played by player 0, and a 1 on top
    // of each column. This hash uniquely identifies the state.
    board column_headers = (b0 | b1 | dead_stones) + BOTTOM_ROW;
    board hash = b0 | dead_stones | column_headers;
    
    // Return the same hash for mirrored states.
    board mirrored = mirror(hash);
    is_mirrored = mirrored < hash;
    if (is_mirrored) {
        return mirrored;
    }

    return hash;
}


void Position::printb() {
    print_mask(b0, b1);
}


void Position::print_mask(board a, board b) {
    std::cout << "+";
    for (int x = 0; x < BOARD_WIDTH; x++) {
        std::cout << "-";
    }
    std::cout << "+" << std::endl;

    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        std::cout << "|";
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int shift = y + x * BOARD_HEIGHT_1;
    
            if ((a >> shift) & 1) {
                std::cout << "O";
            } else if ((b >> shift) & 1) {
                std::cout << "X";
            } else {
                std::cout << ".";
            }
        }

        std::cout << "|" << std::endl;
    }

    std::cout << "+";
    for (int x = 0; x < BOARD_WIDTH; x++) {
        std::cout << "-";
    }
    std::cout << "+" << std::endl;
}


bool Position::are_dead_stones_valid() {
    board dead_stones = find_dead_stones();
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    board b0_wins = find_winning_stones(b0 | empty_positions) & empty_positions;
    board b1_wins = find_winning_stones(b1 | empty_positions) & empty_positions;
    
    board b0_wins_minus_dead_stones = find_winning_stones((b0 & ~dead_stones) | empty_positions) & empty_positions;
    board b1_wins_minus_dead_stones = find_winning_stones((b1 & ~dead_stones) | empty_positions) & empty_positions;
            
    board b0_wins_plus_dead_stones = find_winning_stones(b0 | dead_stones | empty_positions) & empty_positions;
    board b1_wins_plus_dead_stones = find_winning_stones(b1 | dead_stones | empty_positions) & empty_positions;

    // All dead stones must pass the following conditions:
    //    1. Flipping the dead stone to the player's color cannot allow the player more possible wins.
    //    2. Flipping the dead stone to the opponent's color cannot take any possible wins away from the player.
    return b0_wins == b0_wins_minus_dead_stones // Condition #1 for player #1.
        && b1_wins == b1_wins_minus_dead_stones // Condition #1 for player #2.
        && b0_wins == b0_wins_plus_dead_stones  // Condition #2 for player #1.
        && b1_wins == b1_wins_plus_dead_stones; // Condition #2 for player #2.
}


// Private functions


board Position::find_dead_stones() {
    return dead_stones_in_direction(b0, b1, 1)
        & dead_stones_in_direction(b0, b1, BOARD_HEIGHT)
        & dead_stones_in_direction(b0, b1, BOARD_HEIGHT_1)
        & dead_stones_in_direction(b0, b1, BOARD_HEIGHT_2);
}


board Position::mirror(board b) {
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


bool Position::is_board_valid() {
    return !(b0 & ~VALID_CELLS)
        && !(b1 & ~VALID_CELLS)
        && !(b0 & b1);
}
