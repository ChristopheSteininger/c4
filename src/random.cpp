#include <iostream>
#include <random>

#include "solver/position.h"
#include "solver/solver.h"


static inline constexpr int NUM_GAMES = 1000;

// Range of moves play in each generated game. Min is inclusive, max is exclusive.
static inline constexpr int MIN_MOVES = 29;
static inline constexpr int MAX_MOVES = BOARD_WIDTH * BOARD_HEIGHT;

// Range of moves left until the end of the game if all moves are optimal. Min is inclusive, max is exclusive.
static inline constexpr int MIN_MOVES_LEFT = 3;
static inline constexpr int MAX_MOVES_LEFT = 14;

// Maximum number of random moves to try before giving up.
static inline constexpr int MAX_ATTEMPTS_PER_MOVE = 10;


static std::mt19937 engine{0};
static std::uniform_int_distribution<uint16_t> dist{};

static Solver solver{};


static void print_game(const Position &pos, int moves[], int score) {
    int moves_left = pos.score_to_last_move(score) - pos.num_moves();

    assert(MIN_MOVES <= pos.num_moves() && pos.num_moves() < MAX_MOVES);
    assert(MIN_MOVES_LEFT <= moves_left && moves_left < MAX_MOVES_LEFT);
    assert(!pos.is_game_over());

    for (int i = 0; i < pos.num_moves(); i++) {
        assert(0 <= moves[i] && moves[i] < BOARD_WIDTH);

        std::cout << (moves[i] + 1);
    }

    std::cout << " " << score << std::endl;
}

static bool is_game_close_to_end(const Position& pos) {
    if (pos.num_moves() < 7) {
        return false;
    }

    int loss = pos.score_loss();
    int win = pos.score_win();

    // Do not play any move where we can force a win in the next turn.
    // Similarly, do not play any move where the opponent can force a win
    // in one turn.
    return solver.solve(pos, win - 1, win) >= win || solver.solve(pos, loss, loss + 1) <= loss;
}

static int play_random_move(Position& pos) {
    for (int i = 0; i < MAX_ATTEMPTS_PER_MOVE; i++) {
        int move = dist(engine) % BOARD_WIDTH;
        
        if (pos.is_move_valid(move)) {
            board before_move = pos.move(move);
            if (is_game_close_to_end(pos)) {
                pos.unmove(before_move);
            } else {
                return move;
            }
        }
    }

    return -1;
}

static bool try_random_game() {
    Position pos{};
    
    int num_moves = (dist(engine) % (MAX_MOVES - MIN_MOVES)) + MIN_MOVES;
    int moves[BOARD_WIDTH * BOARD_HEIGHT];

    for (int i = 0; i < num_moves; i++) {
        int move = play_random_move(pos);
        
        // Cannot keep playing if we could not find a move to play.
        if (move == -1) {
            return false;
        }

        moves[i] = move;
    }

    // Check that the game has the right level of complexity.
    int score = solver.solve_strong(pos);
    int moves_left = pos.score_to_last_move(score) - num_moves;

    if (MIN_MOVES_LEFT <= moves_left && moves_left < MAX_MOVES_LEFT) {
        print_game(pos, moves, score);
        return true;
    }

    return false;
}

int main() {
    std::cout.imbue(std::locale(""));
    std::cout << solver.get_settings_string();
    std::cout << "Generating " << NUM_GAMES << " random games on " << BOARD_WIDTH << " x " << BOARD_HEIGHT << " boards." << std::endl;
    std::cout << std::endl;

    for (int i = 0; i < NUM_GAMES; i++) {
        while (!try_random_game());
    }

    return 0;
}
