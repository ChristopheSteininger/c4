#include <stdio.h>

#include "solver.h"
#include "settings.h"
#include "board.h"
#include "table.h"


// Scores greater than this are never seen in evaluation.
const int INFINITY = 10;

long nodes_seen = 0;


int max(int a, int b) {
    if (a > b) {
        return a;
    }

    return b;
}

int min(int a, int b) {
    if (a < b) {
        return a;
    }

    return b;
}

int get_node_type(int value, int alpha, int beta) {
    if (value <= alpha) {
        return TYPE_UPPER_BOUND;
    }

    if (value >= beta) {
        return TYPE_LOWER_BOUND;
    }

    return TYPE_EXACT;
}

int negamax(board player, board opponent, int alpha, int beta) {
    nodes_seen++;
    
    // Return immediately if this is a terminal state.
    if (has_won(opponent)) {
        return -1;
    }

    if (is_draw(player, opponent)) {
        return 0;
    }

    int original_alpha = alpha;

    // Check if this state has already been seen.
    int lookup_type;
    int lookup_value;
    int lookup_success = table_lookup(player, opponent, &lookup_type, &lookup_value);

    if (lookup_success) {
        if (lookup_type == TYPE_EXACT) {
            return lookup_value;
        }
        
        else if (lookup_type == TYPE_LOWER_BOUND) {
            alpha = max(alpha, lookup_value);
        }
        
        else if (lookup_type == TYPE_UPPER_BOUND) {
            beta = min(beta, lookup_value);
        }

        if (alpha >= beta) {
            return lookup_value;
        }
    }

    // Evaluate all child states.
    int value = -INFINITY;
    
    for (int col = 0; col < BOARD_WIDTH && alpha < beta; col++) {
        if (is_move_valid(player, opponent, col)) {
            board child_state = move(player, opponent, col);
            int child_score = -negamax(opponent, child_state, -beta, -alpha);
            
            value = max(value, child_score);
            alpha = max(alpha, value);
        }
    }

    // Store the result in the transposition table.
    table_store(player, opponent, get_node_type(value, original_alpha, beta), value);

    return value;
}

int solve(board b0, board b1) {
    int allocate_successful = allocate_table();
    if (!allocate_successful) {
        printf("Failed to allocate memory for transposition table");
        return -1;
    }
    
    printf("Solving:\n");
    printb(b0, b1);

    int score = negamax(b0, b1, -INFINITY, INFINITY);

    printf("\n");
    printf("Score = %d\n", score);
    printf("States seen = %ld\n", nodes_seen);

    free_table();

    return score;
}
