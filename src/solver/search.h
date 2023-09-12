#ifndef SEARCH_H_
#define SEARCH_H_


#include "position.h"
#include "table.h"
#include "stats.h"


// Search returning this value means another thread stopped the search.
extern const int SEARCH_STOPPED;


struct Node {
    Position pos;

    bool did_lookup;
    int table_move;
    board hash;
    bool is_mirrored;

    float dynamic_score;

    Node() {}

    Node(const Position &pos) {
        this->pos = Position(pos);

        this->did_lookup = false;
        this->table_move = -1;
    }
};


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

    int negamax(Node &node, int alpha, int beta, int move_offset);

    bool static_search(Node &node, int col, int &alpha, int &beta);
    board get_forced_move(Position &pos, board opponent_wins, board non_losing_moves);
};


#endif
