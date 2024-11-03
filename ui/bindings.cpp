#include <emscripten/bind.h>
#include <thread>

#include "../src/solver/position.h"
#include "../src/solver/solver.h"
#include "../src/solver/types.h"

using namespace emscripten;

extern "C" {
    extern void solve_callback(int score);
}

// We cannot block the UI thread with a solve, as this can take a long time.
// So spawn a separate thread that will wait for the result and then invoke
// the callback.
void solve_async(Solver &solver, const Position &pos) {
    std::thread([&solver, pos]() {
        int score = solver.solve_strong(pos);

        solve_callback(score);
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
        .function("get_best_move", &Solver::get_best_move)
        .class_function("get_settings_string", &Solver::get_settings_string)
        ;

    class_<Position>("Position")
        .constructor<>()
        .function("move", select_overload<board(int)>(&Position::move))
        .function("unmove", select_overload<void(int)>(&Position::unmove))
        .function("num_moves", &Position::num_moves)
        .function("is_game_over", &Position::is_game_over)
        .function("is_move_valid", &Position::is_move_valid)
        .function("moves_left", &Position::moves_left)
        .function("get_player", &Position::get_player)
        ;
}
