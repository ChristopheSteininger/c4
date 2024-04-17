#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <cmath>
#include <mutex>
#include <set>
#include <string>
#include <thread>

#include "solver/position.h"
#include "solver/solver.h"

static inline constexpr int DEPTH = 4;
static inline constexpr int NUM_SOLVERS = 4;

static std::filesystem::path get_filepath() {
    std::string name = "book-" + std::to_string(BOARD_WIDTH) + "x" + std::to_string(BOARD_HEIGHT) + ".csv";

    return std::filesystem::path(name);
}

static Position to_pos(int index) {
    Position pos{};

    for (int d = 0; d < DEPTH; d++) {
        pos.move(index % BOARD_WIDTH);
        index /= BOARD_WIDTH;
    }

    return pos;
}

static void work(Solver &root_solver, std::mutex &mutex, int &position_index, int &solved_positions,
                 std::set<board> &seen, std::ofstream &file) {
    Solver solver{root_solver};
    bool is_mirrored;
    
    std::unique_lock lock(mutex);

    while (position_index < pow(BOARD_WIDTH, DEPTH)) {
        Position pos = to_pos(position_index);
        board hash = pos.hash(is_mirrored);

        // If we found a new position.
        if (!seen.contains(hash)) {
            seen.insert(hash);

            // We don't access any shared data during solving, so let other threads
            // generate positions to solve.
            lock.unlock();

            int score = solver.solve_strong(pos);
            int move = solver.get_best_move(pos, score);

            if (is_mirrored) {
                move = BOARD_WIDTH - move - 1;
            }

            lock.lock();

            solved_positions++;

            file << hash << "," << move << "," << score << std::endl << std::flush;
            std::cout << "\rSolved " << solved_positions << " positions.";
        }

        position_index++;
    }
}

int main() {
    // Instead of solving one position at a time with n threads, we solve n positions in parallel each with 1 thread.
    if constexpr (NUM_THREADS != 1) {
        std::cout << "Number of worker threads (NUM_THREADS) must be 1, but is " << NUM_THREADS << "." << std::endl;
        return -1;
    }

    // Affinity would pin all solver worker threads to the same core when running multiple solvers in parallel.
    if constexpr (ENABLE_AFFINITY) {
        std::cout << "Thread affinity must be disabled when running multiple solvers in parallel." << std::endl;
        return -1;
    }

    std::filesystem::path filepath = get_filepath();
    if (std::filesystem::exists(filepath)) {
        std::cout << "The file " << filepath << " already exists. Move or delete the file, then rerun." << std::endl;
        return -1;
    }

    std::ofstream file(filepath);
    file << "hash,move,score - This file contains all positions with " << DEPTH
         << " moves on a " << BOARD_WIDTH << "x" << BOARD_HEIGHT << " board." << std::endl;

    std::cout.imbue(std::locale(""));
    std::cout << Solver::get_settings_string()
              << "Generating opening book " << DEPTH << " moves deep." << std::endl
              << std::endl;
    
    Solver root_solver{};
    std::thread threads[NUM_SOLVERS];

    // Data shared between the solver threads.
    std::mutex mutex;
    int position_index = 0;
    int solved_positions = 0;
    std::set<board> seen{};

    auto start_time = std::chrono::steady_clock::now();

    for (int i = 0; i < NUM_SOLVERS; i++) {
        threads[i] = std::thread(&work, std::ref(root_solver), std::ref(mutex), std::ref(position_index),
            std::ref(solved_positions), std::ref(seen), std::ref(file));
    }

    for (int i = 0; i < NUM_SOLVERS; i++) {
        threads[i].join();
    }

    auto run_time = std::chrono::steady_clock::now() - start_time;
    long long run_time_sec = std::chrono::duration_cast<std::chrono::seconds>(run_time).count();

    std::cout << std::endl
              << "Done! Ran for " << run_time_sec << " s." << std::endl
              << std::endl
              << root_solver.get_merged_stats().display_all_stats();

	return 0;
}
