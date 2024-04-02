#include "known_states.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "../src/solver/position.h"
#include "../src/solver/settings.h"
#include "../src/solver/solver.h"
#include "../src/solver/table.h"
#include "minunit.h"

namespace fs = std::filesystem;

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

struct test_data read_line(std::string &line) {
    Position pos{};

    // Reconstruct the board.
    int i = 0;
    for (; line.at(i) != ' '; i++) {
        pos.move(line.at(i) - '1');
    }

    int expected = std::stoi(line.substr(i));

    struct test_data result = {.pos = pos, .expected = expected};

    return result;
}

bool weak_test(Solver &solver, struct test_data test_data) {
    int actual = solver.solve_weak(test_data.pos);
    int expected = sign(test_data.expected);

    if (expected != actual) {
        std::cout << std::endl
                  << "The position below has a weak score of " << expected << ", but got " << actual << std::endl;
        test_data.pos.printb();

        return false;
    }

    return true;
}

bool strong_test(Solver &solver, struct test_data test_data) {
    int actual = solver.solve_strong(test_data.pos);

    if (test_data.expected != actual) {
        std::cout << std::endl
                  << "The position below has a score of " << test_data.expected << ", but got " << actual
                  << std::endl;
        test_data.pos.printb();

        return false;
    }
    return true;
}

// Tests that if playing a game, the game proceeds as expected. The results of
// solve_strong(), score_to_last_move(), and get_principal_variation()
// must be consistent with each other for the entire game.
bool self_play_test(Solver &solver, struct test_data test_data) {
    Position pos{test_data.pos};

    std::vector<int> pv;
    int expected_score = test_data.expected;
    int expected_moves_left = pos.moves_left(expected_score);
    int num_pv_moves = solver.get_principal_variation(pos, pv);

    // The length of the PV must match the number of expected moves.
    if (expected_moves_left != num_pv_moves) {
        std::cout << "PV length does not match expected num moves. Expected num moves was " << expected_moves_left
                  << " but got " << num_pv_moves << " from PV." << std::endl;
        pos.printb();

        return false;
    }

    // Play the game until it is over, checking that the game is proceeding as predicted at each turn.
    int moves_played;
    for (moves_played = 0; !pos.is_game_over(); moves_played++) {
        int score = solver.solve_strong(pos);
        int move = pv[moves_played];

        // Fail if the solver outputted an invalid move.
        if (!pos.is_move_valid(move)) {
            std::cout << "Solver gave an invalid move " << move << "." << std::endl;
            pos.printb();

            return false;
        }

        // Fail if the solver changed score while playing.
        if (score != expected_score) {
            std::cout << "Solver changed score during play. Expected " << expected_score << " but got " << score << "."
                      << std::endl;
            pos.printb();

            return false;
        }

        pos.move(move);
        expected_score = -expected_score;
    }

    // Fail if number of moves played does not match the prediction.
    if (expected_moves_left != moves_played) {
        std::cout << "Game ended after unexpected number of moves. Expected " << expected_moves_left << " moves but got "
                  << moves_played << " moves." << std::endl;
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

    std::cout << "Unknown test type." << std::endl;
    return false;
}

std::string get_type_name(TestType type) {
    switch (type) {
        case TestType::WEAK:
            return "Weak";

        case TestType::STRONG:
            return "Strong";

        case TestType::SELF_PLAY:
            return "Self Play";
    }

    return "Unknown test type.";
}

static void print_update(const fs::path &file, TestType type, const Solver &solver, int num_tests,
                         std::chrono::steady_clock::duration total_run_time) {
    const Stats &stats = solver.get_merged_stats();
    long long total_run_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(total_run_time).count();

    // clang-format off
    std::cout << "\r\t" << std::fixed << std::left << std::setw(35) << file.string() << std::setw(15)
              << get_type_name(type) << std::right << std::setw(10) << std::setprecision(0)
              << (double)stats.get_num_nodes() / num_tests << std::setw(15) << std::setprecision(0)
              << (double)stats.get_num_nodes() / total_run_time_ms << std::setw(14) << std::setprecision(1)
              << stats.get_best_move_guess_rate() * 100 << "%" << std::setw(15) << std::setprecision(2)
              << (double)total_run_time_ms / 1000 << std::setw(15) << num_tests << std::flush;
    // clang-format on
}

bool test_with_file(const fs::path &file, TestType type, Solver &solver) {
    std::ifstream data_file(file);
    if (!data_file.is_open()) {
        std::cout << "Could not open the file: " << file.string() << std::endl;
        return false;
    }

    solver.clear_state();

    std::chrono::steady_clock::duration total_run_time{};
    
    auto last_console_update = std::chrono::steady_clock::now();
    auto min_console_update = std::chrono::milliseconds(100);

    std::string line;
    int num_tests = 0;
    while (std::getline(data_file, line)) {
        // Read the test data.
        struct test_data test_data = read_line(line);

        // Run the test.
        auto start_time = std::chrono::steady_clock::now();
        bool result = run_test(solver, test_data, type);
        total_run_time += std::chrono::steady_clock::now() - start_time;

        num_tests++;

        if (!result) {
            print_update(file, type, solver, num_tests, total_run_time);
            std::cout << std::endl;
            return false;
        }

        // Update the console with our progress so far.
        if (std::chrono::steady_clock::now() - last_console_update > min_console_update) {
            last_console_update = std::chrono::steady_clock::now();
            print_update(file, type, solver, num_tests, total_run_time);
        }
    }

    print_update(file, type, solver, num_tests, total_run_time);
    std::cout << std::endl;

    return true;
}

const char *all_known_states_tests(bool light_mode) {
    std::string dir_name = std::to_string(BOARD_WIDTH) + "x" + std::to_string(BOARD_HEIGHT);
    fs::path test_dir = fs::path("tst") / "data" / dir_name;

    // Test data has not been generated for all board sizes.
    if (!fs::is_directory(test_dir)) {
        std::cout << "Could not find a directory with test data for this board size: '"
            << test_dir << "'." << std::endl;
        return 0;
    }

    // clang-format off
    std::cout.imbue(std::locale(""));
    std::cout << "Running known state tests . . ." << std::endl;
    std::cout << '\t' << std::left << std::setw(35) << "Test"
              << std::setw(10) << "Type"
              << std::right << std::setw(15) << "Mean nodes"
              << std::setw(15) << "Nodes per ms"
              << std::setw(15) << "Guess rate"
              << std::setw(15) << "Time (s)" << std::setw(15) << "Trials"
              << std::endl;
    // clang-format on

    // Sort alphabetically as the first test file will contain the easiest positions,
    // and the last file will contain the most complex positions.
    std::vector<fs::path> test_files;
    std::copy(fs::directory_iterator(test_dir), fs::directory_iterator(), std::back_inserter(test_files));
    std::sort(test_files.begin(), test_files.end());

    // Only test with the easiest positions if light mode is enabled.
    if (light_mode) {
        test_files.erase(test_files.begin() + 1, test_files.end());
    }

    Solver solver{};
    for (fs::path file : test_files) {
        mu_assert("Known state test failed.", test_with_file(file, WEAK, solver));
        mu_assert("Known state test failed.", test_with_file(file, STRONG, solver));
        mu_assert("Known state test failed.", test_with_file(file, SELF_PLAY, solver));

        std::cout << std::endl;
    }

    return 0;
}
