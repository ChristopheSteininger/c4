#ifndef TABLE_H_
#define TABLE_H_

#include <memory>
#include <string>

#include "entry.h"
#include "types.h"
#include "util/stats.h"
#include "util/writer.h"

class Table {
   public:
    Table();
    Table(const Table &parent, std::shared_ptr<Stats> stats)
        : table(parent.table), stats(std::move(stats)), table_writer(parent.table_writer) {}

    void clear();

    void prefetch(board hash) const noexcept;
    Entry get(board hash) const noexcept;
    void put(board hash, bool is_mirrored, int move, NodeType type, int value, unsigned long long num_nodes) noexcept;

    void load_table_file();
    void load_book_file();

    static std::string get_table_size();

   private:
    // The table is shared across all threads.
    std::shared_ptr<Entry[]> table;

    // Stats are only shared with other objects on the same thread.
    std::shared_ptr<Stats> stats;

    // The writer is shared across all threads and is used to save significant results.
    std::shared_ptr<Writer> table_writer;

    void store(board hash, Entry entry) noexcept;
};

#endif
