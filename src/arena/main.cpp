#include <iostream>

#include "arena.h"
#include "player.h"
#include "player_random.h"
#include "player_solver.h"

int main() {
    PlayerSolver solver = PlayerSolver();
    PlayerRandom random = PlayerRandom();

    Arena arena = Arena(&solver, &random);

    arena.play(100000);
}
