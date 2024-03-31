#include "solver.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "Tracy.hpp"
#include "position.h"
#include "settings.h"
#include "table.h"

Solver::~Solver() {
    // pool.print_pool_stats();
}

int Solver::solve_weak(const Position &pos) {
    ZoneScoped;

    int result = solve(pos, -1, 1);

    if (result > 0) {
        return 1;
    } else if (result < 0) {
        return -1;
    } else {
        return 0;
    }
}

int Solver::solve_strong(const Position &pos) {
    ZoneScoped;

    return solve(pos, pos.score_loss(), pos.score_win());
}

int Solver::solve(const Position &pos, int alpha, int beta) {
    assert(alpha < beta);

    int score = (alpha + beta) / 2;

    while (alpha < beta) {
        int window = std::max(score, alpha + 1);
        score = pool.search(pos, window - 1, window);

        if (score < window) {
            beta = score;
        } else {
            alpha = score;
        }
    }

    return score;
}

int Solver::get_best_move(const Position &pos_orig) {
    ZoneScoped;

    Position pos{pos_orig};

    assert(!pos.has_player_won());
    assert(!pos.has_opponent_won());
    assert(!pos.is_draw());

    // This method uses the results written to the t-table by the negamax function
    // to find the best move. However, the table does not store trival positions which
    // can be solved by static analysis. For these positions we need to try each move
    // to find the best move.

    int score = solve_strong(pos);

    // Check if the result is stored in the table.
    bool is_mirrored;
    board hash = pos.hash(is_mirrored);

    Entry entry = table.get(hash);
    if (entry.get_type() == NodeType::EXACT) {
        int table_move = entry.get_move(is_mirrored);

        // Validate the move stored in the table is the best move.
        board before_move = pos.move(table_move);
        int table_score = -solve(pos, -entry.get_score() - 1, -entry.get_score() + 1);
        pos.unmove(before_move);

        // The table doesn't always store the best move to play. If this is the case,
        // Try every move until we find the best move.
        if (table_score == score) {
            return table_move;
        }
    }

    // If we still have a miss, then try each move until we find a move which
    // gives the same score as the position.
    for (int move = 0; move < BOARD_WIDTH; move++) {
        if (pos.is_move_valid(move)) {
            board before_move = pos.move(move);
            int child_score = -solve(pos, -score - 1, -score + 1);
            pos.unmove(before_move);

            if (child_score == score) {
                return move;
            }
        }
    }

    // This point should never be reached.
    std::cout << "Error: could not get a best move in this position:" << std::endl;
    pos.printb();

    assert(0);
    return -1;
}

int Solver::get_principal_variation(const Position &pos, std::vector<int> &moves) {
    ZoneScoped;

    assert(moves.size() == 0);

    Position pv = Position(pos);
    while (!pv.is_game_over()) {
        int best_move = get_best_move(pv);

        moves.push_back(best_move);
        pv.move(best_move);
    }

    return static_cast<int>(moves.size());
}

void Solver::clear_state() {
    table.clear();
    pool.reset_stats();
}

std::string Solver::get_settings_string() { 
    std::stringstream result;
    result << "Using a " << BOARD_WIDTH << " x " << BOARD_HEIGHT << " board";

    result << ", a " << Table::get_table_size() << " table";
    if (ENABLE_HUGE_PAGES) {
        result << " (huge pages on)";
    }
        
    result << ", and " << NUM_THREADS << " threads";
    if (ENABLE_AFFINITY) {
        result << " (affinity on)";
    }

    result << "." << std::endl;

#ifndef NDEBUG
    result << "Running with assertions enabled." << std::endl;
#endif

    return result.str();
}
