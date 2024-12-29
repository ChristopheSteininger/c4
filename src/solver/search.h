#ifndef SEARCH_H_
#define SEARCH_H_

#include <random>

#include "position.h"
#include "table.h"
#include "util/progress.h"
#include "util/stats.h"

// Search returning this value means another thread stopped the search.
inline constexpr int SEARCH_STOPPED = 1000;

struct Node {
    Position pos{};
    float score{0.0f};

    bool did_lookup{false};
    bool is_mirrored{false};
    board hash{0};
    Entry entry{};

    Node() = default;

    Node(const Position &pos) : pos(pos) {};
};

// A single threaded search.
class Search {
   public:
    // Create our own copy of the transposition table. This table will use the same
    // underlying storage as parent_table so this thread can benefit from the work
    // other threads have saved in the table.
    Search(int id, const Table &parent_table, std::shared_ptr<Stats> stats, std::shared_ptr<Progress> progress)
        : table(parent_table, stats), stats(std::move(stats)), progress(std::move(progress)), rand(id) {}

    void start() { stop_search = false; }
    void stop() { stop_search = true; }

    int search(Position &pos, int alpha, int beta, int score_jitter);

   private:
    Table table;
    std::shared_ptr<Stats> stats;
    std::shared_ptr<Progress> progress;

    std::mt19937 rand;
    std::uniform_int_distribution<uint16_t> dist;

    bool stop_search{false};

    int negamax(Node &node, int alpha, int beta, int score_jitter) noexcept;

    void sort_moves(Position &pos, Node *children, board opponent_threats,
        int num_moves, int *moves, int score_jitter, int table_move) noexcept;
    int static_search(Node &node, int alpha, int beta, bool &is_static) noexcept;
};

#endif
