#include <iostream>
#include <iomanip>
#include <cassert>
#include <thread>
#include <vector>
#include <random>

#include "Tracy.hpp"

#include "solver.h"
#include "settings.h"
#include "position.h"
#include "table.h"
#include "order.h"


static const int INF_SCORE = 10000;


Solver::~Solver() {
    // pool.print_pool_stats();
}


int Solver::solve_weak(Position &pos, bool verbose) {
    ZoneScoped;

    int result = pool.search(pos, -1, 1);

    if (result > 0) {
        return 1;
    } else if (result < 0) {
        return -1;
    } else {
        return 0;
    }
}


int Solver::solve_strong(Position &pos, bool verbose) {
    ZoneScoped;

    return pool.search(pos, -INF_SCORE, INF_SCORE);
}


int Solver::get_best_move(Position &pos) {
   ZoneScoped;

   assert(!pos.has_player_won());
   assert(!pos.has_opponent_won());
   assert(!pos.is_draw());

   // This method uses the results written to the t-table by the negamax function
   // to find the best move. However, the table does not store trival positions which
   // can be solved by static analysis. For these positions we need to try each move
   // to find the best move.

   // Check if the result is stored in the table.
   bool is_mirrored;
   board hash = pos.hash(is_mirrored);

   int best_move, type, value;
   bool lookup_success = table.get(hash, is_mirrored, best_move, type, value);
   if (lookup_success && type == TYPE_EXACT) {
       return best_move;
   }

   // The results in the table are occasionally overrwritten. Try a search which and
   // see if this stores the result in the table.
   int score = solve_strong(pos);

   lookup_success = table.get(hash, is_mirrored, best_move, type, value);
   if (lookup_success && type == TYPE_EXACT) {
       return best_move;
   }

   // If we still have a miss, then try each move until we find a move which
   // gives the same score as the position.
   for (int move = 0; move < BOARD_WIDTH; move++) {
       if (pos.is_move_valid(move)) {
           board before_move = pos.move(move);
           int child_score = -pool.search(pos, -score, -score + 1);
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


int Solver::get_principal_variation(Position &pos, std::vector<int> &moves) {
    ZoneScoped;

    assert(moves.size() == 0);

    Position pv = Position(pos);
    while (!pv.is_game_over()) {
        int best_move = get_best_move(pv);

        moves.push_back(best_move);
        pv.move(best_move);
    }

    return moves.size();
}


int Solver::get_num_moves_prediction(Position &pos, int score) const {
    ZoneScoped;

    // Run the calculation from the perspective of the first player.
    if (pos.num_moves() & 1) {
        score *= -1;
    }

    if (score > 0) {
        return BOARD_WIDTH * BOARD_HEIGHT - (score * 2) + 1;
    } else if (score < 0) {
        return BOARD_WIDTH * BOARD_HEIGHT + ((score + 1) * 2);
    } else {
        return BOARD_WIDTH * BOARD_HEIGHT;
    }
}


void Solver::print_pv_update(Position &pos, std::vector<int> &prev_pv, std::vector<int> &curr_pv) {
    curr_pv.clear();
    get_principal_variation(pos, curr_pv);

    if (curr_pv != prev_pv) {
        std::cout << "Principal variation is:" << std::endl;
        for (int move : curr_pv) {
            std::cout << move << " ";
        }
        std::cout << std::endl << std::endl;
    } else {
        std::cout << "Principal variation is unchanged." << std::endl;
    }
}
