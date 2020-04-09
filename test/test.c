#include <stdio.h>
#include <locale.h>

#include "minunit.h"
#include "known_states.h"
#include "../settings.h"
#include "../board.h"
#include "../table.h"
#include "../hashing.h"


int tests_run = 0;


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
        find_threats(b0, b1) == (board) 1 << 3 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", find_threats(b1, b0) == 0);

    // Test a double horizontal threat.
    b0 = move(0, 0, 1);
    b1 = move(0, b0, 1);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a double horizontal threat.",
        find_threats(b0, b1) == (1 | ((board) 1 << 4 * BOARD_HEIGHT_1)));
    mu_assert("Player 2 has no horizontal threat.", find_threats(b1, b0) == 0);

    // Test a horiztonal threat blocked by the right edge of the board.
    b0 = move(0, 0, BOARD_WIDTH - 3);
    b1 = move(0, b0, BOARD_WIDTH - 3);
    b0 = move(b0, b1, BOARD_WIDTH - 2);
    b1 = move(b1, b0, BOARD_WIDTH - 2);
    b0 = move(b0, b1, BOARD_WIDTH - 1);
    b1 = move(b1, b0, BOARD_WIDTH - 1);

    mu_assert("Player 1 has a horizontal threat to the left.",
        find_threats(b0, b1) == (board) 1 << (BOARD_WIDTH - 4) * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", find_threats(b1, b0) == 0);

    // Test a horiztonal threat on the left middle.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 0);
    b0 = move(b0, b1, 2);
    b1 = move(b1, b0, 2);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a horizontal threat to the left middle.",
        find_threats(b0, b1) == (board) 1 << BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", find_threats(b1, b0) == 0);

    // Test a horiztonal threat on the right middle.
    b0 = move(0, 0, 0);
    b1 = move(0, b0, 0);
    b0 = move(b0, b1, 1);
    b1 = move(b1, b0, 1);
    b0 = move(b0, b1, 3);
    b1 = move(b1, b0, 3);

    mu_assert("Player 1 has a horizontal threat to the left middle.",
        find_threats(b0, b1) == (board) 1 << 2 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no horizontal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == (board) 8 << 3 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no positive diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == 1);
    mu_assert("Player 2 has no positive diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == (board) 2 << BOARD_HEIGHT_1);
    mu_assert("Player 2 has no positive diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == (board) 4 << 2 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no positive diagonal threat.", find_threats(b1, b0) == 0);

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

    mu_assert("Player 1 has no positive diagonal threat.", find_threats(b0, b1) == 0);
    mu_assert("Player 2 has no positive diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == 8);
    mu_assert("Player 2 has no negative diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == (board) 1 << 3 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no negative diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == (board) 2 << 2 * BOARD_HEIGHT_1);
    mu_assert("Player 2 has no negative diagonal threat.", find_threats(b1, b0) == 0);

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
        find_threats(b0, b1) == (board) 4 << BOARD_HEIGHT_1);
    mu_assert("Player 2 has no negative diagonal threat.", find_threats(b1, b0) == 0);

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

    mu_assert("Player 1 has no negative diagonal threat.", find_threats(b0, b1) == 0);
    mu_assert("Player 2 has no negative diagonal threat.", find_threats(b1, b0) == 0);

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


char *test_table_lookup_returns_stored_results() {
    board b0 = 0;
    board b1 = 0;
    int counter = 0;
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (counter & 1) {
                b0 = move(b0, b1, x);
            } else {
                b1 = move(b1, b0, x);
            }
            
            table_store(b0, b1, (int) (counter % 3) + 1, (counter % 3) - 1);
            counter++;
        }
    }
    
    b0 = 0;
    b1 = 0;
    counter = 0;
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (counter & 1) {
                b0 = move(b0, b1, x);
            } else {
                b1 = move(b1, b0, x);
            }
            
            int type, value;
            int success = table_lookup(b0, b1, &type, &value);

            mu_assert("table lookup in mock game.", success);
            mu_assert("type lookup in mock game.", type == (counter % 3) + 1);
            mu_assert("value lookup in mock game.", value == (counter % 3) - 1);
            
            counter++;
        }
    }

    return 0;
}


char *test_hash_state_returns_equal_hash_for_equal_states() {
    board b00 = 0;
    board b01 = 0;
    b00 = move(b00, b01, 0);
    b01 = move(b01, b00, 1);
    b00 = move(b00, b01, 2);
    b01 = move(b01, b00, 3);

    board expected = hash_state(b00, b01);

    board b10 = 0;
    board b11 = 0;
    b10 = move(b10, b11, 2);
    b11 = move(b11, b10, 3);
    b10 = move(b10, b11, 0);
    b11 = move(b11, b10, 1);

    board actual = hash_state(b10, b11);

    mu_assert("Equal states must have equal hashes", expected == actual);

    return 0;
}


char *test_hash_state_returns_equal_hash_for_mirrored_state() {
    board b00 = 0;
    board b01 = 0;
    b00 = move(b00, b01, 0);
    b01 = move(b01, b00, 1);
    b00 = move(b00, b01, 2);
    b01 = move(b01, b00, 3);
    b00 = move(b00, b01, 2);
    b01 = move(b01, b00, 3);
    b00 = move(b00, b01, 4);
    b01 = move(b01, b00, 4);
    b00 = move(b00, b01, 5);
    b01 = move(b01, b00, 5);

    board expected = hash_state(b00, b01);

    // Play the same game, but mirrored.
    board b10 = 0;
    board b11 = 0;
    b10 = move(b10, b11, BOARD_WIDTH - 1);
    b11 = move(b11, b10, BOARD_WIDTH - 2);
    b10 = move(b10, b11, BOARD_WIDTH - 3);
    b11 = move(b11, b10, BOARD_WIDTH - 4);
    b10 = move(b10, b11, BOARD_WIDTH - 3);
    b11 = move(b11, b10, BOARD_WIDTH - 4);
    b10 = move(b10, b11, BOARD_WIDTH - 5);
    b11 = move(b11, b10, BOARD_WIDTH - 5);
    b10 = move(b10, b11, BOARD_WIDTH - 6);
    b11 = move(b11, b10, BOARD_WIDTH - 6);

    board actual = hash_state(b10, b11);

    mu_assert("Mirrored states must have equal hashes", expected == actual);

    return 0;
}


char *all_tests() {
    int table_allocation_success = allocate_table();
    mu_assert("table allocation.", table_allocation_success);
    
    mu_assert("Board must be at least 7 wide.", BOARD_WIDTH >= 7);
    mu_assert("Board must be at least 6 high.", BOARD_HEIGHT >= 6);

    printf("Running unit tests . . .\n");

    // Board tests.
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

    mu_run_test(test_is_board_valid_on_boards_with_invalid_column_headers);
    mu_run_test(test_is_board_valid_on_boards_with_valid_board);

    mu_run_test(test_find_dead_stones_with_single_cell);
    mu_run_test(test_find_dead_stones_recognises_stones_blocked_by_left_edge);
    mu_run_test(test_find_dead_stones_recognises_stones_blocked_by_right_edge);

    mu_run_test(test_scenario);

    // Table tests.
    mu_run_test(test_table_lookup_returns_stored_results);

    // Hash tests.
    mu_run_test(test_hash_state_returns_equal_hash_for_equal_states);
    mu_run_test(test_hash_state_returns_equal_hash_for_mirrored_state);

    // Test against states with known scores.
    mu_run_test(all_known_states_tests);

    return 0;
}


int main() {
    // Allow thousands separator.
    setlocale(LC_NUMERIC, "");
    
    printf("Using a %d x %d board and %.2f GB table.\n",
        BOARD_WIDTH, BOARD_HEIGHT, get_table_size_in_gigabytes());
    
    char *result = all_tests();

    free_table();

    printf("Tests run: %d\n", tests_run);
    if (result != 0) {
        printf("Error: %s\n", result);
    } else {
        printf("All tests passed.\n");
    }

    return result != 0;
}
