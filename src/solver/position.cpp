#include "position.h"

#include <cassert>
#include <iomanip>
#include <iostream>

#include "Tracy.hpp"
#include "settings.h"

// Represents a single direction in which a player can win.
enum class Direction {
    VERTICAL = 1,
    HORIZONTAL = BOARD_HEIGHT + 1,

    // From top left to bottom right.
    NEGATIVE_DIAGONAL = BOARD_HEIGHT,

    // From bottom left to top right.
    POSITIVE_DIAGONAL = BOARD_HEIGHT + 2
};

static constexpr board set_ones(int n) {
    // If n is equal to the number of bits in board, then 1 << n would overflow,
    // so handle seperately here.
    if (n == 8 * sizeof(board)) {
        return ~((board)0);
    }

    return ((board)1 << n) - 1;
}

static constexpr int BOARD_HEIGHT_1 = BOARD_HEIGHT + 1;

// 1 at each playable position of the first column.
static constexpr board FIRST_COLUMN = set_ones(BOARD_HEIGHT);

// 1 at the playable position of the first column, plus the first column header.
static constexpr board FIRST_COLUMN_1 = set_ones(BOARD_HEIGHT_1);

// 1 at the bottom of each column.
static constexpr board BOTTOM_ROW = set_ones(BOARD_HEIGHT_1 * BOARD_WIDTH) / set_ones(BOARD_HEIGHT_1);

// 1 in each column header.
static constexpr board COLUMN_HEADERS = BOTTOM_ROW << BOARD_HEIGHT;

// 1 on each playable posiition.
static constexpr board VALID_CELLS = COLUMN_HEADERS - BOTTOM_ROW;

// 1 on each stone next to an edge in each possible direction.
static constexpr board border_stones_in_direction(const Direction dir) {
    int shift = static_cast<int>(dir);

    board stones_right_of_border = (VALID_CELLS << shift) & VALID_CELLS;
    board stones_left_of_border = (VALID_CELLS >> shift) & VALID_CELLS;

    board center_stones = stones_right_of_border & stones_left_of_border;

    return ~center_stones;
}
static constexpr board BORDER_VERTICAL = border_stones_in_direction(Direction::VERTICAL);
static constexpr board BORDER_HORIZONTAL = border_stones_in_direction(Direction::HORIZONTAL);
static constexpr board BORDER_NEGATIVE_DIAGONAL = border_stones_in_direction(Direction::NEGATIVE_DIAGONAL);
static constexpr board BORDER_POSITIVE_DIAGONAL = border_stones_in_direction(Direction::POSITIVE_DIAGONAL);

// These patterns occur at the corners of the board when checking the diagonals.
// All stones in these positions are dead.
static constexpr board too_short(const Direction dir) {
    int shift = static_cast<int>(dir);

    board pairs = (VALID_CELLS >> shift) & VALID_CELLS;
    board triples = (pairs >> shift) & VALID_CELLS;
    board quads = (triples >> shift) & VALID_CELLS;

    board quads_shifted = quads | (quads << shift);
    board possible_wins = quads_shifted | (quads_shifted << 2 * shift);

    return VALID_CELLS & ~possible_wins;
}
static constexpr board TOO_SHORT_NEGATIVE_DIAGONAL = too_short(Direction::NEGATIVE_DIAGONAL);
static constexpr board TOO_SHORT_POSITIVE_DIAGONAL = too_short(Direction::POSITIVE_DIAGONAL);

// Helper methods.

static board find_threats_in_direction(const board b, const Direction dir) {
    int shift = static_cast<int>(dir);

    board doubles = b & (b << shift);
    board triples = doubles & (doubles << shift);

    return ((b >> shift) & (doubles << shift)) | ((b << shift) & (doubles >> 2 * shift)) | (triples << shift) |
           (triples >> 3 * shift);
}

static board find_threats(const board b) {
    return find_threats_in_direction(b, Direction::VERTICAL) | find_threats_in_direction(b, Direction::HORIZONTAL) |
           find_threats_in_direction(b, Direction::NEGATIVE_DIAGONAL) |
           find_threats_in_direction(b, Direction::POSITIVE_DIAGONAL);
}

static board dead_stones_in_direction(const board b0, const board b1, const Direction dir, const board border) {
    ZoneScoped;

    int shift = static_cast<int>(dir);

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
    board uncovered = ((empty_positions >> shift) & played_positions)
        | ((empty_positions << shift) & played_positions);

    // Detect the patterns ##. and .##
    //                     ^         ^
    board covered_by_1 = ((uncovered >> shift) & played_positions)
        | ((uncovered << shift) & played_positions);

    // Detect the patterns #XX. and .XX#
    //                     ^           ^
    board pairs = ((b0 >> shift) & b0) | ((b1 >> shift) & b1);
    board covered_by_pair = ((covered_by_1 >> shift) & (pairs >> shift))
        | ((covered_by_1 << shift) & (pairs << 2 * shift));

    // Use the previous patterns to find all stones covered by
    // enough other stones that we know these are dead stones.
    board covered_stones = played_positions & ~uncovered & ~covered_by_1 & ~covered_by_pair;

    // Detect the patterns O_X and X_O
    //                      ^       ^
    board between = ((b0 >> shift) & (b1 << shift)) | ((b1 >> shift) & (b0 << shift));

    // Detect the patterns |#X_O and O_X#|
    //                      ^           ^
    board pinned = border & played_positions & ((between >> 2 * shift) | (between << 2 * shift));

    return covered_stones | pinned;
}

static board find_winning_stones_in_direction(const board b, const Direction dir) {
    int shift = static_cast<int>(dir);

    board pairs = b & (b << 2 * shift);
    board quads = pairs & (pairs << shift);

    board winning_pairs = quads | (quads >> shift);

    return winning_pairs | (winning_pairs >> 2 * shift);
}

// Returns a 1 in any cell which is part of a 4 in a row.
static board find_winning_stones(const board b) {
    return find_winning_stones_in_direction(b, Direction::VERTICAL) |
           find_winning_stones_in_direction(b, Direction::HORIZONTAL) |
           find_winning_stones_in_direction(b, Direction::NEGATIVE_DIAGONAL) |
           find_winning_stones_in_direction(b, Direction::POSITIVE_DIAGONAL);
}

static board has_won_in_direction(const board b, const Direction dir) {
    int shift = static_cast<int>(dir);

    board pairs = b & (b << 2 * shift);
    return pairs & (pairs << shift);
}

static board has_won(const board b) {
    return has_won_in_direction(b, Direction::VERTICAL)
        | has_won_in_direction(b, Direction::HORIZONTAL)
        | has_won_in_direction(b, Direction::NEGATIVE_DIAGONAL)
        | has_won_in_direction(b, Direction::POSITIVE_DIAGONAL);
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

bool Position::has_player_won() const { return has_won(b0) != 0; }

bool Position::has_opponent_won() const { return has_won(b1) != 0; }

bool Position::is_draw() const {
    assert(!has_player_won());
    assert(!has_opponent_won());

    return (b0 | b1) == VALID_CELLS;
}

bool Position::is_game_over() const { return has_player_won() || has_opponent_won() || is_draw(); }

bool Position::can_player_win() const {
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    return has_won(b0 | empty_positions);
}

bool Position::can_opponent_win() const {
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    return has_won(b1 | empty_positions);
}

board Position::find_player_threats() const {
    assert(!has_player_won());
    assert(!has_opponent_won());
    assert(!is_draw());

    // Exclude any threats which the opponent already blocked.
    return find_threats(b0) & ~b1 & VALID_CELLS;
}

board Position::find_opponent_threats() const {
    assert(!has_player_won());
    assert(!has_opponent_won());
    assert(!is_draw());

    // Exclude any threats which the opponent already blocked.
    return find_threats(b1) & ~b0 & VALID_CELLS;
}

board Position::find_next_turn_threats(board threats) const {
    board valid_moves = ((b0 | b1) + BOTTOM_ROW) & VALID_CELLS;
    board next_valid_moves = valid_moves << 1;

    return threats & next_valid_moves;
}

board Position::find_next_next_turn_threats(board threats) const {
    board valid_moves = ((b0 | b1) + BOTTOM_ROW) & VALID_CELLS;
    board next_valid_moves = (valid_moves << 1) & VALID_CELLS;
    board next_next_valid_moves = next_valid_moves << 1;

    return threats & next_next_valid_moves;
}

board Position::wins_this_move(board threats) const {
    board next_valid_moves = (b0 | b1) + BOTTOM_ROW;

    // Exclude any threat which cannot be played immediately.
    return threats & next_valid_moves;
}

board Position::find_non_losing_moves(board opponent_threats) const {
    board below_threats = opponent_threats >> 1;
    board valid_moves = (b0 | b1) + BOTTOM_ROW;

    return valid_moves & ~below_threats & VALID_CELLS;
}

bool Position::is_move_valid(int col) const {
    assert(0 <= col && col < BOARD_WIDTH);

    board moves_played = b0 | b1;
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    return (moves_played & move_mask) != move_mask;
}

bool Position::is_non_losing_move(board non_losing_moves, int col) const {
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    return is_move_valid(col) && (move_mask & non_losing_moves);
}

int Position::moves_left(int score) const {
    // Run the calculation from the perspective of the first player.
    if (ply & 1) {
        score *= -1;
    }

    int max_moves = BOARD_WIDTH * BOARD_HEIGHT;

    int last_move;
    if (score > 0) {
        last_move = max_moves - 2 * score + 1 + (max_moves % 2);
    } else if (score < 0) {
        last_move = max_moves + 2 * (score + 1) - (max_moves % 2);
    } else {
        last_move = max_moves;
    }

    return last_move - ply;
}

board Position::hash(bool &is_mirrored) const {
    ZoneScoped;

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

void Position::printb() const { print_mask(b0, b1); }

void Position::print_mask(board a, board b) const {
    // Allow colors to be switched off if not displaying correctly.
#ifdef NO_COLOR_OUTPUT
    const char *p0 = " O";
    const char *p1 = " X";
#else
    const char *p0 = " \x1B[31mO\033[0m";
    const char *p1 = " \x1B[33mX\033[0m";
#endif

    // Print the board.
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int shift = y + x * BOARD_HEIGHT_1;

            if ((a >> shift) & 1) {
                std::cout << p0;
            } else if ((b >> shift) & 1) {
                std::cout << p1;
            } else {
                std::cout << " .";
            }
        }

        std::cout << std::endl;
    }

    // Print the column numbers.
    for (int x = 0; x < BOARD_WIDTH; x++) {
        std::cout << std::setw(2) << std::setfill(' ') << x;
    }
    std::cout << std::endl;
}

bool Position::are_dead_stones_valid() const {
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
    return b0_wins == b0_wins_minus_dead_stones     // Condition #1 for player #1.
           && b1_wins == b1_wins_minus_dead_stones  // Condition #1 for player #2.
           && b0_wins == b0_wins_plus_dead_stones   // Condition #2 for player #1.
           && b1_wins == b1_wins_plus_dead_stones;  // Condition #2 for player #2.
}

// Private functions

board Position::find_dead_stones() const {
    board vertical = dead_stones_in_direction(b0, b1, Direction::VERTICAL, BORDER_VERTICAL);
    board horizontal = dead_stones_in_direction(b0, b1, Direction::HORIZONTAL, BORDER_HORIZONTAL);
    board pos_diag = dead_stones_in_direction(b0, b1, Direction::POSITIVE_DIAGONAL, BORDER_POSITIVE_DIAGONAL)
        | TOO_SHORT_POSITIVE_DIAGONAL;
    board neg_diag = dead_stones_in_direction(b0, b1, Direction::NEGATIVE_DIAGONAL, BORDER_NEGATIVE_DIAGONAL)
        | TOO_SHORT_NEGATIVE_DIAGONAL;

    return vertical & horizontal & pos_diag & neg_diag;
}

board Position::mirror(board b) const {
    ZoneScoped;

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

bool Position::is_board_valid() const { return !(b0 & ~VALID_CELLS) && !(b1 & ~VALID_CELLS) && !(b0 & b1); }
