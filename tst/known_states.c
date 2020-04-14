#include <stdio.h>
#include <string.h>
#include <time.h>

#include "known_states.h"
#include "minunit.h"
#include "../src/settings.h"
#include "../src/board.h"
#include "../src/solver.h"
#include "../src/table.h"


struct test_data {
    board b0;
    board b1;
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
    board b0 = 0;
    board b1 = 0;

    // Reconstruct the board.
    int pos = 0;
    for (; '1' <= line[pos] && line[pos] <= '7'; pos++) {
        b0 = move(b0, b1, line[pos] - '1');

        board tmp = b0;
        b0 = b1;
        b1 = tmp;
    }

    int expected = sign(atoi(line + pos));
    
    struct test_data result = {
        .b0 = b0,
        .b1 = b1,
        .expected = expected
    };

    return result;
}


char *test_with_file(char *filename) {
    
    FILE *data_file = fopen(filename, "r");
    mu_assert("Could not open the file.", data_file != NULL);

    unsigned long total_nodes = 0;
    double total_run_time_ms = 0;

    char line[100];
    for (int line_number = 0; fgets(line, sizeof(line), data_file) != NULL;) {
        // Read the test data.
        struct test_data test_data = read_line(line);

        clear_table();

        unsigned long start_time = clock();
        int actual = solve(test_data.b0, test_data.b1);
        
        total_run_time_ms += (clock() - start_time) * 1000 / (double) CLOCKS_PER_SEC;
        total_nodes += get_num_nodes();

        // Fail if the solver returned the wrong result.
        if (test_data.expected != actual) {
            printf("\nThe position below has score %d, but got %d.\n", test_data.expected, actual);
            printb(test_data.b0, test_data.b1);
            
            fclose(data_file);
            mu_assert("Known state evaluation failed.", 0);
        }

        // Increment before we print the update.
        line_number++;

        printf("\r\t%-30s %'15.0f %'15.0f %'15.0f %'15d",
            filename,
            (double) total_nodes / line_number,
            total_nodes / total_run_time_ms,
            total_run_time_ms / 1000,
            line_number);
        fflush(stdout);
    }

    printf("\n");

    fclose(data_file);
    
    return 0;
}


char *test_endgame_L1() {
    return test_with_file("tst/data/endgame_L1.txt");
}


char *test_midgame_L1() {
    return test_with_file("tst/data/midgame_L1.txt");
}


char *test_midgame_L2() {
    return test_with_file("tst/data/midgame_L2.txt");
}


char *test_opening_L1() {
    return test_with_file("tst/data/opening_L1.txt");
}


char *test_opening_L2() {
    return test_with_file("tst/data/opening_L2.txt");
}


char *test_opening_L3() {
    return test_with_file("tst/data/opening_L3.txt");
}


char *all_known_states_tests() {
    mu_assert("Board must be 7 wide.", BOARD_WIDTH == 7);
    mu_assert("Board must be 6 high.", BOARD_HEIGHT == 6);

    printf("Running known state tests . . .\n");
    printf("\t%-30s %15s %15s %15s %15s\n", "Test", "Mean nodes", "Nodes per ms", "Time (s)",
        "Trials passed");

    mu_run_test(test_endgame_L1);
    
    mu_run_test(test_midgame_L1);
    mu_run_test(test_midgame_L2);
    
    mu_run_test(test_opening_L1);
    mu_run_test(test_opening_L2);
    mu_run_test(test_opening_L3);

    return 0;
}
