#include "position.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <sstream>

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

// 1 at each column header.
static constexpr board COLUMN_HEADERS = BOTTOM_ROW << BOARD_HEIGHT;

// 1 at each playable posiition.
static constexpr board VALID_CELLS = COLUMN_HEADERS - BOTTOM_ROW;

// 1 at each odd row cell in the first column.
static constexpr board ODD_FIRST_COLUMN = set_ones(BOARD_HEIGHT + (BOARD_HEIGHT % 2)) / 3;

// 1 at each odd cell.
static constexpr board ODD_CELLS = ODD_FIRST_COLUMN * BOTTOM_ROW;

// 1 at each even cell.
static constexpr board EVEN_CELLS = ODD_CELLS << 1;

// 1 on each stone next to an edge in each possible direction.
static constexpr board border_stones_in_direction(const Direction dir) {
    int shift = static_cast<int>(dir);

    board stones_right_of_border = (VALID_CELLS << shift) & VALID_CELLS;
    board stones_left_of_border = (VALID_CELLS >> shift) & VALID_CELLS;

    board center_stones = stones_right_of_border & stones_left_of_border;

    return ~center_stones;
}

// These patterns occur at the corners of the board when checking the diagonals.
// All stones in these positions are dead.
static constexpr board too_short_in_direction(const Direction dir) {
    int shift = static_cast<int>(dir);

    board pairs = (VALID_CELLS >> shift) & VALID_CELLS;
    board triples = (pairs >> shift) & VALID_CELLS;
    board quads = (triples >> shift) & VALID_CELLS;

    board quads_shifted = quads | (quads << shift);
    board possible_wins = quads_shifted | (quads_shifted << 2 * shift);

    return VALID_CELLS & ~possible_wins;
}

// Helper methods.

template <Direction dir>
static board find_threats_in_direction(const board b) noexcept {
    constexpr int shift = static_cast<int>(dir);

    board doubles = b & (b << shift);
    board triples = doubles & (doubles << shift);

    if constexpr (dir == Direction::VERTICAL) {
        return triples << 1;
    } else {
        return ((b >> shift) & (doubles << shift))
            | ((b << shift) & (doubles >> 2 * shift))
            | (triples << shift)
            | (triples >> 3 * shift);
    }
}

static board find_threats(const board b) noexcept {
    return find_threats_in_direction<Direction::VERTICAL>(b)
         | find_threats_in_direction<Direction::HORIZONTAL>(b)
         | find_threats_in_direction<Direction::NEGATIVE_DIAGONAL>(b)
         | find_threats_in_direction<Direction::POSITIVE_DIAGONAL>(b);
}

template <Direction dir>
static board dead_stones_in_direction(const board b0, const board b1) noexcept {
    constexpr int shift = static_cast<int>(dir);

    constexpr board border = border_stones_in_direction(dir);
    constexpr board too_short = too_short_in_direction(dir);

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

    return covered_stones | pinned | too_short;
}

template<Direction dir>
static board find_winning_stones_in_direction(const board b) noexcept {
    constexpr int shift = static_cast<int>(dir);

    board pairs = b & (b << 2 * shift);
    board quads = pairs & (pairs << shift);

    board winning_pairs = quads | (quads >> shift);

    return winning_pairs | (winning_pairs >> 2 * shift);
}

// Returns a 1 in any cell which is part of a 4 in a row.
static board find_winning_stones(const board b) noexcept {
    return find_winning_stones_in_direction<Direction::VERTICAL>(b)
         | find_winning_stones_in_direction<Direction::HORIZONTAL>(b)
         | find_winning_stones_in_direction<Direction::NEGATIVE_DIAGONAL>(b)
         | find_winning_stones_in_direction<Direction::POSITIVE_DIAGONAL>(b);
}

template<Direction dir>
static board has_won_in_direction(const board b) noexcept {
    constexpr int shift = static_cast<int>(dir);

    board pairs = b & (b << 2 * shift);
    return pairs & (pairs << shift);
}

static bool has_won(const board b) noexcept {
    return (has_won_in_direction<Direction::VERTICAL>(b) != 0)
        || (has_won_in_direction<Direction::HORIZONTAL>(b) != 0)
        || (has_won_in_direction<Direction::NEGATIVE_DIAGONAL>(b) != 0)
        || (has_won_in_direction<Direction::POSITIVE_DIAGONAL>(b) != 0);
}

// Public functions.

board Position::move(int col) noexcept {
    assert(is_board_valid());
    assert(is_move_valid(col));
    assert(0 <= col && col < BOARD_WIDTH);

    board valid_moves = (b0 | b1) + BOTTOM_ROW;
    board mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    board before_move = b0;

    b0 = b1;
    b1 = before_move | (valid_moves & mask);

    moves_played++;

    assert(is_board_valid());

    return before_move;
}

board Position::move(board mask) noexcept {
    assert(is_board_valid());
    assert(!(mask & (mask - 1)));
    assert(!(mask & b0));
    assert(!(mask & b1));
    assert(mask & ((b0 | b1) + BOTTOM_ROW));

    board before_move = b0;

    b0 = b1;
    b1 = before_move | mask;

    moves_played++;

    assert(is_board_valid());

    return before_move;
}

void Position::unmove(board before_move) noexcept {
    assert(is_board_valid());

    b1 = b0;
    b0 = before_move;

    moves_played--;

    assert(is_board_valid());
}

bool Position::has_player_won() const noexcept { return has_won(b0); }

bool Position::has_opponent_won() const noexcept { return has_won(b1); }

bool Position::is_draw() const noexcept { return (b0 | b1) == VALID_CELLS; }

bool Position::is_game_over() const noexcept { return has_player_won() || has_opponent_won() || is_draw(); }

bool Position::can_player_win() const noexcept {
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    return has_won(b0 | empty_positions);
}

bool Position::can_opponent_win() const noexcept {
    board empty_positions = VALID_CELLS & ~(b0 | b1);

    return has_won(b1 | empty_positions);
}

board Position::find_player_threats() const noexcept {
    assert(!has_player_won());
    assert(!has_opponent_won());
    assert(!is_draw());

    // Exclude any threats which the opponent already blocked.
    return find_threats(b0) & ~b1 & VALID_CELLS;
}

board Position::find_opponent_threats() const noexcept {
    assert(!has_player_won());
    assert(!has_opponent_won());
    assert(!is_draw());

    // Exclude any threats which the opponent already blocked.
    return find_threats(b1) & ~b0 & VALID_CELLS;
}

board Position::find_next_turn_threats(board threats) const noexcept {
    board valid_moves = ((b0 | b1) + BOTTOM_ROW) & VALID_CELLS;
    board next_valid_moves = valid_moves << 1;

    return threats & next_valid_moves;
}

board Position::find_next_next_turn_threats(board threats) const noexcept {
    board valid_moves = ((b0 | b1) + BOTTOM_ROW) & VALID_CELLS;
    board next_valid_moves = (valid_moves << 1) & VALID_CELLS;
    board next_next_valid_moves = next_valid_moves << 1;

    return threats & next_next_valid_moves;
}

board Position::wins_this_move(board threats) const noexcept {
    board next_valid_moves = (b0 | b1) + BOTTOM_ROW;

    // Exclude any threat which cannot be played immediately.
    return threats & next_valid_moves;
}

board Position::find_non_losing_moves(board opponent_threats) const noexcept {
    board below_threats = opponent_threats >> 1;
    board valid_moves = (b0 | b1) + BOTTOM_ROW;

    return valid_moves & ~below_threats & VALID_CELLS;
}

bool Position::is_forced_loss_next_turn(board opponent_wins, board non_losing_moves) const noexcept {
    // If the player can only move below the opponents threats, the player will lose.
    if (non_losing_moves == 0) {
        return true;
    }

    // If the opponent has no threats next move, then the player cannot lose next turn.
    if (opponent_wins == 0) {
        return false;
    }

    // Otherwise the game is lost if and only if:
    //  * The opponent has multiple threats
    //  * Or, the opponent has two threats on top of each other
    return (opponent_wins & (opponent_wins - 1)) || !(opponent_wins & non_losing_moves);
}

board Position::find_forced_move(board opponent_wins, board non_losing_moves) const noexcept {
    assert(!is_forced_loss_next_turn(opponent_wins, non_losing_moves));

    // A move is forced if the opponent could win next turn.
    if (opponent_wins) {
        assert((opponent_wins & (opponent_wins - 1)) == 0);
        assert((opponent_wins & non_losing_moves) == opponent_wins);

        return opponent_wins;
    }

    // A move is also forced if the player has only one move which does not lose immediately.
    if ((non_losing_moves & (non_losing_moves - 1)) == 0) {
        return non_losing_moves;
    }

    return 0;
}

int Position::upper_bound_from_evens_strategy() const noexcept {
    assert((moves_played & 1) == 0);
    assert((BOARD_HEIGHT & 1) == 0);

    board valid_moves = (b0 | b1) + BOTTOM_ROW;

    // Assume the opponent takes all remaining even cells which are not valid moves this
    // turn, and assume the current player takes all other cells.
    board opponent_evens = b1 | (EVEN_CELLS & ~b0 & ~valid_moves);
    board player_odds = VALID_CELLS & ~opponent_evens;

    if (has_won_in_direction<Direction::VERTICAL>(player_odds) != 0) {
        return MAX_SCORE;
    }

    board opponent_hori = has_won_in_direction<Direction::HORIZONTAL>(opponent_evens);
    board opponent_neg_diag = has_won_in_direction<Direction::NEGATIVE_DIAGONAL>(opponent_evens);
    board opponent_pos_diag = has_won_in_direction<Direction::POSITIVE_DIAGONAL>(opponent_evens);

    board player_hori = has_won_in_direction<Direction::HORIZONTAL>(player_odds);
    board player_neg_diag = has_won_in_direction<Direction::NEGATIVE_DIAGONAL>(player_odds);
    board player_pos_diag = has_won_in_direction<Direction::POSITIVE_DIAGONAL>(player_odds);

    constexpr board BELOW_COLUMNS = COLUMN_HEADERS + 1;

    // If the current player could win horizontally below the opponent's
    // horizontal even threat, then the evens strategy will not work.
    if (player_hori & (opponent_hori - BELOW_COLUMNS)) {
        return MAX_SCORE;
    }

    // Repeat the same check for the two diagonal directions.
    if (player_neg_diag & (opponent_neg_diag - BELOW_COLUMNS)) {
        return MAX_SCORE;
    }

    if (player_pos_diag & (opponent_pos_diag - BELOW_COLUMNS)) {
        return MAX_SCORE;
    }

    return (opponent_hori | opponent_neg_diag | opponent_pos_diag) ? -1 : 0;
}

bool Position::is_move_valid(int col) const noexcept {
    assert(0 <= col && col < BOARD_WIDTH);

    board moves = b0 | b1;
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    return (moves & move_mask) != move_mask;
}

bool Position::is_non_losing_move(board non_losing_moves, int col) const noexcept {
    board move_mask = FIRST_COLUMN << (BOARD_HEIGHT_1 * col);

    return is_move_valid(col) && (move_mask & non_losing_moves);
}

int Position::score_win(int moves_until_win) const noexcept {
    // A player can never win on the opponent's move, so number of moves must be odd.
    assert((moves_until_win & 1) == 1);

    return score_win_at(moves_played + moves_until_win);
}

int Position::score_loss(int moves_until_loss) const noexcept {
    // A player can never lose on their own move, so number of moves must be even.
    assert((moves_until_loss & 1) == 0);

    return -score_win_at(moves_played + moves_until_loss);
}

int Position::moves_left(int score) const noexcept {
    // Run the calculation from the perspective of the first player.
    if (moves_played & 1) {
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

    return last_move - moves_played;
}

board Position::hash(bool &is_mirrored) const noexcept {
    // Find any stones which cannot impact the rest of the game and assume
    // player 0 played these stones. This prevents these stones from
    // influencing the hash.
    board dead_stones = find_dead_stones();

    // The hash is a 1 on all positions played by player 0, and a 1 on top
    // of each column. This hash uniquely identifies the state.
    board column_headers = (b0 | b1) + BOTTOM_ROW;
    board hash = b0 | dead_stones | column_headers;

    // Return the same hash for mirrored states.
    board mirrored = mirror(hash);
    is_mirrored = mirrored < hash;
    if (is_mirrored) {
        return mirrored;
    }

    return hash;
}

void Position::print() const noexcept { std::cout << display_board(); }

void Position::print_mask(board a, board b) const noexcept { std::cout << display_mask(a, b);  }

std::string Position::display_board() const noexcept {
    if ((moves_played & 1) == 0) {
        return display_mask(b0, b1);
    } else {
        return display_mask(b1, b0);
    }
}

std::string Position::display_mask(board a, board b) const noexcept {
    std::stringstream result;

    // Print the board.
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int shift = y + x * BOARD_HEIGHT_1;

            if ((a >> shift) & 1) {
                result << " " << P0_STONE;
            } else if ((b >> shift) & 1) {
                result << " " << P1_STONE;
            } else {
                result << " .";
            }
        }

        result << std::endl;
    }

    // Print the column numbers.
    for (int x = 0; x < BOARD_WIDTH; x++) {
        result << std::setw(2) << std::setfill(' ') << x;
    }
    result << std::endl;

    return result.str();
}

bool Position::are_dead_stones_valid() const noexcept {
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

board Position::find_dead_stones() const noexcept {
    board vertical = dead_stones_in_direction<Direction::VERTICAL>(b0, b1);
    if (vertical == 0) {
        return 0;
    }

    board horizontal = dead_stones_in_direction<Direction::HORIZONTAL>(b0, b1);
    if (horizontal == 0) {
        return 0;
    }

    board pos_diag = dead_stones_in_direction<Direction::POSITIVE_DIAGONAL>(b0, b1);
    if (pos_diag == 0) {
        return 0;
    }

    board neg_diag = dead_stones_in_direction<Direction::NEGATIVE_DIAGONAL>(b0, b1);

    return vertical & horizontal & pos_diag & neg_diag;
}

board Position::mirror(board b) const noexcept {
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

bool Position::is_board_valid() const noexcept { return !(b0 & ~VALID_CELLS) && !(b1 & ~VALID_CELLS) && !(b0 & b1); }
