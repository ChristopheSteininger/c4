#include <iostream>

#include "player.h"
#include "player_solver.h"
#include "player_random.h"
#include "arena.h"


int main() {
    PlayerSolver solver = PlayerSolver();
    PlayerRandom random = PlayerRandom();

    Arena arena = Arena(&solver, &random);

    arena.play(100000);
}
