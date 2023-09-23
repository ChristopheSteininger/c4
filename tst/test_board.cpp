#include <cstdio>
#include <cstdlib>

#include "../src/solver/position.h"
#include "../src/solver/settings.h"
#include "minunit.h"

static int get_random_move(Position &pos) {
    int col;
    do {
        col = rand() % BOARD_WIDTH;
    } while (!pos.is_move_valid(col));

    return col;
}

static board set_bit(int x, int y) {
    int shift = y + x * (BOARD_HEIGHT + 1);

    return (board)1 << shift;
}

const char *test_has_won_with_vertical() {
    Position pos = Position();

    // Player 1; Player 2
    pos.move(0);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(0);

    mu_assert("first column win for player 1", pos.has_opponent_won());
    mu_assert("no second column win for player 2", !pos.has_player_won());

    pos = Position();

    // Player 1; Player 2
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 1);

    mu_assert("last column win for player 1", pos.has_opponent_won());
    mu_assert("no column win for player 2", !pos.has_player_won());

    return 0;
}

const char *test_has_won_with_horizontal() {
    Position pos = Position();

    // Player 1; Player 2
    pos.move(0);
    pos.move(0);
    pos.move(1);
    pos.move(0);
    pos.move(2);
    pos.move(0);
    pos.move(3);

    mu_assert("first row win for player 1", pos.has_opponent_won());
    mu_assert("no second row win for player 2", !pos.has_player_won());

    pos = Position();
    pos.move(0);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 4);

    mu_assert("no first row win for player 1", !pos.has_player_won());
    mu_assert("first row win for player 2", pos.has_opponent_won());

    return 0;
}

const char *test_has_won_with_positive_diagonal() {
    Position pos = Position();

    // Player 1; Player 2
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(3);
    pos.move(2);
    pos.move(2);
    pos.move(3);
    pos.move(3);
    pos.move(0);
    pos.move(3);

    // Test evaluation along / diagonal.
    mu_assert("first / diagonal win for player 1", pos.has_opponent_won());
    mu_assert("no first / diagonal win for player 2", !pos.has_player_won());

    return 0;
}

const char *test_has_won_with_negative_diagonal() {
    Position pos = Position();

    // Player 1; Player 2
    pos.move(3);
    pos.move(2);
    pos.move(2);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(0);
    pos.move(0);
    pos.move(3);
    pos.move(0);

    // Test evaluation along / diagonal.
    mu_assert("first \\ diagonal win for player 1", pos.has_opponent_won());
    mu_assert("no first \\ diagonal win for player 2", !pos.has_player_won());

    return 0;
}

const char *test_is_draw_on_unfinished_games() {
    Position pos = Position();
    mu_assert("empty board is not a draw.", !pos.is_draw());

    pos.move(0);
    pos.move(1);
    mu_assert("board with several moves is not a draw.", !pos.is_draw());

    return 0;
}

const char *test_find_threats_on_games_with_vertical_threat() {
    Position pos = Position();

    // Test a vertical threat in the first column.
    // Player 1; Player 2
    pos.move(0);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(0);

    mu_assert("Player 1 has a vertical threat in the first column", pos.find_opponent_threats() == set_bit(0, 3));
    mu_assert("Player 2 has no vertical threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a vertical threat in the last column.
    // Player 1; Player 2
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 1);

    mu_assert("Player 1 has a vertical threat in the last column",
              pos.find_opponent_threats() == set_bit(BOARD_WIDTH - 1, 3));
    mu_assert("Player 2 has no vertical threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a vertical triple blocked by the top of the board.
    // Player 1; Player 2
    for (int y = 0; y < BOARD_HEIGHT - 3; y++) {
        pos.move(0);
    }
    pos.move(0);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(0);

    mu_assert("Player 1 has no vertical threat.", pos.find_opponent_threats() == 0);
    mu_assert("Player 2 has no vertical threat.", pos.find_player_threats() == 0);

    return 0;
}

const char *test_find_threats_on_games_with_horizontal_threat() {
    Position pos = Position();

    // Test a single horiztonal threat.
    // Player 1; Player 2
    pos.move(0);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(2);

    mu_assert("Player 1 has a horizontal threat to the right.", pos.find_player_threats() == set_bit(3, 0));
    mu_assert("Player 2 has no horizontal threat.", pos.find_opponent_threats() == set_bit(3, 1));

    pos = Position();

    // Test a double horizontal threat.
    // Player 1; Player 2
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(2);
    pos.move(3);
    pos.move(3);

    mu_assert("Player 1 has a double horizontal threat.", pos.find_player_threats() == (set_bit(0, 0) | set_bit(4, 0)));
    mu_assert("Player 2 has no horizontal threat.", pos.find_opponent_threats() == (set_bit(0, 1) | set_bit(4, 1)));

    pos = Position();

    // Test a horiztonal threat blocked by the right edge of the board.
    // Player 1              ; Player 2
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 1);

    mu_assert("Player 1 has a horizontal threat to the left.",
              pos.find_player_threats() == set_bit(BOARD_WIDTH - 4, 0));
    mu_assert("Player 2 has no horizontal threat.", pos.find_opponent_threats() == set_bit(BOARD_WIDTH - 4, 1));

    pos = Position();

    // Test a horiztonal threat on the left middle.
    // Player 1; Player 2
    pos.move(0);
    pos.move(0);
    pos.move(2);
    pos.move(2);
    pos.move(3);
    pos.move(3);

    mu_assert("Player 1 has a horizontal threat to the left middle.", pos.find_player_threats() == set_bit(1, 0));
    mu_assert("Player 2 has no horizontal threat.", pos.find_opponent_threats() == set_bit(1, 1));

    pos = Position();

    // Test a horiztonal threat on the right middle.
    // Player 1; Player 2
    pos.move(0);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(3);
    pos.move(3);

    mu_assert("Player 1 has a horizontal threat to the left middle.", pos.find_player_threats() == set_bit(2, 0));
    mu_assert("Player 2 has no horizontal threat.", pos.find_opponent_threats() == set_bit(2, 1));

    return 0;
}

const char *test_find_threats_on_games_with_positive_diagonal_threat() {
    Position pos = Position();

    // Test a threat with the highest stone missing.
    // Player 1; Player 2
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(3);
    pos.move(2);
    pos.move(2);

    mu_assert("Player 1 has a positive diagonal threat for the highest stone.",
              pos.find_opponent_threats() == set_bit(3, 3));
    mu_assert("Player 2 has no positive diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat with the lowest stone missing.
    // Player 1; Player 2
    pos.move(3);
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(2);
    pos.move(3);
    pos.move(2);
    pos.move(3);
    pos.move(3);

    mu_assert("Player 1 has a positive diagonal threat for the lowest stone.",
              pos.find_opponent_threats() == (set_bit(0, 0) | set_bit(4, 4)));
    mu_assert("Player 2 has no positive diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat with the second lowest stone missing.
    // Player 1; Player 2
    pos.move(0);
    pos.move(2);
    pos.move(3);
    pos.move(2);
    pos.move(3);
    pos.move(3);
    pos.move(2);
    pos.move(0);
    pos.move(3);

    mu_assert("Player 1 has a positive diagonal threat for the second lowest stone.",
              pos.find_opponent_threats() == set_bit(1, 1));
    mu_assert("Player 2 has no positive diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat with the second highest stone missing.
    // Player 1; Player 2
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(3);
    pos.move(3);
    pos.move(2);
    pos.move(3);
    pos.move(3);
    pos.move(0);
    pos.move(3);

    mu_assert("Player 1 has a positive diagonal threat for the second highest stone.",
              pos.find_opponent_threats() == set_bit(2, 2));
    mu_assert("Player 2 has no positive diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat blocked by the left edge of the board
    // Player 1; Player 2
    pos.move(2);
    pos.move(0);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(2);
    pos.move(1);
    pos.move(2);
    pos.move(2);

    mu_assert("Player 1 has no positive diagonal threat.", pos.find_opponent_threats() == set_bit(3, 4));
    mu_assert("Player 2 has no positive diagonal threat.", pos.find_player_threats() == 0);

    return 0;
}

const char *test_find_threats_on_games_with_negative_diagonal_threat() {
    Position pos = Position();

    // Test a threat with the highest stone missing.
    // Player 1; Player 2
    pos.move(3);
    pos.move(2);
    pos.move(2);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(3);
    pos.move(0);

    mu_assert("Player 1 has a negative diagonal threat for the highest stone.",
              pos.find_opponent_threats() == set_bit(0, 3));
    mu_assert("Player 2 has no negative diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat with the lowest stone missing.
    // Player 1; Player 2
    pos.move(0);
    pos.move(2);
    pos.move(2);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(0);
    pos.move(0);

    mu_assert("Player 1 has a negative diagonal threat for the lowest stone.",
              pos.find_opponent_threats() == set_bit(3, 0));
    mu_assert("Player 2 has no negative diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat with the second lowest stone missing.
    // Player 1; Player 2
    pos.move(3);
    pos.move(1);
    pos.move(0);
    pos.move(1);
    pos.move(1);
    pos.move(0);
    pos.move(0);
    pos.move(1);
    pos.move(0);

    mu_assert("Player 1 has a negative diagonal threat for the second lowest stone.",
              pos.find_opponent_threats() == set_bit(2, 1));
    mu_assert("Player 2 has no negative diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // Test a threat with the second highest stone missing.
    // Player 1; Player 2
    pos.move(3);
    pos.move(2);
    pos.move(2);
    pos.move(1);
    pos.move(0);
    pos.move(0);
    pos.move(0);
    pos.move(3);
    pos.move(0);

    mu_assert("Player 1 has a negative diagonal threat for the second highest stone.",
              pos.find_opponent_threats() == set_bit(1, 2));
    mu_assert("Player 2 has no negative diagonal threat.", pos.find_player_threats() == 0);

    pos = Position();

    // // Test a threat blocked by the right edge of the board
    // Player 1              ; Player 2
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 1);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 2);
    pos.move(BOARD_WIDTH - 3);
    pos.move(BOARD_WIDTH - 3);

    mu_assert("Player 1 has no negative diagonal threat.", pos.find_opponent_threats() == set_bit(BOARD_WIDTH - 4, 4));
    mu_assert("Player 2 has no negative diagonal threat.", pos.find_player_threats() == set_bit(BOARD_WIDTH - 4, 3));

    return 0;
}

const char *test_is_move_valid() {
    Position pos = Position();

    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            mu_assert("valid move for player 0.", pos.is_move_valid(x));
            pos.move(x);
        }

        mu_assert("invalid move for player 0.", !pos.is_move_valid(x));
    }

    return 0;
}

const char *test_mirror_hash_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 100000; trial++) {
        Position pos = Position();
        Position mirror = Position();

        // Play random moves until the game is draw, or the last player won the game.
        while (!pos.has_opponent_won() && !pos.is_draw()) {
            // Pick and play a random valid move on both boards.
            int col = get_random_move(pos);
            pos.move(col);
            mirror.move(BOARD_WIDTH - col - 1);

            bool is_mirrored_1, is_mirrored_2;
            mu_assert("mirrored hashes must be equal", pos.hash(is_mirrored_1) == mirror.hash(is_mirrored_2));
        }
    }

    return 0;
}

const char *test_find_dead_stones_returns_subset_of_dead_stones_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 100000; trial++) {
        Position pos = Position();

        // Play random moves until the game is draw, or the last player won the game.
        while (!pos.has_opponent_won() && !pos.is_draw()) {
            if (!pos.are_dead_stones_valid()) {
                printf("Trial #%d. Found dead stones which may impact the rest of the game.\n", trial + 1);
                pos.printb();

                mu_fail("Dead stone check on random board failed.");
            }

            int col = get_random_move(pos);
            pos.move(col);
        }
    }

    return 0;
}

// const char *test_find_dead_stones_returns_superset_of_dead_stones_on_random_games() {
//     // Reset the random number sequence.
//     srand(0);

//     // Fewer trials than normal as each trial is expensive.
//     for (int trial = 0; trial < 100000; trial++) {
//         board b0 = 0;
//         board b1 = 0;

//         // Play random moves until the game is draw, or the last player won the game.
//         while (!has_won(b1) && !is_draw(b0, b1)) {
//             play_random_move(&b0, &b1);

//             // Assert that no dead stones can be added without impacting the future of the game.
//             board dead_stones = find_dead_stones(b0, b1);
//             board alive_stones = (b0 | b1) & ~dead_stones;
//             board empty_positions = VALID_CELLS & ~(b0 | b1);

//             board b0_wins = find_winning_stones(b0 | empty_positions);
//             board b1_wins = find_winning_stones(b1 | empty_positions);

//             for (int x = 0; x < BOARD_WIDTH; x++) {
//                 for (int y = 0; y < BOARD_HEIGHT; y++) {
//                     board current_stone = (board) 1 << (y + x * BOARD_HEIGHT_1);
//                     board extra_dead_stones = dead_stones | current_stone;

//                     board b0_wins_minus_dead_stones = find_winning_stones(
//                         (b0 & ~extra_dead_stones) | empty_positions);
//                     board b1_wins_minus_dead_stones = find_winning_stones(
//                         (b1 & ~extra_dead_stones) | empty_positions);

//                     board b0_wins_plus_dead_stones = find_winning_stones(
//                         b0 | extra_dead_stones | empty_positions);
//                     board b1_wins_plus_dead_stones = find_winning_stones(
//                         b1 | extra_dead_stones | empty_positions);

//                     if (has_piece_on(alive_stones, x, y)
//                             && b0_wins == b0_wins_minus_dead_stones
//                             && b0_wins == b0_wins_plus_dead_stones
//                             && b1_wins == b1_wins_minus_dead_stones
//                             && b1_wins == b1_wins_plus_dead_stones) {
//                         printf("Trial #%d. Found additional dead stones.\n", trial + 1);
//                         printb(b0, b1);
//                         printb(dead_stones, current_stone);
//                         printb(b0 | empty_positions, 0);

//                         mu_fail("Dead stone check on random board failed.");
//                     }
//                 }
//             }

//             swap(&b0, &b1);
//         }
//     }

//     return 0;
// }

const char *all_board_tests() {
    mu_run_test(test_has_won_with_vertical);
    mu_run_test(test_has_won_with_horizontal);
    mu_run_test(test_has_won_with_positive_diagonal);
    mu_run_test(test_has_won_with_negative_diagonal);

    mu_run_test(test_is_draw_on_unfinished_games);

    mu_run_test(test_find_threats_on_games_with_vertical_threat);
    mu_run_test(test_find_threats_on_games_with_horizontal_threat);
    mu_run_test(test_find_threats_on_games_with_positive_diagonal_threat);
    mu_run_test(test_find_threats_on_games_with_negative_diagonal_threat);

    mu_run_test(test_is_move_valid);

    mu_run_test(test_mirror_hash_on_random_games);

    mu_run_test(test_find_dead_stones_returns_subset_of_dead_stones_on_random_games);
    // // mu_run_test(test_find_dead_stones_returns_superset_of_dead_stones_on_random_games);

    return 0;
}
