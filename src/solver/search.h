#ifndef SEARCH_H_
#define SEARCH_H_


#include "position.h"
#include "table.h"
#include "stats.h"


// Search returning this value means another thread stopped the search.
extern const int SEARCH_STOPPED;


// A single threaded search.
class Search {
public:
    Search(const Table &parent_table, const std::shared_ptr<Stats> stats);

    void start();
    void stop();

    int search(Position &pos, int alpha, int beta, int move_offset);

private:
    Table table;
    std::shared_ptr<Stats> stats;

    bool stop_search;

    int negamax(Position &pos, int alpha, int beta, int move_offset);

    bool evaluate(Position &pos, int col, int &static_score, float &dynamic_score);
    board get_forced_move(Position &pos, board opponent_wins, board non_losing_moves);
};


#endif
