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


static void insert(board *moves, int *scores, int count, board move, int score) {
    int pos = count;
    for (; pos > 0 && score > scores[pos - 1]; pos--) {
        moves[pos] = moves[pos - 1];
        scores[pos] = scores[pos - 1];
    }

    moves[pos] = move;
    scores[pos] = score;
}


static int calc_score(board player, board opponent) {
    return count_bits(find_opportunities(player, opponent));
}


int order_moves(board player, board opponent, board *moves) {
    int scores[BOARD_WIDTH];

    for (int col = 0; col < BOARD_WIDTH; col++) {
        moves[col] = 0;
        scores[col] = -1000;
    }

    int num_moves = 0;

    for (int x = 0; x < BOARD_WIDTH; x++) {
        int col = BOARD_WIDTH/2 + x/2 - x * (x & 1);
        if (is_move_valid(player, opponent, col)) {
            board child_state = move(player, opponent, col);
            int score = calc_score(child_state, opponent);

            insert(moves, scores, num_moves, child_state, score);
            num_moves++;
        }
    }

    return num_moves;
}
