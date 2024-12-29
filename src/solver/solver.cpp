#include "solver.h"

#include <cassert>
#include <iomanip>
#include <iostream>
#include <random>
#include <string>
#include <sstream>
#include <thread>
#include <vector>

#include "position.h"
#include "settings.h"
#include "table.h"

Solver::Solver()
    : table(), pool(table, progress) {
    table.load_table_file();
    table.load_book_file();
}

Solver::Solver(const Solver& solver)
    : table(solver.table, std::make_shared<Stats>()), pool(table, progress) {
}

int Solver::solve_weak(const Position &pos) {
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
    return solve(pos, Position::MIN_SCORE, Position::MAX_SCORE);
}

int Solver::solve(const Position &pos, int lower, int upper) {
    assert(lower < upper);

    // Check if the game is already over before launching the full search.
    if (pos.has_opponent_won()) {
        return pos.score_loss(0);
    }
    if (pos.has_player_won()) {
        return pos.score_win(-1);
    }
    if (pos.is_draw()) {
        return 0;
    }
    if (pos.wins_this_move(pos.find_player_threats())) {
        return pos.score_win();
    }

    int min_score = std::max(pos.score_loss(), Position::MIN_SCORE);
    int max_score = std::min(pos.score_win(), Position::MAX_SCORE);

    // If the bounds of the search are beyond the best or worst possible
    // scores in this position, then immediately return.
    if (upper <= min_score) {
        return min_score;
    }
    if (lower >= max_score) {
        return max_score;
    }

    int alpha = std::max(lower, min_score);
    int beta = std::min(upper, max_score);
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

int Solver::get_best_move(const Position &pos_orig, int score) {
    assert(!pos_orig.is_game_over());

    Position pos{pos_orig};

    // This method uses the results written to the t-table by the negamax function
    // to find the best move. However, the table does not store trival positions which
    // can be solved by static analysis. For these positions we need to try each move
    // to find the best move.

    // Check if the result is stored in the table.
    bool is_mirrored;
    board hash = pos.hash(is_mirrored);

    Entry entry = table.get(hash);
    if (entry.get_type() != NodeType::MISS) {
        int table_move = entry.get_move(is_mirrored);

        // Validate the move stored in the table is the best move.
        board before_move = pos.move(table_move);
        int table_score = -solve(pos, -score, -score + 1);
        pos.unmove(before_move);

        // The table doesn't always store the best move to play. If this is the case,
        // Try every move until we find the best move.
        if (table_score >= score) {
            return table_move;
        }
    }

    // If we still have a miss, then try each move until we find a move which
    // gives the same score as the position.
    for (int move = 0; move < BOARD_WIDTH; move++) {
        if (pos.is_move_valid(move)) {
            board before_move = pos.move(move);
            int child_score = -solve(pos, -score, -score + 1);
            pos.unmove(before_move);

            if (child_score >= score) {
                return move;
            }
        }
    }

    // This point should never be reached.
    std::cout << "Error: could not get a best move in this position:" << std::endl
              << pos.display_board();

    assert(0);
    return -1;
}

int Solver::get_principal_variation(const Position &pos, std::vector<int> &moves) {
    assert(moves.size() == 0);

    int score = solve_strong(pos);

    Position pv = Position(pos);
    while (!pv.is_game_over()) {
        int best_move = get_best_move(pv, score);

        moves.push_back(best_move);
        pv.move(best_move);

        score = -score;
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
        
    result << ", and " << pool.get_num_workers() << " threads";
    if (ENABLE_AFFINITY) {
        result << " (affinity on)";
    }

    result << "." << std::endl;

#ifndef NDEBUG
    result << "Running with assertions enabled." << std::endl;
#endif

    return result.str();
}
