#include <locale.h>
#include <iostream>

#include "../src/solver/settings.h"
#include "../src/solver/solver.h"
#include "known_states.h"
#include "test_position.h"
#include "test_table.h"
#include "unit_test.h"

static_assert(BOARD_WIDTH >= 4, "Board must be at least 4 wide.");
static_assert(BOARD_HEIGHT >= 4, "Board must be at least 4 high.");

static bool all_tests_successful(Solver &solver, bool light_mode) {
    if (light_mode) {
        std::cout << "Running in light test mode." << std::endl;
    }

    std::cout << "Running unit tests . . ." << std::endl;
    run_test(all_position_tests());
    run_test(all_table_tests());

    // Test against states with known scores.
    std::cout << "Running known state tests . . ." << std::endl;
    run_test(all_known_states_tests(solver, light_mode));

    return true;
}

int main(int argc, char **argv) {
    using namespace std::literals;

    Solver solver{};
    
    std::cout.imbue(std::locale(""));
    std::cout << solver.get_settings_string();

    // Check if long running tests are disabled.
    bool light_mode = argc > 1 && argv[1] == "--light"sv;

    if (all_tests_successful(solver, light_mode)) {
        std::cout << "All tests passed." << std::endl;
        return 0;
    } else {
        std::cout << "Tests failed. " << std::endl;
        return -1;
    }
}
