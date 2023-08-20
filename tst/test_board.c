#include <stdio.h>

#include "minunit.h"
#include "../src/settings.h"
#include "../src/board.h"


void swap(board *b0, board *b1) {
    board tmp = *b0;
    *b0 = *b1;
    *b1 = tmp;
}


void play_random_move(board *b0, board *b1) {
    // Pick and play a random valid move.
    int col;
    do {
        col = rand() % BOARD_WIDTH;
    } while (!is_move_valid(*b0, *b1, col));

    *b0 = move(*b0, *b1, col);
}


char *test_has_piece_on_with_empty_board() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            mu_assert("board is empty.", !has_piece_on(0, x, y));
        }
    }

    return 0;
}


char *test_has_piece_on_with_full_board() {
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            mu_assert("board is full.", has_piece_on((board) 1 << (y + x * BOARD_HEIGHT_1), x, y));
        }
    }

    return 0;
}


char *test_has_piece_on_with_one_piece() {
    board b = (board) 1 << (BOARD_HEIGHT_1 * (BOARD_WIDTH - 1));

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (y == 0 && x == BOARD_WIDTH - 1) {
                mu_assert("board has only piece here.", has_piece_on(b, x, y));
            } else {
                mu_assert("board has only piece elsewhere.", !has_piece_on(b, x, y));
            }
        }
    }

    return 0;
}


char *test_move_on_empty_board() {
    for (int x = 0; x < BOARD_WIDTH; x++) {
        board expected = (board) 1 << (BOARD_HEIGHT_1 * x);
        mu_assert("placing piece on empty board.", move(0, 0, x) == expected);
    }

    return 0;
}


char *test_move_sequentially() {
    board b0 = 0;
    board b1 = 0;

    int col = BOARD_WIDTH / 2;
    board col_index = (board) 1 << (BOARD_HEIGHT_1 * col);

    for (int y = 0; y + 1 < BOARD_HEIGHT; y += 2) {
        board expected0 = b0 + (col_index << y);
        board expected1 = b1 + (col_index << (y + 1));

        b0 = move(b0, b1, col);
        mu_assert("player 0 move", b0 == expected0);

        b1 = move(b1, b0, col);
        mu_assert("player 1 move", b1 == expected1);
    }

    return 0;
}


char *test_has_won_with_vertical() {
    mu_assert("first column win", has_won(15));
    mu_assert("higher first column win", has_won(30));
    mu_assert("3 in a row on first column", !has_won(7));

    return 0;
}


char *test_has_won_with_horizontal() {
    mu_assert("first row win", has_won(1
        | (1 << BOARD_HEIGHT_1)
        | (1 << BOARD_HEIGHT_1 * 2)
        | (1 << BOARD_HEIGHT_1 * 3)));
    mu_assert("second row win", has_won(2
        | (2 << BOARD_HEIGHT_1)
        | (2 << BOARD_HEIGHT_1 * 2)
        | (2 << BOARD_HEIGHT_1 * 3)));
    mu_assert("3 in a row on first row", !has_won(1
        | (1 << BOARD_HEIGHT_1)
        | (1 << BOARD_HEIGHT_1  * 2)));
    
    return 0;
}


char *test_has_won_with_positive_diagonal() {
    // Test evaluation along / diagonal.
    mu_assert("first / diagonal win", has_won(1
        | ((board) 2 << BOARD_HEIGHT_1)
        | ((board) 4 << (BOARD_HEIGHT_1 * 2))
        | ((board) 8 << (BOARD_HEIGHT_1 * 3))));
    mu_assert("second / diagonal win", has_won(((board) 4 << BOARD_HEIGHT_1)
        | ((board) 8 << (BOARD_HEIGHT_1 * 2))
        | ((board) 16 << (BOARD_HEIGHT_1 * 3))
        | ((board) 32 << (BOARD_HEIGHT_1 * 4))));
    mu_assert("3 in a row on / diagonal", !has_won(1
        | ((board) 2 << (BOARD_HEIGHT_1))
        | ((board) 4 << (BOARD_HEIGHT_1 * 2))));
    
    return 0;
}


char *test_has_won_with_negative_diagonal() {
    // Test evaluation along \ diagonal.
    mu_assert("first \\ diagonal win", has_won(8
        | ((board) 4 << BOARD_HEIGHT_1)
        | ((board) 2 << (BOARD_HEIGHT_1 * 2))
        | ((board) 1 << (BOARD_HEIGHT_1 * 3))));
    mu_assert("second \\ diagonal win", has_won((board) 32 << (BOARD_HEIGHT_1 * 2)
        | ((board) 16 << (BOARD_HEIGHT_1 * 3))
        | ((board) 8 << (BOARD_HEIGHT_1 * 4))
        | ((board) 4 << (BOARD_HEIGHT_1 * 5))));
    mu_assert("3 in a row on \\ diagonal", !has_won((board) 32 << (BOARD_HEIGHT_1 * 2)
        | ((board) 16 << (BOARD_HEIGHT_1 * 3))
        | ((board) 8 << (BOARD_HEIGHT_1 * 4))));

    return 0;
}


char *test_is_draw_on_unfinished_games() {
    mu_assert("empty board is not a draw.", !is_draw(0, 0));
    mu_assert("board with several moves is not a draw.", !is_draw(1, (board) 1 << BOARD_HEIGHT_1));

    return 0;
}


char *test_is_draw_on_drawn_game() {
    board b0 = 0;
    board b1 = 0;

    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (((y + 2 * x) >> 1) & 1) {
                b0 = move(b0, b1, x);
            } else {
                b1 = move(b1, b0, x);
            }
        }
    }

    mu_assert("game is drawn, variant 1.", is_draw(b0, b1));
    mu_assert("game is drawn, variant 2.", is_draw(b1, b0));

    return 0;
}


char *test_find_threats_on_games_with_vertical_threat() {
    board b0, b1;

    // Test a vertical win in the first column.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 1);
    b0 = move(b0, b1, 0);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 0);

    mu_assert("Player 1 has a vertical threat in the first column",
        find_threats(b0, b1) == 8);
    mu_assert("Player 2 has no vertical threat.", find_threats(b1, b0) == 0);

    // Test a vertical win in the last column.
    b0 = move(0, 0, BOARD_WIDTH - 1);
    b1 = move(0, b0, BOARD_WIDTH - 2);
    b0 = move(b0, b1, BOARD_WIDTH - 1);
    b1 = move(b1, b0, BOARD_WIDTH - 2);
    b0 = move(b0, b1, BOARD_WIDTH - 1);

    mu_assert("Player 1 has a vertical threat in the last column",
        find_threats(b0, b1) == (board) 8 << (BOARD_WIDTH - 1) * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no vertical threat.", find_threats(b1, b0) == 0);

    // Test a vertical triple blocked by the top of the board.
    b0 = 0;
    b1 = 0;
    for (int y = 0; y < BOARD_HEIGHT - 4; y++) {
        if (y & 1) {
            b0 = move(b0, b1, 0);
        } else {
            b1 = move(b1, b0, 0);
        }
    }
    b1 = move(b1, b0, 0);
    b0 = move(b0, b1, 0);
    b0 = move(b0, b1, 0);
    b0 = move(b0, b1, 0);

    mu_assert("Player 1 has no vertical threat.", find_threats(b0, b1) == 0);
    mu_assert("Player 2 has no vertical threat.", find_threats(b1, b0) == 0);

    return 0;
}


char *test_find_threats_on_games_with_horizontal_threat() {
    board b0, b1;

    // Test a single horiztonal threat.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);

    mu_assert("Player 1 has a horizontal threat to the right.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 1 << 3 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a double horizontal threat.
    b0 = move(0, 0, 1);
    b1 = move(0, b0, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a double horizontal threat.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (1 | ((board) 1 << 4 * BOARD_HEIGHT_1)));
    mu_assert("Player 2 has no horizontal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a horiztonal threat blocked by the right edge of the board.
    b0 = move(0, 0, BOARD_WIDTH - 3);
    b1 = move(0, b0, BOARD_WIDTH - 3);
    b0 = move(b0, b1, BOARD_WIDTH - 2);
    b1 = move(b1, b0, BOARD_WIDTH - 2);
    b0 = move(b0, b1, BOARD_WIDTH - 1);
    b1 = move(b1, b0, BOARD_WIDTH - 1);

    mu_assert("Player 1 has a horizontal threat to the left.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 1 << (BOARD_WIDTH - 4) * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a horiztonal threat on the left middle.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 0);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a horizontal threat to the left middle.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 1 << BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a horiztonal threat on the right middle.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a horizontal threat to the left middle.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 1 << 2 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    return 0;
}


char *test_find_threats_on_games_with_positive_diagonal_threat() {
    board b0, b1;

    // Test a threat with the highest stone missing.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 1);
    b0 = move(b0, b1, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a positive diagonal threat for the highest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 8 << 3 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no positive diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat with the lowest stone missing.
    b1 = move(0, 0, 1);
    b0 = move(0, b1, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);

    mu_assert("Player 1 has a positive diagonal threat for the lowest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == 1);
    mu_assert("Player 2 has no positive diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat with the second lowest stone missing.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);

    mu_assert("Player 1 has a positive diagonal threat for the second lowest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 2 << BOARD_HEIGHT_1);
    mu_assert("Player 2 has no positive diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat with the second highest stone missing.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 1);
    b0 = move(b0, b1, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);
    b0 = move(b0, b1, 3);

    mu_assert("Player 1 has a positive diagonal threat for the second highest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 4 << 2 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no positive diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat blocked by the left edge of the board
    b1 = move(0, 0, 0);
    b0 = move(0, b1, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);

    mu_assert("Player 1 has no positive diagonal threat.", wins_this_move(b0, b1, find_threats(b0, b1)) == 0);
    mu_assert("Player 2 has no positive diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    return 0;
}


char *test_find_threats_on_games_with_negative_diagonal_threat() {
    board b0, b1;

    // Test a threat with the highest stone missing.
    b1 = move(0, 0, 0);
    b0 = move(0, b1, 0);
    b1 = move(b1, b0, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);
    b0 = move(b0, b1, 3);
    
    mu_assert("Player 1 has a negative diagonal threat for the highest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == 8);
    mu_assert("Player 2 has no negative diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat with the lowest stone missing.
    b1 = move(0, 0, 0);
    b0 = move(0, b1, 0);
    b1 = move(b1, b0, 0);
    b0 = move(b0, b1, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);

    mu_assert("Player 1 has a negative diagonal threat for the lowest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 1 << 3 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no negative diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat with the second lowest stone missing.
    b1 = move(0, 0, 0);
    b0 = move(0, b1, 0);
    b1 = move(b1, b0, 0);
    b0 = move(b0, b1, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 3);

    mu_assert("Player 1 has a negative diagonal threat for the second lowest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 2 << 2 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no negative diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat with the second highest stone missing.
    b1 = move(0, 0, 0);
    b0 = move(0, b1, 0);
    b1 = move(b1, b0, 0);
    b0 = move(b0, b1, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 2);
    b0 = move(b0, b1, 3);

    mu_assert("Player 1 has a negative diagonal threat for the second highest stone.",
        wins_this_move(b0, b1, find_threats(b0, b1)) == (board) 4 << BOARD_HEIGHT_1);
    mu_assert("Player 2 has no negative diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    // Test a threat blocked by the right edge of the board
    b1 = move(0, 0, BOARD_WIDTH - 3);
    b0 = move(0, b1, BOARD_WIDTH - 3);
    b1 = move(b1, b0, BOARD_WIDTH - 3);
    b0 = move(b0, b1, BOARD_WIDTH - 3);
    b0 = move(b0, b1, BOARD_WIDTH - 2);
    b1 = move(b1, b0, BOARD_WIDTH - 2);
    b0 = move(b0, b1, BOARD_WIDTH - 2);
    b1 = move(b1, b0, BOARD_WIDTH - 1);
    b0 = move(b0, b1, BOARD_WIDTH - 1);

    mu_assert("Player 1 has no negative diagonal threat.", wins_this_move(b0, b1, find_threats(b0, b1)) == 0);
    mu_assert("Player 2 has no negative diagonal threat.", wins_this_move(b1, b0, find_threats(b1, b0)) == 0);

    return 0;
}


char *test_is_move_valid() {
    board b = 0;
    
    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            mu_assert("valid move for player 0.", is_move_valid(b, 0, x));
            mu_assert("valid move for player 1.", is_move_valid(0, b, x));
            b = move(b, 0, x);
        }
        
        mu_assert("invalid move for player 0.", !is_move_valid(b, 0, x));
        mu_assert("invalid move for player 1.", !is_move_valid(0, b, x));
    }

    return 0;
}


char *test_get_num_valid_moves_on_known_state() {
    mu_assert("empty board, all moves valid", get_num_valid_moves(0, 0) == BOARD_WIDTH);

    mu_assert("first column full, one invalid move", get_num_valid_moves(FIRST_COLUMN, 0) == BOARD_WIDTH - 1);

    return 0;
}


char *test_get_num_valid_moves_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 1000000; trial++) {
        board b0 = 0;
        board b1 = 0;

        // Play random moves until the game is draw, or the last player won the game.
        while (1) {
            int actual = get_num_valid_moves(b0, b1);

            // Count the number of valid moves.
            int expected = 0;
            for (int col = 0; col < BOARD_WIDTH; col++) {
                if (is_move_valid(b0, b1, col)) {
                    expected++;
                }
            }

            // End the loop if the game is over.
            if (!has_won(b1) && !is_draw(b0, b1)) {
                break;
            }

            if (expected != actual) {
                printf("Expected %d valid moves, got %d.\n", expected, actual);
                printb(b0, b1);

                mu_fail("get_num_valid_moves did not return expected result in random game.");
            }
            
            play_random_move(&b0, &b1);
            swap(&b0, &b1);
        }
    }
    
    return 0;
}


char *test_is_board_valid_on_boards_with_invalid_column_headers() {
    for (int x = 0; x < BOARD_WIDTH - 1; x++) {
        mu_assert("invalid column header.", !is_board_valid(
            (board) 1 << (BOARD_HEIGHT + BOARD_HEIGHT_1 * x)));
    }

    return 0;
}


char *test_is_board_valid_on_boards_with_valid_board() {
    mu_assert("empty board.", is_board_valid(0));
    mu_assert("board with move in first row", is_board_valid((board) 1 << BOARD_HEIGHT_1));
    mu_assert("board with move in last row", is_board_valid((board) 1 << (BOARD_HEIGHT - 1)));

    return 0;
}


char *test_mirror_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 1000000; trial++) {
        board b0 = 0;
        board b1 = 0;

        board mirror_b0 = 0;
        board mirror_b1 = 0;
        
        // Play random moves until the game is draw, or the last player won the game.
        while (!has_won(b1) && !is_draw(b0, b1)) {
            // Pick and play a random valid move on both boards.
            int col;
            do {
                col = rand() % BOARD_WIDTH;
            } while (!is_move_valid(b0, b1, col));

            b0 = move(b0, b1, col);
            mirror_b0 = move(mirror_b0, mirror_b1, BOARD_WIDTH - col - 1);

            mu_assert("mirrored b0 not correct", mirror_b0 == mirror(b0));
            mu_assert("mirrored b1 not correct", mirror_b1 == mirror(b1));
            mu_assert("double mirrored b0 not correct", mirror(mirror_b0) == b0);
            mu_assert("double mirrored b1 not correct", mirror(mirror_b1) == b1);

            // Swap players.
            swap(&b0, &b1);
            swap(&mirror_b0, &mirror_b1);
        }
    }

    return 0;
}


char *test_find_dead_stones_with_single_cell() {
    board b0, b1;

    b0 = move(0, 0, 1);  mu_assert("Ply 0, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(0, b0, 1);  mu_assert("Ply 1, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 1);  mu_assert("Ply 2, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 2);  mu_assert("Ply 3, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 2);  mu_assert("Ply 4, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 3);  mu_assert("Ply 5, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 3);  mu_assert("Ply 6, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 3);  mu_assert("Ply 7, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 4);  mu_assert("Ply 8, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 4);  mu_assert("Ply 9, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 5);  mu_assert("Ply 10, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 5);  mu_assert("Ply 11, no dead cells", find_dead_stones(b0, b1) == 0);
    
    b1 = move(b1, b0, 5);
    mu_assert("Ply 12, single dead cell.", find_dead_stones(b0, b1) == (board) 1 << 3 * BOARD_HEIGHT_1);

    return 0;
}


char *test_find_dead_stones_recognises_stones_blocked_by_left_edge() {
    board b0 = 0;
    board b1 = 0;

    b0 = move(b0, b1, 0);  mu_assert("Ply 0, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 0);  mu_assert("Ply 1, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 0);  mu_assert("Ply 2, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 1);  mu_assert("Ply 3, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 1);  mu_assert("Ply 4, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 1);  mu_assert("Ply 5, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, 2);  mu_assert("Ply 6, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 2);  mu_assert("Ply 7, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 2);  mu_assert("Ply 8, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, 3); mu_assert("Ply 9, no dead cells", find_dead_stones(b0, b1) == 1);
    b1 = move(b1, b0, 3);  mu_assert("Ply 10, no dead cells", find_dead_stones(b0, b1) == 1);
    
    b0 = move(b0, b1, 3);
    mu_assert("Ply 11, no dead cells", find_dead_stones(b0, b1) == (1 | (board) 1 << BOARD_HEIGHT_1));
    
    return 0;
}


char *test_find_dead_stones_recognises_stones_blocked_by_right_edge() {
    board b0 = 0;
    board b1 = 0;

    b0 = move(b0, b1, BOARD_WIDTH - 1);  mu_assert("Ply 0, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, BOARD_WIDTH - 1);  mu_assert("Ply 1, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, BOARD_WIDTH - 1);  mu_assert("Ply 2, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, BOARD_WIDTH - 2);  mu_assert("Ply 3, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, BOARD_WIDTH - 2);  mu_assert("Ply 4, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, BOARD_WIDTH - 2);  mu_assert("Ply 5, no dead cells", find_dead_stones(b0, b1) == 0);
    b0 = move(b0, b1, BOARD_WIDTH - 3);  mu_assert("Ply 6, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, BOARD_WIDTH - 3);  mu_assert("Ply 7, no dead cells", find_dead_stones(b0, b1) == 0);
    b1 = move(b1, b0, BOARD_WIDTH - 3);  mu_assert("Ply 8, no dead cells", find_dead_stones(b0, b1) == 0);
    
    b1 = move(b1, b0, BOARD_WIDTH - 4);
    mu_assert("Ply 9, 1 dead cell", find_dead_stones(b0, b1)
        == (board) 1 << ((BOARD_WIDTH - 1) * BOARD_HEIGHT_1));
    
    b1 = move(b1, b0, BOARD_WIDTH - 4);
    mu_assert("Ply 10, 1 dead cell", find_dead_stones(b0, b1)
        == (board) 1 << ((BOARD_WIDTH - 1) * BOARD_HEIGHT_1));
    
    b0 = move(b0, b1, 3);
    mu_assert("Ply 11, 2 dead cells", find_dead_stones(b0, b1)
        == ((board) 1 << ((BOARD_WIDTH - 1) * BOARD_HEIGHT_1)
            | ((board) 1 << ((BOARD_WIDTH - 2) * BOARD_HEIGHT_1))));
    
    return 0;
}


char *test_find_dead_stones_on_drawn_board() {
    board b0 = 0;
    board b1 = 0;

    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            if ((x + y / 2) & 1) {
                b0 = move(b0, b1, x);
            } else {
                b1 = move(b1, b0, x);
            }
        }
    }

    mu_assert("Drawn board contains only dead cells", find_dead_stones(b0, b1) == (b0 | b1));

    return 0;
}


char *test_find_dead_stones_returns_subset_of_dead_stones_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 1000000; trial++) {
        board b0 = 0;
        board b1 = 0;
        
        // Play random moves until the game is draw, or the last player won the game.
        while (!has_won(b1) && !is_draw(b0, b1)) {
            // Assert that all dead stones returned have no impact on the future of the game.
            board dead_stones = find_dead_stones(b0, b1);
            board empty_positions = VALID_CELLS & ~(b0 | b1);

            board b0_wins = find_winning_stones(b0 | empty_positions) & empty_positions;
            board b1_wins = find_winning_stones(b1 | empty_positions) & empty_positions;
            
            board b0_wins_minus_dead_stones = find_winning_stones((b0 & ~dead_stones) | empty_positions) & empty_positions;
            board b1_wins_minus_dead_stones = find_winning_stones((b1 & ~dead_stones) | empty_positions) & empty_positions;
                    
            board b0_wins_plus_dead_stones = find_winning_stones(b0 | dead_stones | empty_positions) & empty_positions;
            board b1_wins_plus_dead_stones = find_winning_stones(b1 | dead_stones | empty_positions) & empty_positions;
            
            if (b0_wins != b0_wins_minus_dead_stones
                    || b1_wins != b1_wins_minus_dead_stones
                    || (b0_wins & empty_positions) != (b0_wins_plus_dead_stones & empty_positions)
                    || (b1_wins & empty_positions) != (b1_wins_plus_dead_stones & empty_positions)) {
                printf("Trial #%d. Found dead stones which may impact the rest of the game.\n", trial + 1);
                printb(b0, b1);
                printb(dead_stones, 0);

                mu_fail("Dead stone check on random board failed.");
            }
            
            play_random_move(&b0, &b1);
            swap(&b0, &b1);
        }
    }

    return 0;
}


char *test_find_dead_stones_returns_superset_of_dead_stones_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    // Fewer trials than normal as each trial is expensive.
    for (int trial = 0; trial < 100000; trial++) {
        board b0 = 0;
        board b1 = 0;
        
        // Play random moves until the game is draw, or the last player won the game.
        while (!has_won(b1) && !is_draw(b0, b1)) {
            play_random_move(&b0, &b1);

            // Assert that no dead stones can be added without impacting the future of the game.
            board dead_stones = find_dead_stones(b0, b1);
            board alive_stones = (b0 | b1) & ~dead_stones;
            board empty_positions = VALID_CELLS & ~(b0 | b1);

            board b0_wins = find_winning_stones(b0 | empty_positions);
            board b1_wins = find_winning_stones(b1 | empty_positions);

            for (int x = 0; x < BOARD_WIDTH; x++) {
                for (int y = 0; y < BOARD_HEIGHT; y++) {
                    board current_stone = (board) 1 << (y + x * BOARD_HEIGHT_1);
                    board extra_dead_stones = dead_stones | current_stone;
                    
                    board b0_wins_minus_dead_stones = find_winning_stones(
                        (b0 & ~extra_dead_stones) | empty_positions);
                    board b1_wins_minus_dead_stones = find_winning_stones(
                        (b1 & ~extra_dead_stones) | empty_positions);
                    
                    board b0_wins_plus_dead_stones = find_winning_stones(
                        b0 | extra_dead_stones | empty_positions);
                    board b1_wins_plus_dead_stones = find_winning_stones(
                        b1 | extra_dead_stones | empty_positions);
                    
                    if (has_piece_on(alive_stones, x, y)
                            && b0_wins == b0_wins_minus_dead_stones
                            && b0_wins == b0_wins_plus_dead_stones
                            && b1_wins == b1_wins_minus_dead_stones
                            && b1_wins == b1_wins_plus_dead_stones) {
                        printf("Trial #%d. Found additional dead stones.\n", trial + 1);
                        printb(b0, b1);
                        printb(dead_stones, current_stone);
                        printb(b0 | empty_positions, 0);

                        mu_fail("Dead stone check on random board failed.");
                    }
                }
            }
            
            swap(&b0, &b1);
        }
    }

    return 0;
}


char *test_scenario() {
    board b0 = 0;
    board b1 = 0;
    
    mu_assert("ply 0, player 0 has not won.", !has_won(b0));
    mu_assert("ply 0, player 1 has not won.", !has_won(b1));

    b0 = move(b0, b1, 3);
    mu_assert("ply 1, player 0 has not won.", !has_won(b0));
    mu_assert("ply 1, player 1 has not won.", !has_won(b1));
    mu_assert("ply 1, player 1 move.", has_piece_on(b0, 3, 0));

    b1 = move(b1, b0, 3);
    mu_assert("ply 2, player 0 has not won.", !has_won(b0));
    mu_assert("ply 2, player 1 has not won.", !has_won(b1));
    mu_assert("ply 2, player 2 move.", has_piece_on(b1, 3, 1));

    b0 = move(b0, b1, 3);
    mu_assert("ply 3, player 0 has not won.", !has_won(b0));
    mu_assert("ply 3, player 1 has not won.", !has_won(b1));
    mu_assert("ply 3, player 1 move.", has_piece_on(b0, 3, 2));

    b1 = move(b1, b0, 3);
    mu_assert("ply 4, player 0 has not won.", !has_won(b0));
    mu_assert("ply 4, player 1 has not won.", !has_won(b1));
    mu_assert("ply 4, player 2 move.", has_piece_on(b1, 3, 3));

    b0 = move(b0, b1, 3);
    mu_assert("ply 4, player 0 has not won.", !has_won(b0));
    mu_assert("ply 4, player 1 has not won.", !has_won(b1));
    mu_assert("ply 4, player 1 move.", has_piece_on(b0, 3, 4));

    b1 = move(b1, b0, 4);
    mu_assert("ply 5, player 0 has not won.", !has_won(b0));
    mu_assert("ply 5, player 1 has not won.", !has_won(b1));
    mu_assert("ply 5, player 2 move.", has_piece_on(b1, 4, 0));

    b0 = move(b0, b1, 4);
    mu_assert("ply 6, player 0 has not won.", !has_won(b0));
    mu_assert("ply 6, player 1 has not won.", !has_won(b1));
    mu_assert("ply 6, player 0 move.", has_piece_on(b0, 4, 1));

    b1 = move(b1, b0, 4);
    mu_assert("ply 7, player 0 has not won.", !has_won(b0));
    mu_assert("ply 7, player 1 has not won.", !has_won(b1));
    mu_assert("ply 7, player 2 move.", has_piece_on(b1, 4, 2));

    b1 = move(b1, b0, 4);
    mu_assert("ply 8, player 0 has not won.", !has_won(b0));
    mu_assert("ply 8, player 1 has not won.", !has_won(b1));
    mu_assert("ply 8, player 2 move.", has_piece_on(b1, 4, 3));

    b1 = move(b1, b0, 4);
    mu_assert("ply 9, player 0 has not won.", !has_won(b0));
    mu_assert("ply 9, player 1 has not won.", !has_won(b1));
    mu_assert("ply 9, player 2 move.", has_piece_on(b1, 4, 4));

    b1 = move(b1, b0, 4);
    mu_assert("ply 10, player 0 has not won.", !has_won(b0));
    mu_assert("ply 10, player 1 has won.", has_won(b1));
    mu_assert("ply 10, player 2 move.", has_piece_on(b1, 4, 5));

    return 0;
}


char *all_board_tests() {
    mu_run_test(test_has_piece_on_with_empty_board);
    mu_run_test(test_has_piece_on_with_full_board);
    mu_run_test(test_has_piece_on_with_one_piece);
    
    mu_run_test(test_move_on_empty_board);
    mu_run_test(test_move_sequentially);

    mu_run_test(test_has_won_with_vertical);
    mu_run_test(test_has_won_with_horizontal);
    mu_run_test(test_has_won_with_positive_diagonal);
    mu_run_test(test_has_won_with_negative_diagonal);

    mu_run_test(test_is_draw_on_unfinished_games);
    mu_run_test(test_is_draw_on_drawn_game);

    mu_run_test(test_find_threats_on_games_with_vertical_threat);
    mu_run_test(test_find_threats_on_games_with_horizontal_threat);
    mu_run_test(test_find_threats_on_games_with_positive_diagonal_threat);
    mu_run_test(test_find_threats_on_games_with_negative_diagonal_threat);

    mu_run_test(test_is_move_valid);

    mu_run_test(test_get_num_valid_moves_on_known_state);
    mu_run_test(test_get_num_valid_moves_on_random_games);

    mu_run_test(test_is_board_valid_on_boards_with_invalid_column_headers);
    mu_run_test(test_is_board_valid_on_boards_with_valid_board);

    mu_run_test(test_mirror_on_random_games);

    mu_run_test(test_find_dead_stones_with_single_cell);
    mu_run_test(test_find_dead_stones_recognises_stones_blocked_by_left_edge);
    mu_run_test(test_find_dead_stones_recognises_stones_blocked_by_right_edge);
    mu_run_test(test_find_dead_stones_on_drawn_board);
    mu_run_test(test_find_dead_stones_returns_subset_of_dead_stones_on_random_games);
    // mu_run_test(test_find_dead_stones_returns_superset_of_dead_stones_on_random_games);
    
    mu_run_test(test_scenario);

    return 0;
}
