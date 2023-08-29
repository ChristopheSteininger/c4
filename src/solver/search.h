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

    int search(Position &pos, int alpha, int beta, int move_offset);

    void start();
    void stop();

private:
    Table table;
    std::shared_ptr<Stats> stats;

    bool stop_search;

    int negamax(Position &pos, int alpha, int beta, int move_offset);
};


#endif
