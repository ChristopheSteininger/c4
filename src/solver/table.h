#ifndef TABLE_H_
#define TABLE_H_


#include <string>
#include <memory>

#include "position.h"
#include "stats.h"


extern const int TYPE_LOWER;
extern const int TYPE_UPPER;
extern const int TYPE_EXACT;


class Table {
public:
    Table();
    Table(const Table &parent, const std::shared_ptr<Stats> stats);
    ~Table();

    Table(const Table &table) = delete;

    void clear();

    void prefetch(board hash);
    bool get(board hash, bool is_mirrored, int &best_move, int &type, int &value);
    void put(board hash, bool is_mirrored, int best_move, int type, int value);

private:
    // The table is shared across all threads.
    std::shared_ptr<board[]> table;

    // Stats are only shared with other objects on the same thread.
    std::shared_ptr<Stats> stats;
};


std::string get_table_size();


#endif
