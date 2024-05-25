#include "test_position.h"

#include <cstdlib>
#include <iostream>

#include "../src/solver/position.h"
#include "../src/solver/settings.h"
#include "unit_test.h"

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

static bool test_has_won_with_vertical() {
    Position pos1{};

    // Player 1; Player 2
    pos1.move(0); pos1.move(1);
    pos1.move(0); pos1.move(1);
    pos1.move(0); pos1.move(1);
    pos1.move(0);

    expect_true("first column win for player 1", pos1.has_opponent_won());
    expect_true("no second column win for player 2", !pos1.has_player_won());

    Position pos2{};

    // Player 1; Player 2
    pos2.move(0); pos2.move(BOARD_WIDTH - 1);
    pos2.move(BOARD_WIDTH - 2); pos2.move(BOARD_WIDTH - 1);
    pos2.move(BOARD_WIDTH - 2); pos2.move(BOARD_WIDTH - 1);
    pos2.move(BOARD_WIDTH - 2); pos2.move(BOARD_WIDTH - 1);

    expect_true("last column win for player 1", pos2.has_opponent_won());
    expect_true("no column win for player 2", !pos2.has_player_won());

    return true;
}

static bool test_has_won_with_horizontal() {
    Position pos1{};

    // Player 1; Player 2
    pos1.move(0); pos1.move(0);
    pos1.move(1); pos1.move(0);
    pos1.move(2); pos1.move(0);
    pos1.move(3);

    expect_true("first row win for player 1", pos1.has_opponent_won());
    expect_true("no second row win for player 2", !pos1.has_player_won());

    Position pos2{};

    // Player 1; Player 2
    pos2.move(0); pos2.move(BOARD_WIDTH - 1);
    pos2.move(BOARD_WIDTH - 1); pos2.move(BOARD_WIDTH - 2);
    pos2.move(BOARD_WIDTH - 2); pos2.move(BOARD_WIDTH - 3);
    pos2.move(BOARD_WIDTH - 3); pos2.move(BOARD_WIDTH - 4);

    expect_true("no first row win for player 1", !pos2.has_player_won());
    expect_true("first row win for player 2", pos2.has_opponent_won());

    return true;
}

static bool test_has_won_with_positive_diagonal() {
    Position pos{};

    // Player 1; Player 2
    pos.move(0); pos.move(1);
    pos.move(1); pos.move(2);
    pos.move(3); pos.move(2);
    pos.move(2); pos.move(3);
    pos.move(3); pos.move(0);
    pos.move(3);

    // Test evaluation along / diagonal.
    expect_true("first / diagonal win for player 1", pos.has_opponent_won());
    expect_true("no first / diagonal win for player 2", !pos.has_player_won());

    return true;
}

static bool test_has_won_with_negative_diagonal() {
    Position pos = Position();

    // Player 1; Player 2
    pos.move(3); pos.move(2);
    pos.move(2); pos.move(1);
    pos.move(0); pos.move(1);
    pos.move(1); pos.move(0);
    pos.move(0); pos.move(3);
    pos.move(0);

    // Test evaluation along \ diagonal.
    expect_true("first \\ diagonal win for player 1", pos.has_opponent_won());
    expect_true("no first \\ diagonal win for player 2", !pos.has_player_won());

    return true;
}

static bool test_is_draw_on_unfinished_games() {
    Position pos{};
    expect_true("empty board is not a draw", !pos.is_draw());

    // Player 1; Player 2
    pos.move(0); pos.move(1);

    expect_true("board with several moves is not a draw", !pos.is_draw());

    return true;
}

static bool test_find_threats_on_games_with_vertical_threat() {
    Position pos1{};

    // Test a vertical threat in the first column.
    // Player 1; Player 2
    pos1.move(0); pos1.move(1);
    pos1.move(0); pos1.move(1);
    pos1.move(0);

    expect_true("Player 1 has a vertical threat in the first column",
        pos1.find_opponent_threats() == set_bit(0, 3));
    expect_true("Player 2 has no vertical threat", pos1.find_player_threats() == 0);

    Position pos2{};

    // Test a vertical threat in the last column.
    // Player 1; Player 2
    pos2.move(BOARD_WIDTH - 1); pos2.move(BOARD_WIDTH - 2);
    pos2.move(BOARD_WIDTH - 1); pos2.move(BOARD_WIDTH - 2);
    pos2.move(BOARD_WIDTH - 1);

    expect_true("Player 1 has a vertical threat in the last column",
              pos2.find_opponent_threats() == set_bit(BOARD_WIDTH - 1, 3));
    expect_true("Player 2 has no vertical threat", pos2.find_player_threats() == 0);

    Position pos3{};

    // Test a vertical triple blocked by the top of the board.
    // Player 1; Player 2
    for (int y = 0; y < BOARD_HEIGHT - 3; y++) {
        pos3.move(0);
    }
    pos3.move(0); pos3.move(1);
    pos3.move(0); pos3.move(1);
    pos3.move(0);

    expect_true("Player 1 has no vertical threat", pos3.find_opponent_threats() == 0);
    expect_true("Player 2 has no vertical threat", pos3.find_player_threats() == 0);

    return true;
}

static bool test_find_threats_on_games_with_horizontal_threat() {
    Position pos1{};

    // Test a single horiztonal threat.
    // Player 1; Player 2
    pos1.move(0); pos1.move(0);
    pos1.move(1); pos1.move(1);
    pos1.move(2); pos1.move(2);

    expect_true("Player 1 has a horizontal threat to the right", pos1.find_player_threats() == set_bit(3, 0));
    expect_true("Player 2 has no horizontal threat", pos1.find_opponent_threats() == set_bit(3, 1));

    Position pos2{};

    // Test a double horizontal threat.
    // Player 1; Player 2
    pos2.move(1); pos2.move(1);
    pos2.move(2); pos2.move(2);
    pos2.move(3); pos2.move(3);

    expect_true("Player 1 has a double horizontal threat", pos2.find_player_threats() == (set_bit(0, 0) | set_bit(4, 0)));
    expect_true("Player 2 has no horizontal threat", pos2.find_opponent_threats() == (set_bit(0, 1) | set_bit(4, 1)));

    Position pos3{};

    // Test a horiztonal threat blocked by the right edge of the board.
    // Player 1               ; Player 2
    pos3.move(BOARD_WIDTH - 3); pos3.move(BOARD_WIDTH - 3);
    pos3.move(BOARD_WIDTH - 2); pos3.move(BOARD_WIDTH - 2);
    pos3.move(BOARD_WIDTH - 1); pos3.move(BOARD_WIDTH - 1);

    expect_true("Player 1 has a horizontal threat to the left",
              pos3.find_player_threats() == set_bit(BOARD_WIDTH - 4, 0));
    expect_true("Player 2 has no horizontal threat", pos3.find_opponent_threats() == set_bit(BOARD_WIDTH - 4, 1));

    Position pos4{};

    // Test a horiztonal threat on the left middle.
    // Player 1; Player 2
    pos4.move(0); pos4.move(0);
    pos4.move(2); pos4.move(2);
    pos4.move(3); pos4.move(3);

    expect_true("Player 1 has a horizontal threat to the left middle", pos4.find_player_threats() == set_bit(1, 0));
    expect_true("Player 2 has no horizontal threat", pos4.find_opponent_threats() == set_bit(1, 1));

    Position pos5{};

    // Test a horiztonal threat on the right middle.
    // Player 1; Player 2
    pos5.move(0); pos5.move(0);
    pos5.move(1); pos5.move(1);
    pos5.move(3); pos5.move(3);

    expect_true("Player 1 has a horizontal threat to the left middle", pos5.find_player_threats() == set_bit(2, 0));
    expect_true("Player 2 has no horizontal threat", pos5.find_opponent_threats() == set_bit(2, 1));

    return true;
}

static bool test_find_threats_on_games_with_positive_diagonal_threat() {
    Position pos1{};

    // Test a threat with the highest stone missing.
    // Player 1 ; Player 2
    pos1.move(0); pos1.move(1);
    pos1.move(1); pos1.move(2);
    pos1.move(3); pos1.move(2);
    pos1.move(2);

    expect_true("Player 1 has a positive diagonal threat for the highest stone",
              pos1.find_opponent_threats() == set_bit(3, 3));
    expect_true("Player 2 has no positive diagonal threat", pos1.find_player_threats() == 0);

    Position pos2{};

    // Test a threat with the lowest stone missing.
    // Player 1 ; Player 2
    pos2.move(3); pos2.move(1);
    pos2.move(1); pos2.move(2);
    pos2.move(2); pos2.move(3);
    pos2.move(2); pos2.move(3);
    pos2.move(3);

    expect_true("Player 1 has a positive diagonal threat for the lowest stone",
              pos2.find_opponent_threats() == (set_bit(0, 0) | set_bit(4, 4)));
    expect_true("Player 2 has no positive diagonal threat", pos2.find_player_threats() == 0);

    Position pos4{};

    // Test a threat with the second lowest stone missing.
    // Player 1 ; Player 2
    pos4.move(0); pos4.move(2);
    pos4.move(3); pos4.move(2);
    pos4.move(3); pos4.move(3);
    pos4.move(2); pos4.move(0);
    pos4.move(3);

    expect_true("Player 1 has a positive diagonal threat for the second lowest stone",
              pos4.find_opponent_threats() == set_bit(1, 1));
    expect_true("Player 2 has no positive diagonal threat", pos4.find_player_threats() == 0);

    Position pos5{};

    // Test a threat with the second highest stone missing.
    // Player 1 ; Player 2
    pos5.move(0); pos5.move(1);
    pos5.move(1); pos5.move(2);
    pos5.move(3); pos5.move(3);
    pos5.move(2); pos5.move(3);
    pos5.move(3); pos5.move(0);
    pos5.move(3);

    expect_true("Player 1 has a positive diagonal threat for the second highest stone",
              pos5.find_opponent_threats() == set_bit(2, 2));
    expect_true("Player 2 has no positive diagonal threat", pos5.find_player_threats() == 0);

    Position pos6{};

    // Test a threat blocked by the left edge of the board
    // Player 1 ; Player 2
    pos6.move(2); pos6.move(0);
    pos6.move(0); pos6.move(1);
    pos6.move(1); pos6.move(2);
    pos6.move(1); pos6.move(2);
    pos6.move(2);

    expect_true("Player 1 has no positive diagonal threat", pos6.find_opponent_threats() == set_bit(3, 4));
    expect_true("Player 2 has no positive diagonal threat", pos6.find_player_threats() == 0);

    return true;
}

static bool test_find_threats_on_games_with_negative_diagonal_threat() {
    Position pos1{};

    // Test a threat with the highest stone missing.
    // Player 1 ; Player 2
    pos1.move(3); pos1.move(2);
    pos1.move(2); pos1.move(1);
    pos1.move(0); pos1.move(1);
    pos1.move(1); pos1.move(3);
    pos1.move(0);

    expect_true("Player 1 has a negative diagonal threat for the highest stone",
              pos1.find_opponent_threats() == set_bit(0, 3));
    expect_true("Player 2 has no negative diagonal threat", pos1.find_player_threats() == 0);

    Position pos2{};

    // Test a threat with the lowest stone missing.
    // Player 1 ; Player 2
    pos2.move(0); pos2.move(2);
    pos2.move(2); pos2.move(1);
    pos2.move(0); pos2.move(1);
    pos2.move(1); pos2.move(0);
    pos2.move(0);

    expect_true("Player 1 has a negative diagonal threat for the lowest stone",
              pos2.find_opponent_threats() == set_bit(3, 0));
    expect_true("Player 2 has no negative diagonal threat", pos2.find_player_threats() == 0);

    Position pos3{};

    // Test a threat with the second lowest stone missing.
    // Player 1 ; Player 2
    pos3.move(3); pos3.move(1);
    pos3.move(0); pos3.move(1);
    pos3.move(1); pos3.move(0);
    pos3.move(0); pos3.move(1);
    pos3.move(0);

    expect_true("Player 1 has a negative diagonal threat for the second lowest stone",
              pos3.find_opponent_threats() == set_bit(2, 1));
    expect_true("Player 2 has no negative diagonal threat", pos3.find_player_threats() == 0);

    Position pos4{};

    // Test a threat with the second highest stone missing.
    // Player 1 ; Player 2
    pos4.move(3); pos4.move(2);
    pos4.move(2); pos4.move(1);
    pos4.move(0); pos4.move(0);
    pos4.move(0); pos4.move(3);
    pos4.move(0);

    expect_true("Player 1 has a negative diagonal threat for the second highest stone",
              pos4.find_opponent_threats() == set_bit(1, 2));
    expect_true("Player 2 has no negative diagonal threat", pos4.find_player_threats() == 0);

    Position pos5{};

    // // Test a threat blocked by the right edge of the board
    // Player 1               ; Player 2
    pos5.move(BOARD_WIDTH - 3); pos5.move(BOARD_WIDTH - 1);
    pos5.move(BOARD_WIDTH - 1); pos5.move(BOARD_WIDTH - 2);
    pos5.move(BOARD_WIDTH - 3); pos5.move(BOARD_WIDTH - 2);
    pos5.move(BOARD_WIDTH - 2); pos5.move(BOARD_WIDTH - 3);
    pos5.move(BOARD_WIDTH - 3);

    expect_true("Player 1 has no negative diagonal threat", pos5.find_opponent_threats() == set_bit(BOARD_WIDTH - 4, 4));
    expect_true("Player 2 has no negative diagonal threat", pos5.find_player_threats() == set_bit(BOARD_WIDTH - 4, 3));

    return true;
}

static bool test_is_move_valid() {
    Position pos{};

    for (int x = 0; x < BOARD_WIDTH; x++) {
        for (int y = 0; y < BOARD_HEIGHT; y++) {
            expect_true("valid move for player 0", pos.is_move_valid(x));
            pos.move(x);
        }

        expect_true("invalid move for player 0", !pos.is_move_valid(x));
    }

    return true;
}

static bool test_mirror_hash_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 100 * 1000; trial++) {
        Position pos{};
        Position mirror{};

        // Play random moves until the game is draw, or the last player won the game.
        while (!pos.is_game_over()) {
            // Pick and play a random valid move on both boards.
            int col = get_random_move(pos);
            pos.move(col);
            mirror.move(BOARD_WIDTH - col - 1);

            bool is_mirrored_1, is_mirrored_2;
            expect_true("mirrored hashes must be equal", pos.hash(is_mirrored_1) == mirror.hash(is_mirrored_2));
        }
    }

    return true;
}

static bool test_find_dead_stones_returns_subset_of_dead_stones_on_random_games() {
    // Reset the random number sequence.
    srand(0);

    for (int trial = 0; trial < 100 * 1000; trial++) {
        Position pos = Position();

        // Play random moves until the game is draw, or the last player won the game.
        while (!pos.is_game_over()) {
            if (!pos.are_dead_stones_valid()) {
                std::cout << "Trial #" << trial + 1 << ". Found dead stones which may impact the rest of the game" << std::endl
                          << pos.display_board();

                fail("Dead stone check on random board failed");
            }

            int col = get_random_move(pos);
            pos.move(col);
        }
    }

    return true;
}

// Commented out as we do not have an efficent way of detecting dead stones in all
// possible cases yet.

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
//                         std::cout << "Trial #" << trial + 1 << ". Found additional dead stones" << std::endl;
//                         printb(b0, b1);
//                         printb(dead_stones, current_stone);
//                         printb(b0 | empty_positions, 0);

//                         mu_fail("Dead stone check on random board failed");
//                     }
//                 }
//             }

//             swap(&b0, &b1);
//         }
//     }

//     return 0;
// }

bool all_position_tests() {
    run_test(test_has_won_with_vertical());
    run_test(test_has_won_with_horizontal());
    run_test(test_has_won_with_positive_diagonal());
    run_test(test_has_won_with_negative_diagonal());

    run_test(test_is_draw_on_unfinished_games());

    run_test(test_find_threats_on_games_with_vertical_threat());
    run_test(test_find_threats_on_games_with_horizontal_threat());
    run_test(test_find_threats_on_games_with_positive_diagonal_threat());
    run_test(test_find_threats_on_games_with_negative_diagonal_threat());

    run_test(test_is_move_valid());

    run_test(test_mirror_hash_on_random_games());

    run_test(test_find_dead_stones_returns_subset_of_dead_stones_on_random_games());
    // run_test(test_find_dead_stones_returns_superset_of_dead_stones_on_random_games());

    return true;
}
