#ifndef SEARCH_H_
#define SEARCH_H_

#include "position.h"
#include "stats.h"
#include "table.h"

// Search returning this value means another thread stopped the search.
inline constexpr int SEARCH_STOPPED = 1000;

struct Node {
    Position pos{};
    float score{0.0f};

    bool did_lookup{false};
    bool is_mirrored{false};
    int table_move{-1};
    board hash{0};

    Node() = default;

    Node(const Position &pos) : pos(pos) {};
};

// A single threaded search.
class Search {
   public:
    // Create our own copy of the transposition table. This table will use the same
    // underlying storage as parent_table so this thread can benefit from the work
    // other threads have saved in the table.
    Search(const Table &parent_table, const std::shared_ptr<Stats> &stats)
        : table(parent_table, stats), stats(stats) {}

    void start() { stop_search = false; }
    void stop() { stop_search = true; }

    int search(Position &pos, int alpha, int beta, int move_offset);

   private:
    Table table;
    std::shared_ptr<Stats> stats;

    bool stop_search{false};

    int negamax(Node &node, int alpha, int beta, int move_offset);

    int static_search(Node &node, int col, int alpha, int beta, bool &is_static);
    board get_forced_move(board opponent_wins, board non_losing_moves);
};

#endif
