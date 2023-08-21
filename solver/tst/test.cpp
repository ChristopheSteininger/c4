#include <stdio.h>
#include <locale.h>

#include "minunit.h"
#include "known_states.h"
#include "test_board.h"
#include "../src/settings.h"
#include "../src/position.h"
#include "../src/table.h"


int tests_run = 0;


const char *test_table_lookup_returns_stored_results() {
    Position pos1 = Position();
    Table table = Table();
    int counter = 0;
    
    for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            pos1.move(x);
            
            table.put(pos1, x, (counter % 3) + 1, (counter % 3) - 1);
            counter++;
        }
    }
    
    Position pos2 = Position();
    counter = 0;
    
    for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            pos2.move(x);
            
            int move, type, value;
            int success = table.get(pos2, move, type, value);

            mu_assert("table lookup in mock game.", success);
            mu_assert("move lookup in mock game.", move == x);
            mu_assert("type lookup in mock game.", type == (counter % 3) + 1);
            mu_assert("value lookup in mock game.", value == (counter % 3) - 1);
            
            counter++;
        }
    }

    return 0;
}


const char *test_hash_state_returns_equal_hash_for_equal_states() {
    Position pos1 = Position();
    pos1.move(0);
    pos1.move(1);
    pos1.move(2);
    pos1.move(3);

    bool is_mirrored_1;
    board expected = pos1.hash(is_mirrored_1);

    Position pos2 = Position();
    pos2.move(0);
    pos2.move(1);
    pos2.move(2);
    pos2.move(3);

    bool is_mirrored_2;
    board actual = pos2.hash(is_mirrored_2);

    mu_assert("Equal states must have equal hashes", expected == actual);
    mu_assert("Neither state should be mirrored", is_mirrored_1 == is_mirrored_2);

    return 0;
}


const char *test_hash_state_returns_equal_hash_for_mirrored_state() {
    Position pos1 = Position();
    pos1.move(0);
    pos1.move(1);
    pos1.move(2);
    pos1.move(3);
    pos1.move(2);
    pos1.move(3);
    pos1.move(4);
    pos1.move(4);
    pos1.move(5);
    pos1.move(5);

    bool is_mirrored_1;
    board expected = pos1.hash(is_mirrored_1);

    // Play the same game, but mirrored.
    Position pos2 = Position();
    pos2.move(BOARD_WIDTH - 1);
    pos2.move(BOARD_WIDTH - 2);
    pos2.move(BOARD_WIDTH - 3);
    pos2.move(BOARD_WIDTH - 4);
    pos2.move(BOARD_WIDTH - 3);
    pos2.move(BOARD_WIDTH - 4);
    pos2.move(BOARD_WIDTH - 5);
    pos2.move(BOARD_WIDTH - 5);
    pos2.move(BOARD_WIDTH - 6);
    pos2.move(BOARD_WIDTH - 6);

    bool is_mirrored_2;
    board actual = pos2.hash(is_mirrored_2);

    mu_assert("Mirrored states must have equal hashes", expected == actual);
    mu_assert("One state musts be mirrored", is_mirrored_1 != is_mirrored_2);

    return 0;
}


const char *test_hash_state_returns_equal_hash_for_states_with_dead_stones() {
    Position pos1 = Position();
    pos1.move(0);
    pos1.move(0);
    pos1.move(0);
    pos1.move(1);
    pos1.move(1);
    pos1.move(1);
    pos1.move(2);
    pos1.move(2);
    pos1.move(2);

    bool is_mirrored_1;
    board expected = pos1.hash(is_mirrored_1);

    // Play the same game, but change the dead stone.
    Position pos2 = Position();
    pos2.move(0);
    pos2.move(0);
    pos2.move(0);
    pos2.move(1);
    pos2.move(1);
    pos2.move(1);
    pos2.move(2);
    pos2.move(2);
    pos2.move(2);

    // TODO: Equal states ^ ??

    bool is_mirrored_2;
    board actual = pos2.hash(is_mirrored_2);

    mu_assert("Equal states after accounting for dead cells must have equal hashes", expected == actual);
    mu_assert("Neither state should be mirrored", is_mirrored_1 == is_mirrored_2);

    return 0;
}


const char *all_tests() {
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
    
    printf("Using a %d x %d board.\n", BOARD_WIDTH, BOARD_HEIGHT);
    
    const char *result = all_tests();

    printf("Tests run: %d\n", tests_run);
    if (result != 0) {
        printf("Error: %s\n", result);
    } else {
        printf("All tests passed.\n");
    }

    return result != 0;
}
