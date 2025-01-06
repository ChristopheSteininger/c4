#include <emscripten/bind.h>
#include <thread>

#include "../src/solver/position.h"
#include "../src/solver/solver.h"
#include "../src/solver/types.h"

using namespace emscripten;

// Functions implemented on JS side, and called by C++ side.
extern "C" {
    extern void solve_callback(int score, int best_move, int moves_left);

    extern void win_callback(int score);

    extern void cancelled_callback();
}

// We cannot block the UI thread with a solve, as this can take a long time.
// So spawn a separate thread that will wait for the result and then invoke
// the callback.
void solve_async(Solver &solver, const Position &pos) {
    std::thread([&solver, pos]() {
        int score = solver.solve_strong(pos);

        // The score returned will indicate either a game over, a cancelled solve,
        // or a successfully solved position. Invoke the correct callback.
        if (pos.is_game_over()) {
            win_callback(score);
        } else if (Position::MIN_SCORE <= score && score <= Position::MAX_SCORE) {
            int best_move = solver.get_best_move(pos, score);
            int moves_left = pos.moves_left(score);

            solve_callback(score, best_move, moves_left);
        } else {
            cancelled_callback();
        }
    }).detach();
}

EMSCRIPTEN_BINDINGS(solver_module) {
    // 64 bit numbers should already be registered as a BitInt.
    if (IS_128_BIT_BOARD) {
        register_type<board>("BigInt");
    }

    function("solve_async", &solve_async);
    class_<Solver>("Solver")
        .constructor<>()
        .function("cancel", &Solver::cancel)
        .function("get_settings_string", &Solver::get_settings_string)
        ;

    class_<Position>("Position")
        .constructor<>()
        .function("move", select_overload<board(int)>(&Position::move))
        .function("unmove", select_overload<void(int)>(&Position::unmove))
        .function("num_moves", &Position::num_moves)
        .function("is_game_over", &Position::is_game_over)
        .function("is_move_valid", &Position::is_move_valid)
        .function("get_player", &Position::get_player)
        ;
}
