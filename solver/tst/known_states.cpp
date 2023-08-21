#include <stdio.h>
#include <string.h>
#include <time.h>

#include "known_states.h"
#include "minunit.h"
#include "../src/settings.h"
#include "../src/position.h"
#include "../src/solver.h"
#include "../src/table.h"


struct test_data {
    Position pos;
    int expected;
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

    int expected = sign(atoi(line + i));
    
    struct test_data result = {
        .pos = pos,
        .expected = expected
    };

    return result;
}


const char *test_with_file(const char *filename) {
    
    FILE *data_file = fopen(filename, "r");
    mu_assert("Could not open the file.", data_file != NULL);

    unsigned long total_nodes = 0;
    unsigned long total_interior_nodes = 0;
    unsigned long total_best_moves_guessed = 0;
    double total_run_time_ms = 0;

    Solver solver = Solver();

    char line[100];
    for (int num_tests = 0; fgets(line, sizeof(line), data_file) != NULL;) {
        // Read the test data.
        struct test_data test_data = read_line(line);

        unsigned long start_time = clock();
        int actual = solver.solve(test_data.pos);

        total_run_time_ms += (clock() - start_time) * 1000 / (double) CLOCKS_PER_SEC;
        total_nodes += solver.get_num_nodes();
        total_interior_nodes += solver.get_num_interior_nodes();
        total_best_moves_guessed += solver.get_num_best_moves_guessed();

        // Fail if the solver returned the wrong result.
        if (test_data.expected != actual) {
            printf("\nThe position below has score %d, but got %d.\n", test_data.expected, actual);
            test_data.pos.printb();
            
            fclose(data_file);
            mu_assert("Known state evaluation failed.", 0);
        }

        // Increment before we print the update.
        num_tests++;

        printf("\r\t%-30s %'15.0f %'15.0f %14.1f%% %'15.0f %'15d",
            filename,
            (double) total_nodes / num_tests,
            total_nodes / total_run_time_ms,
            (double) total_best_moves_guessed * 100 / total_interior_nodes,
            total_run_time_ms / 1000,
            num_tests);
        fflush(stdout);
    }

    printf("\n");

    fclose(data_file);
    
    return 0;
}


const char *test_endgame_L1() {
    return test_with_file("tst/data/endgame_L1.txt");
}


const char *test_midgame_L1() {
    return test_with_file("tst/data/midgame_L1.txt");
}


const char *test_midgame_L2() {
    return test_with_file("tst/data/midgame_L2.txt");
}


const char *test_opening_L1() {
    return test_with_file("tst/data/opening_L1.txt");
}


const char *test_opening_L2() {
    return test_with_file("tst/data/opening_L2.txt");
}


const char *test_opening_L3() {
    return test_with_file("tst/data/opening_L3.txt");
}


const char *all_known_states_tests() {
    mu_assert("Board must be 7 wide.", BOARD_WIDTH == 7);
    mu_assert("Board must be 6 high.", BOARD_HEIGHT == 6);

    printf("Running known state tests . . .\n");
    printf("\t%-30s %15s %15s %15s %15s %15s\n", "Test", "Mean nodes", "Nodes per ms", "Guess rate", "Time (s)",
        "Trials passed");

    mu_run_test(test_endgame_L1);
    
    mu_run_test(test_midgame_L1);
    mu_run_test(test_midgame_L2);
    
    mu_run_test(test_opening_L1);
    mu_run_test(test_opening_L2);
    mu_run_test(test_opening_L3);

    return 0;
}
