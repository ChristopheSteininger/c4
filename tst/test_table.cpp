#include "test_table.h"

#include <iostream>

#include "../src/solver/position.h"
#include "../src/solver/table.h"
#include "unit_test.h"

static bool test_table_lookup_returns_stored_results() {
    Position pos1{};
    Table table{};

    int counter = 0;
    bool is_mirrored;

    for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            pos1.move(x);

            board hash = pos1.hash(is_mirrored);
            table.put(hash, is_mirrored, x, static_cast<NodeType>((counter % 3) + 1), (counter % 3), 1);
            counter++;
        }
    }

    Position pos2{};
    counter = 0;

    for (int y = 0; y < BOARD_HEIGHT - 1; y++) {
        for (int x = 0; x < BOARD_WIDTH; x++) {
            pos2.move(x);

            board hash = pos2.hash(is_mirrored);
            Entry entry = table.get(hash);

            expect_true("move lookup in mock game.", entry.get_move(is_mirrored) == x);
            expect_true("type lookup in mock game.", entry.get_type() == static_cast<NodeType>((counter % 3) + 1));
            expect_true("value lookup in mock game.", entry.get_score() == (counter % 3));

            counter++;
        }
    }

    return true;
}

static bool test_hash_state_returns_equal_hash_for_equal_states() {
    Position pos1{};

    // Player 1 ; Player 2
    pos1.move(0); pos1.move(1);
    pos1.move(2); pos1.move(3);

    bool is_mirrored_1;
    board expected = pos1.hash(is_mirrored_1);

    Position pos2{};

    // Player 1 ; Player 2
    pos2.move(0); pos2.move(1);
    pos2.move(2); pos2.move(3);

    bool is_mirrored_2;
    board actual = pos2.hash(is_mirrored_2);

    expect_true("Equal states must have equal hashes", expected == actual);
    expect_true("Neither state should be mirrored", is_mirrored_1 == is_mirrored_2);

    return true;
}

static bool test_hash_state_returns_equal_hash_for_mirrored_state() {
    Position pos1{};

    // Player 1 ; Player 2
    pos1.move(0); pos1.move(1);
    pos1.move(2); pos1.move(3);
    pos1.move(2); pos1.move(3);
    pos1.move(4); pos1.move(4);
    pos1.move(5); pos1.move(5);

    bool is_mirrored_1;
    board expected = pos1.hash(is_mirrored_1);

    // Play the same game, but mirrored.
    Position pos2{};

    // Player 1               ; Player 2
    pos2.move(BOARD_WIDTH - 1); pos2.move(BOARD_WIDTH - 2);
    pos2.move(BOARD_WIDTH - 3); pos2.move(BOARD_WIDTH - 4);
    pos2.move(BOARD_WIDTH - 3); pos2.move(BOARD_WIDTH - 4);
    pos2.move(BOARD_WIDTH - 5); pos2.move(BOARD_WIDTH - 5);
    pos2.move(BOARD_WIDTH - 6); pos2.move(BOARD_WIDTH - 6);

    bool is_mirrored_2;
    board actual = pos2.hash(is_mirrored_2);

    expect_true("Mirrored states must have equal hashes", expected == actual);
    expect_true("One state musts be mirrored", is_mirrored_1 != is_mirrored_2);

    return true;
}

static bool test_hash_state_returns_equal_hash_for_states_with_dead_stones() {
    Position pos1 = Position();

    // Player 1 ; Player 2
    pos1.move(0); pos1.move(0);
    pos1.move(0); pos1.move(1);
    pos1.move(1); pos1.move(1);
    pos1.move(2); pos1.move(2);
    pos1.move(2);

    bool is_mirrored_1;
    board expected = pos1.hash(is_mirrored_1);

    // Play the same game, but change the dead stone.
    Position pos2 = Position();

    // Player 1 ; Player 2
    pos2.move(0); pos2.move(0);
    pos2.move(0); pos2.move(1);
    pos2.move(1); pos2.move(1);
    pos2.move(2); pos2.move(2);
    pos2.move(2);

    bool is_mirrored_2;
    board actual = pos2.hash(is_mirrored_2);

    expect_true("Equal states after accounting for dead cells must have equal hashes", expected == actual);
    expect_true("Neither state should be mirrored", is_mirrored_1 == is_mirrored_2);

    return true;
}

bool all_table_tests() {
    run_test(test_table_lookup_returns_stored_results());

    run_test(test_hash_state_returns_equal_hash_for_equal_states());
    run_test(test_hash_state_returns_equal_hash_for_mirrored_state());
    run_test(test_hash_state_returns_equal_hash_for_states_with_dead_stones());

    return true;
}
