#include <stdio.h>
#include <string.h>
#include <time.h>

#include "known_states.h"
#include "minunit.h"
#include "../settings.h"
#include "../board.h"
#include "../solver.h"


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
    printf("\t%-30s", filename);
    
    FILE *data_file = fopen(filename, "r");
    mu_assert("Could not open the file.", data_file != NULL);

    unsigned long total_nodes = 0;
    double total_run_time_ms = 0;

    char line[100];
    int line_number;
    for (line_number = 0; fgets(line, sizeof(line), data_file) != NULL; line_number++) {
        // Read the test data, and execute.
        struct test_data test_data = read_line(line);

        unsigned long start_time = clock();
        int actual = solve(test_data.b0, test_data.b1);
        
        total_run_time_ms += (clock() - start_time) * 1000 / (double) CLOCKS_PER_SEC;
        total_nodes += get_num_nodes();

        // Fail if the solver returned the wrong result.
        if (test_data.expected != actual) {
            printf("The position below has score %d, but got %d.\n", test_data.expected, actual);
            printb(test_data.b0, test_data.b1);
            
            fclose(data_file);
            mu_assert("Known state evaluation failed.", 0);
        }
    }

    fclose(data_file);

    printf(" %'15.2f %'15.0f %'15.0f\n", (double) total_run_time_ms / line_number,
        (double) total_nodes / line_number, total_nodes / total_run_time_ms);
    
    return 0;
}


char *test_easy_end() {
    return test_with_file("test/data/easy_end.txt");
}


char *test_easy_middle() {
    return test_with_file("test/data/easy_middle.txt");
}


char *test_medium_middle() {
    return test_with_file("test/data/medium_middle.txt");
}


char *test_easy_start() {
    return test_with_file("test/data/easy_start.txt");
}


char *all_known_states_tests() {
    mu_assert("Board must be 7 wide.", BOARD_WIDTH == 7);
    mu_assert("Board must be 6 high.", BOARD_HEIGHT == 6);

    printf("Running known state tests . . .\n");
    printf("\t%-30s %15s %15s %15s\n", "Test", "Mean time (ms)", "Mean nodes", "Nodes per ms");

    mu_run_test(test_easy_end);
    
    mu_run_test(test_easy_middle);
    mu_run_test(test_medium_middle);
    
    // mu_run_test(test_easy_start);

    return 0;
}
