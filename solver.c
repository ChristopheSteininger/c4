#include <stdio.h>
#include <assert.h>
#include <time.h>

#include "solver.h"
#include "settings.h"
#include "board.h"
#include "table.h"


unsigned long stat_num_nodes = 0;


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
    stat_num_nodes++;

    if (is_draw(player, opponent)) {
        return 0;
    }

    // The player can win on this move if the current player has any active threats.
    if (find_threats(player, opponent) != 0) {
        return 1;
    }

    // If the opponent has any active threats, then the player must block one.
    board threats = find_threats(opponent, player);
    if (threats != 0) {
        for (int col = 0; col < BOARD_WIDTH; col++) {
            // If this column contains a threat, play that column.
            if (threats & (COLUMN_MASK << col * BOARD_HEIGHT_1)) {
                board child_state = move(player, opponent, col);
                return -negamax(opponent, child_state, -beta, -alpha);
            }
        }

        // If there is a threat, then it must be found in the loop above.
        assert(0);
    }

    int original_alpha = alpha;

    // Check if this state has already been seen.
    int lookup_type, lookup_value;
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
    int value = -1;
    
    for (int x = 0; x < BOARD_WIDTH && alpha < beta; x++) {
        int col = BOARD_WIDTH/2 + x/2 - x * (x & 1);
        if (is_move_valid(player, opponent, col)) {
            board child_state = move(player, opponent, col);
            int child_score = -negamax(opponent, child_state, -beta, -alpha);
            
            value = max(value, child_score);
            alpha = max(value, alpha);
        }
    }

    // Store the result in the transposition table.
    table_store(player, opponent, get_node_type(value, original_alpha, beta), value);

    return value;
}


int solve(board b0, board b1) {
    stat_num_nodes = 0;
    
    return negamax(b0, b1, -1, 1);
}


int solve_verbose(board b0, board b1) {
    int allocate_successful = allocate_table();
    if (!allocate_successful) {
        printf("Failed to allocate memory for transposition table");
        return -1;
    }
    
    printf("Solving:\n");
    printb(b0, b1);

    unsigned long start_time = clock();
    int score = solve(b0, b1);
    double run_time_ms = (clock() - start_time) * 1000 / (double) CLOCKS_PER_SEC;

    printf("\n");
    printf("Score is %d\n", score);

    printf("\n");
    printf("Nodes seen           = %'lu\n", stat_num_nodes);
    printf("Nodes per ms         = %'.0f\n", stat_num_nodes / run_time_ms);
    printf("Table size           = %.2f GB\n", get_table_size_in_gigabytes());
    printf("Table hit rate       = %6.2f%%\n", get_table_hit_rate() * 100);
    printf("Table collision rate = %6.2f%%\n", get_table_collision_rate() * 100);
    printf("Table density        = %6.2f%%\n", get_table_density() * 100);
    printf("Table overwrite rate = %6.2f%%\n", get_table_overwrite_rate() * 100);

    free_table();

    return score;
}


unsigned long get_num_nodes() {
    return stat_num_nodes;
}
