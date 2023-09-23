#ifndef TABLE_H_
#define TABLE_H_

#include <memory>
#include <string>

#include "position.h"
#include "settings.h"
#include "stats.h"
#include "types.h"

constexpr int const_log2(const int n) { return (n <= 1) ? 0 : 1 + const_log2(n / 2); }

class Entry {
   public:
    Entry();
    Entry(board hash, int move, NodeType type, int score);

    inline bool is_empty() const { return data == 0; }
    inline bool is_equal(board hash) const { return data != 0 && (hash & HASH_MASK) == (data >> HASH_SHIFT); }

    int get_move(bool is_mirrored) const;
    NodeType get_type() const;
    int get_score() const;

   private:
    // An entry contains the following information packed in 64 bits.
    //    bits: data
    // 14 - 64: Parital hash
    // 10 - 13: Move
    //  8 -  9: Type
    //  0 -  7: Score
    uint64_t data{0};

    // The constants below define where information is packed into each 64 bit entry.
    static constexpr int HASH_BITS = 50;
    static constexpr uint64_t HASH_MASK = ((uint64_t)1 << HASH_BITS) - 1;
    static constexpr int HASH_SHIFT = 14;

    static constexpr int MOVE_MASK = (1 << 4) - 1;
    static constexpr int MOVE_SHIFT = 10;

    static constexpr int TYPE_MASK = (1 << 2) - 1;
    static constexpr int TYPE_SHIFT = 8;

    static constexpr int SCORE_MASK = (1 << 8) - 1;
    static constexpr int SCORE_SHIFT = 0;

    // Not all bits of the hash are saved, however the hashing will still by unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    static_assert(const_log2(NUM_TABLE_ENTRIES) + HASH_BITS > (BOARD_HEIGHT + 1) * BOARD_WIDTH);
};

class Table {
   public:
    Table();
    Table(const Table &parent, const std::shared_ptr<Stats> stats);
    ~Table();

    Table(const Table &table) = delete;

    void clear();

    void prefetch(board hash);
    Entry get(board hash);
    void put(board hash, bool is_mirrored, int move, NodeType type, int value);

    static std::string get_table_size();

   private:
    // The table is shared across all threads.
    std::shared_ptr<Entry[]> table;

    // Stats are only shared with other objects on the same thread.
    std::shared_ptr<Stats> stats;
};

#endif
