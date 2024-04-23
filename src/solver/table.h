#ifndef TABLE_H_
#define TABLE_H_

#include <filesystem>
#include <memory>
#include <string>

#include "position.h"
#include "settings.h"
#include "stats.h"
#include "types.h"
#include "writer.h"

constexpr uint64_t const_log2(const uint64_t n) { return (n <= 1) ? 0 : 1 + const_log2(n / 2); }

class Entry {
    friend class Table;

   public:
    Entry();
    Entry(board hash, int move, NodeType type, int score, int work);

    inline bool is_empty() const { return data == 0; }
    inline bool is_equal(board hash) const { return data != 0 && (hash & HASH_MASK) == (data >> HASH_SHIFT); }

    int get_move(bool is_mirrored) const;
    int get_score() const;
    NodeType get_type() const;
    int get_work() const;

   private:
    // An entry contains the following information packed in 64 bits.
    //    bits: data
    //  0 -  6: Score
    //  7 -  8: Type
    //  9 - 12: Move
    // 13 - 17: Work
    // 18 - 64: Parital hash
    uint64_t data{0};

    // The constants below define where information is packed into each 64 bit entry.
    static constexpr int SCORE_BITS = 7;
    static constexpr int SCORE_MASK = (1 << SCORE_BITS) - 1;
    static constexpr int SCORE_SHIFT = 0;

    static constexpr int TYPE_BITS = 2;
    static constexpr int TYPE_MASK = (1 << TYPE_BITS) - 1;
    static constexpr int TYPE_SHIFT = SCORE_BITS;

    static constexpr int MOVE_BITS = 4;
    static constexpr int MOVE_MASK = (1 << MOVE_BITS) - 1;
    static constexpr int MOVE_SHIFT = TYPE_SHIFT + TYPE_BITS;

    static constexpr int WORK_BITS = 5;
    static constexpr int WORK_MASK = (1 << WORK_BITS) - 1;
    static constexpr int WORK_SHIFT = MOVE_SHIFT + MOVE_BITS;

    static constexpr int HASH_BITS = 8 * sizeof(data) - WORK_SHIFT - WORK_BITS;
    static constexpr uint64_t HASH_MASK = ((uint64_t)1 << HASH_BITS) - 1;
    static constexpr int HASH_SHIFT = WORK_SHIFT + WORK_BITS;

    // Not all bits of the hash are saved, however the hashing will still be unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    static_assert(const_log2(NUM_TABLE_ENTRIES) + HASH_BITS > (BOARD_HEIGHT + 1) * BOARD_WIDTH);

    // The number of entries must be odd otherwise CRT does not apply.
    static_assert(NUM_TABLE_ENTRIES % 2 == 1);

    // Move bits must be wide enough to store any valid move.
    static_assert((1 << MOVE_BITS) >= BOARD_WIDTH);

    // Score bits must be wide enough to store the entire range of possible scores.
    static_assert((1 << SCORE_BITS) > Position::MAX_SCORE - Position::MIN_SCORE);
};

class Table {
   public:
    Table();
    Table(const Table &parent, std::shared_ptr<Stats> stats)
        : table(parent.table), stats(std::move(stats)), table_writer(parent.table_writer) {}

    void clear();

    void prefetch(board hash) const;
    Entry get(board hash) const;
    void put(board hash, bool is_mirrored, int move, NodeType type, int value, unsigned long long num_nodes);

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

    void store(board hash, Entry entry);
    int num_nodes_to_work(unsigned long long num_nodes) const;
};

#endif
