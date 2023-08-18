#include <assert.h>

#include "order.h"
#include "settings.h"
#include "board.h"


static int count_bits(board b) {
    int result;
    for (result = 0; b; result++) {
        b &= b - 1;
    }

    return result;
}


static void insert(int *moves, int *scores, int count, int move, int score) {
    int pos = count;
    for (; pos > 0 && score > scores[pos - 1]; pos--) {
        moves[pos] = moves[pos - 1];
        scores[pos] = scores[pos - 1];
    }

    moves[pos] = move;
    scores[pos] = score;
}


static int calc_score(board player, board opponent, int col, int best_move) {
    if (col == best_move) {
        return 1000;
    }

    board after_move = move(player, opponent, col);
    return count_bits(find_opportunities(after_move, opponent));
}


int order_moves(board player, board opponent, int *moves, int best_move) {
    assert(best_move == BOARD_WIDTH || is_move_valid(player, opponent, best_move));

    int scores[BOARD_WIDTH];

    for (int col = 0; col < BOARD_WIDTH; col++) {
        scores[col] = -1000;
    }

    int num_moves = 0;

    for (int x = 0; x < BOARD_WIDTH; x++) {
        int col = BOARD_WIDTH/2 + x/2 - x * (x & 1);
        if (is_move_valid(player, opponent, col)) {
            int score = calc_score(player, opponent, col, best_move);

            insert(moves, scores, num_moves, col, score);
            num_moves++;
        }
    }

    return num_moves;
}
