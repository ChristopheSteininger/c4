#include <cassert>
#include <algorithm>

#include "Tracy.hpp"

#include "order.h"
#include "settings.h"
#include "position.h"


static int count_bits(board b) {
    int result;
    for (result = 0; b; result++) {
        b &= b - 1;
    }

    return result;
}


static void rotate_moves(int *moves, int num_moves, int offset, bool has_table_move) {
    int *first_move = moves;
    int num_rotate_moves = num_moves;

    // Don't rotate table moves.
    if (has_table_move) {
        first_move++;
        num_rotate_moves--;
    }

    if (num_rotate_moves > 1) {
        std::rotate(
            first_move,
            first_move + (offset % num_rotate_moves),
            first_move + num_rotate_moves);
    }
}


static float calc_score(Position &pos, int col, int table_move) {
    if (col == table_move) {
        return 1000;
    }

    board before_move = pos.move(col);
    board threats = pos.find_opponent_threats();
    pos.unmove(before_move);

    int num_threats = count_bits(threats);
    int num_odd_even_threats = count_bits(pos.find_odd_even_threats(threats));
    float center_score = (float) std::min(col, BOARD_WIDTH - col - 1) / BOARD_WIDTH;

    return num_threats
        + 0.5 * num_odd_even_threats
        + 0.1 * center_score;
}


int order_moves(Position &pos, int *moves, board non_losing_moves, int table_move, int offset) {
    ZoneScoped;

    assert(table_move == -1 || pos.is_move_valid(table_move));

    float scores[BOARD_WIDTH];
    int num_moves = 0;

    // Calculate the score of each valid move.
    for (int col = 0; col < BOARD_WIDTH; col++) {
        if (pos.is_non_losing_move(non_losing_moves, col)) {
            moves[num_moves] = col;
            scores[col] = calc_score(pos, col, table_move);

            num_moves++;
        }
    }

    assert(num_moves > 0);

    // Sort moves according to score, high to low.
    std::sort(moves, moves + num_moves,
       [&scores](size_t a, size_t b) {return scores[a] > scores[b];});

    // Rotate any non table moves to help threads desync.
    if (offset != 0) {
        rotate_moves(moves, num_moves, offset, table_move != -1);
    }

    return num_moves;
}
