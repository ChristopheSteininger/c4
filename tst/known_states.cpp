#include <stdio.h>
#include <string.h>
#include <time.h>

#include "known_states.h"
#include "minunit.h"
#include "../src/solver/settings.h"
#include "../src/solver/position.h"
#include "../src/solver/solver.h"
#include "../src/solver/table.h"


struct test_data {
    Position pos;
    int expected;
};


enum TestType {
    WEAK,
    STRONG,
    SELF_PLAY,
};


int sign(int x) {
    if (x < 0) {
        return -1;
    }

    if (x > 0) {
        return 1;
    }

    return 0;
}


struct test_data read_line(char *line) {
    Position pos = Position();

    // Reconstruct the board.
    int i = 0;
    for (; '1' <= line[i] && line[i] <= '7'; i++) {
        pos.move(line[i] - '1');
    }

    int expected = atoi(line + i);

    struct test_data result = {
        .pos = pos,
        .expected = expected
    };

    return result;
}


bool weak_test(Solver &solver, struct test_data test_data) {
    int actual = solver.solve_weak(test_data.pos);
    int expected = sign(test_data.expected);
    
    if (expected != actual) {
        printf("\nThe position below has a weak score %d, but got %d.\n", expected, actual);
        test_data.pos.printb();

        return false;
    }
    return true;
}


bool strong_test(Solver &solver, struct test_data test_data) {
    int actual = solver.solve_strong(test_data.pos);
    
    if (test_data.expected != actual) {
        printf("\nThe position below has a strong score %d, but got %d.\n",
            test_data.expected, actual);
        test_data.pos.printb();

        return false;
    }
    return true;
}


// Tests that if playing a game, the game proceeds as expected. The results of
// solve_strong(), get_best_move(), get_num_moves_prediction(), and get_principal_variation()
// must be consistent with each other for the entire game.
bool self_play_test(Solver &solver, struct test_data test_data) {
    Position pos = Position(test_data.pos);

    int pv_moves[BOARD_WIDTH * BOARD_HEIGHT];
    int expected_score = test_data.expected;
    int expected_num_moves = solver.get_num_moves_prediction(pos, expected_score);
    int num_pv_moves = solver.get_principal_variation(pos, pv_moves);

    // The length of the PV must match the number of expected moves.
    if (expected_num_moves != num_pv_moves + pos.num_moves()) {
        printf("PV length does not match expected num moves. Expected num moves was %d but got %d from PV.",
            expected_num_moves, num_pv_moves + pos.num_moves());
        pos.printb();

        return false;
    }

    // Play the game until it is over, checking that the game is proceeding as predicted at each turn.
    int moves_played;
    for (moves_played = 0; !pos.is_game_over(); moves_played++) {
        int score = solver.solve_strong(pos);
        int move = solver.get_best_move(pos);

        // Fail if the solver outputted an invalid move.
        if (!pos.is_move_valid(move)) {
            printf("Solver gave an invalid move %d.\n", move);
            pos.printb();

            return false;
        }

        // Fail if the solver changed score while playing.
        if (score != expected_score) {
            printf("Solver changed score during play. Expected %d but got %d.", expected_score, score);
            pos.printb();
            
            return false;
        }

        // Fail if the move is not part of the principal variation.
        if (move != pv_moves[moves_played]) {
            printf("Solver gave a move not part of the original principal variation. Move was %d PV move was %d.",
                move, pv_moves[moves_played]);
            pos.printb();

            return false;
        }

        pos.move(move);
        expected_score = -expected_score;
    }

    // Fail if the game ended earlier or later than predicted.
    if (expected_num_moves != pos.num_moves()) {
        printf("Game ended on unexpected move. Expected move count was %d but got %d.", expected_num_moves, pos.num_moves());
        pos.printb();
        
        return false;
    }

    // Fail if the game ended earlier or later than predicted.
    if (expected_num_moves != moves_played + test_data.pos.num_moves()) {
        printf("Game ended on unexpected move. Expected move count was %d but moved %d times.",
            expected_num_moves, moves_played + test_data.pos.num_moves());
        pos.printb();
        
        return false;
    }

    return true;
}


bool run_test(Solver &solver, struct test_data test_data, TestType type) {
    switch (type) {
        case WEAK:
            return weak_test(solver, test_data);

        case STRONG:
            return strong_test(solver, test_data);

        case SELF_PLAY:
            return self_play_test(solver, test_data);
    }

    printf("Unknown test type.\n");
    return false;
}


bool test_with_file(const char *filename, TestType type) {
    FILE *data_file = fopen(filename, "r");
    if (!data_file) {
        printf("Could not open the file.\n");
        return false;
    }

    Solver solver = Solver();

    double total_run_time_ms = 0;
    char line[100];
    for (int num_tests = 0; fgets(line, sizeof(line), data_file) != NULL;) {
        // Read the test data.
        struct test_data test_data = read_line(line);

        // Run the test.
        unsigned long start_time = clock();
        bool result = run_test(solver, test_data, type);
        total_run_time_ms += (clock() - start_time) * 1000 / (double) CLOCKS_PER_SEC;

        if (!result) {
            fclose(data_file);
            return false;
        }

        // Increment before we print the update.
        num_tests++;
        printf("\r\t%-30s %-15s %'15.0f %'15.0f %14.1f%% %'15.0f %'15d",
            filename,
            (type == 0) ? "Weak" : (type == 1) ? "Strong" : "Self Play",
            (double) solver.get_num_nodes() / num_tests,
            solver.get_num_nodes() / total_run_time_ms,
            (double) solver.get_num_best_moves_guessed() * 100 / solver.get_num_interior_nodes(),
            total_run_time_ms / 1000,
            num_tests);
        fflush(stdout);
    }

    printf("\n");

    fclose(data_file);
    return true;
}


const char *all_known_states_tests() {
    mu_assert("Board must be 7 wide.", BOARD_WIDTH == 7);
    mu_assert("Board must be 6 high.", BOARD_HEIGHT == 6);

    printf("Running known state tests . . .\n");
    printf("\t%-30s %-15s %15s %15s %15s %15s %15s\n", "Test", "Type", "Mean nodes", "Nodes per ms",
        "Guess rate", "Time (s)", "Trials");

    const char *test_files[] = {
        "tst/data/endgame_L1.txt",
        "tst/data/midgame_L1.txt",
        "tst/data/midgame_L2.txt",
        "tst/data/opening_L1.txt",
        "tst/data/opening_L2.txt",
        "tst/data/opening_L3.txt",
    };

    for (int i = 0; i < 6; i++) {
        mu_assert("Known state test failed.", test_with_file(test_files[i], WEAK));
        mu_assert("Known state test failed.", test_with_file(test_files[i], STRONG));
        mu_assert("Known state test failed.", test_with_file(test_files[i], SELF_PLAY));

        printf("\n");
    }

    return 0;
}
