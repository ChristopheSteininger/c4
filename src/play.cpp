/*

Use this program to play games of Connect 4 against the solver.
The program will show the optimal moves to play in any position, as well as the
outcome of the game if both players play perfectly.

*/

#include <iostream>
#include <string>

#include "solver/position.h"
#include "solver/settings.h"
#include "solver/solver.h"

static constexpr int BACK_MOVE_FLAG = -1;
static constexpr int RESET_FLAG = -2;

static const char* get_current_player_stone(const Position &pos) {
    return (pos.num_moves() & 1) == 0
        ? Position::P0_STONE
        : Position::P1_STONE;
}

static const char *get_next_player_stone(const Position &pos) {
    return (pos.num_moves() & 1) == 0
        ? Position::P1_STONE
        : Position::P0_STONE;
}

static int string_to_col(const std::string &str) {
    try {
        return std::stoi(str);
    } catch (std::exception const &) {
        return -1;
    }
}

static int get_move(const Position &pos) {
    while (true) {
        // Print a prompt.
        if (pos.is_game_over()) {
            std::cout << "Game over. Type \"b\" to go back, or \"r\" to reset > ";
        } else {
            std::cout << get_current_player_stone(pos) << "'s move > ";
        }

        std::string input;
        std::cin >> input;

        // Check if the given move is valid.
        int col = string_to_col(input);
        if (!pos.is_game_over() && 0 <= col && col < BOARD_WIDTH
                && pos.is_move_valid(col)) {
            return col;
        }

        // If the user wants to undo a move.
        if (pos.num_moves() > 0 && input == "b") {
            return BACK_MOVE_FLAG;
        }

        // If the user wants to reset the board.
        if (pos.num_moves() > 0 && input == "r") {
            return RESET_FLAG;
        }

        std::cout << "Invalid move." << std::endl;
    }
}

static void print_best_moves(Solver &solver, Position &pos, int score) {
    // Draw a ^ under the optimal columns to play in.
    for (int i = 0; i < BOARD_WIDTH; i++) {
        bool is_optimal_move = false;

        if (pos.is_move_valid(i)) {
            board before_move = pos.move(i);
            int move_score = -solver.solve(pos, -score, -score + 1);
            pos.unmove(before_move);

            is_optimal_move = move_score >= score;
        }

        std::cout << (is_optimal_move ? " ^" : "  ");
    }

    std::cout << std::endl;
}

static void print_score(const Position &pos, int score) {
    std::cout << std::endl << "Eval: ";

    int last_move = pos.num_moves() + pos.moves_left(score);
    if (score < 0) {
        std::cout << get_next_player_stone(pos) << " will win on move #" << last_move;
    } else if (score > 0) {
        std::cout << get_current_player_stone(pos) << " will win on move #" << last_move;
    } else {
        std::cout << "Draw";
    }

    std::cout << "." << std::endl << std::endl;
}

static void print_result(const Position &pos) {
    std::cout << std::endl << "Game over: ";

    if (pos.has_player_won()) {
        std::cout << get_current_player_stone(pos) << " won on move #" << pos.num_moves();
    } else if (pos.has_opponent_won()) {
        std::cout << get_next_player_stone(pos) << " won on move #" << pos.num_moves();
    } else {
        std::cout << "Draw";
    }

    std::cout << "!" << std::endl << std::endl;
}

int main() {
    Position pos{};
    Solver solver{};

    board before_moves[BOARD_WIDTH * BOARD_HEIGHT];

    std::cout << Solver::get_settings_string()
              << "The optimal moves will be indicated by a \"^\" under the column." << std::endl
              << std::endl;

    while (true) {
        int num_moves = pos.num_moves();

        // Print the current game state, and the evaluation of the position.
        std::cout << "Move #" << num_moves << ":" << std::endl
                  << pos.display_board();

        if (pos.is_game_over()) {
            print_result(pos);
        } else {
            int score = solver.solve_strong(pos);
            print_best_moves(solver, pos, score);
            print_score(pos, score);
        }

        int move = get_move(pos);

        // Move, unmove, or reset.
        if (move == BACK_MOVE_FLAG) {
            pos.unmove(before_moves[num_moves - 1]);
        } else if (move == RESET_FLAG) {
            pos = Position();
        } else {
            before_moves[num_moves] = pos.move(move);
        }

        std::cout << "==============="
                  << std::endl << std::endl << std::endl;
    }

    return 0;
}
