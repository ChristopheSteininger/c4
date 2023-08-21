#include <assert.h>

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


static void insert(int *moves, float *scores, int count, int move, float score) {
    int pos = count;
    for (; pos > 0 && score > scores[pos - 1]; pos--) {
        moves[pos] = moves[pos - 1];
        scores[pos] = scores[pos - 1];
    }

    moves[pos] = move;
    scores[pos] = score;
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

    return num_threats + 0.5 * num_odd_even_threats;
}


int order_moves(Position &pos, int *moves, board non_losing_moves, int table_move) {
    assert(table_move == BOARD_WIDTH || pos.is_move_valid(table_move));

    float scores[BOARD_WIDTH];
    for (int col = 0; col < BOARD_WIDTH; col++) {
        scores[col] = -1000;
    }

    int num_moves = 0;

    for (int x = 0; x < BOARD_WIDTH; x++) {
        int col = BOARD_WIDTH/2 + x/2 - x * (x & 1);
        if (pos.is_non_losing_move(non_losing_moves, col)) {
            float score = calc_score(pos, col, table_move);

            insert(moves, scores, num_moves, col, score);
            num_moves++;
        }
    }

    assert(num_moves > 0);

    return num_moves;
}
