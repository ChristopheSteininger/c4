#include <stdio.h>
#include <locale.h>

#include "minunit.h"
#include "known_states.h"
#include "test_board.h"
#include "../src/settings.h"
#include "../src/board.h"
#include "../src/table.h"
#include "../src/hashing.h"


int tests_run = 0;


char *test_table_lookup_returns_stored_results() {
    board b0 = 0;
    board b1 = 0;
    int counter = 0;
    for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (counter & 1) {
                b0 = move(b0, b1, x);
            } else {
                b1 = move(b1, b0, x);
            }
            
            table_store(b0, b1, x, (counter % 3) + 1, (counter % 3) - 1);
            counter++;
        }
    }
    
    b0 = 0;
    b1 = 0;
    counter = 0;
    for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (counter & 1) {
                b0 = move(b0, b1, x);
            } else {
                b1 = move(b1, b0, x);
            }
            
            int move, type, value;
            int success = table_lookup(b0, b1, &move, &type, &value);

            mu_assert("table lookup in mock game.", success);
            mu_assert("move lookup in mock game.", move == x);
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

    int is_mirrored_1;
    board expected = hash_state(b00, b01, &is_mirrored_1);

    board b10 = 0;
    board b11 = 0;
    b10 = move(b10, b11, 2);
    b11 = move(b11, b10, 3);
    b10 = move(b10, b11, 0);
    b11 = move(b11, b10, 1);

    int is_mirrored_2;
    board actual = hash_state(b10, b11, &is_mirrored_2);

    mu_assert("Equal states must have equal hashes", expected == actual);
    mu_assert("Neither state should be mirrored", is_mirrored_1 == is_mirrored_2);

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

    int is_mirrored_1;
    board expected = hash_state(b00, b01, &is_mirrored_1);

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

    int is_mirrored_2;
    board actual = hash_state(b10, b11, &is_mirrored_2);

    mu_assert("Mirrored states must have equal hashes", expected == actual);
    mu_assert("One state musts be mirrored", is_mirrored_1 != is_mirrored_2);

    return 0;
}


char *test_hash_state_returns_equal_hash_for_states_with_dead_stones() {
    board b00 = 0;
    board b01 = 0;
    b00 = move(b00, b01, 0);
    b00 = move(b00, b01, 0);
    b01 = move(b01, b00, 0);
    b00 = move(b00, b01, 1);
    b00 = move(b00, b01, 1);
    b01 = move(b01, b00, 1);
    b01 = move(b01, b00, 2);
    b01 = move(b01, b00, 2);
    b01 = move(b01, b00, 2);

    int is_mirrored_1;
    board expected = hash_state(b00, b01, &is_mirrored_1);

    // Play the same game, but change the dead stone.
    board b10 = 0;
    board b11 = 0;
    b11 = move(b11, b10, 0);
    b10 = move(b10, b11, 0);
    b11 = move(b11, b10, 0);
    b10 = move(b10, b11, 1);
    b10 = move(b10, b11, 1);
    b11 = move(b11, b10, 1);
    b11 = move(b11, b10, 2);
    b11 = move(b11, b10, 2);
    b11 = move(b11, b10, 2);

    int is_mirrored_2;
    board actual = hash_state(b10, b11, &is_mirrored_2);

    mu_assert("Equal states after accounting for dead cells must have equal hashes", expected == actual);
    mu_assert("Neither state should be mirrored", is_mirrored_1 == is_mirrored_2);

    return 0;
}


char *all_tests() {
    int table_allocation_success = allocate_table();
    mu_assert("table allocation.", table_allocation_success);
    
    mu_assert("Board must be at least 7 wide.", BOARD_WIDTH >= 7);
    mu_assert("Board must be at least 6 high.", BOARD_HEIGHT >= 6);

    printf("Running unit tests . . .\n");

    mu_run_test(all_board_tests);

    // Table tests.
    mu_run_test(test_table_lookup_returns_stored_results);

    // Hash tests.
    mu_run_test(test_hash_state_returns_equal_hash_for_equal_states);
    mu_run_test(test_hash_state_returns_equal_hash_for_mirrored_state);
    mu_run_test(test_hash_state_returns_equal_hash_for_states_with_dead_stones);

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
