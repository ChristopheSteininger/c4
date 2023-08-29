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


static int get_any_move(const Position &pos) {
    assert(!pos.is_game_over());

    for (int i = 0; i < BOARD_WIDTH; i++) {
        if (pos.is_move_valid(i)) {
            return i;
        }
    }

    assert(0);
    return -1;
}


static int get_move_from_mask(const board b) {
    assert(b != 0);

    for (int i = 0; i < BOARD_WIDTH; i++) {
        board mask = FIRST_COLUMN << (i * BOARD_HEIGHT_1);
        if (b & mask) {
            return i;
        }
    }

    assert(0);
    return -1;
}


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
   // can be solved by static analysis. For these positions we need find the best
   // move ourselves.

   // If neither player will be able to win, moves don't matter anymore.
   if (!pos.can_player_win() && !pos.can_opponent_win()) {
       return get_any_move(pos);
   }

   // If the player can win this move, then end the game.
   board player_threats = pos.find_player_threats();
   board player_wins = pos.wins_this_move(player_threats);
   if (player_wins) {
       return get_move_from_mask(player_wins);
   }

   // If the player can only move below the opponents threats, the player will lose.
   board opponent_threats = pos.find_opponent_threats();
   board non_losing_moves = pos.find_non_losing_moves(opponent_threats);
   if (non_losing_moves == 0) {
       return get_any_move(pos);
   }

   // Check if the opponent could win next move.
   board opponent_wins = pos.wins_this_move(opponent_threats);
   if (opponent_wins) {
       // If the opponent has multiple threats, then the game is lost.
       if (opponent_wins & (opponent_wins - 1)) {
           return get_any_move(pos);
       }

       // If the opponent has two threats on top of each other, then the game is also lost.
       if (!(opponent_wins & non_losing_moves)) {
           return get_any_move(pos);
       }

       // Otherwise, the opponent has only one threat, and the player must block the threat.
       else {
           return get_move_from_mask(opponent_wins);
       }
   }

   // Otherwise this is a complex position and will be stored in the table.
   bool is_mirrored;
   int best_move, type, value;
   
   board hash = pos.hash(is_mirrored);
   bool lookup_success = table.get(hash, is_mirrored, best_move, type, value);
   if (lookup_success && type == TYPE_EXACT) {
       return best_move;
   }

   // The results are occasionally overrwritten. If so start another search which
   // will write the best move into the table before returning.
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
   std::cout << "Error: could not get a best move. In this position:" << std::endl;
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
