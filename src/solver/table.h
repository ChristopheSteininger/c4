#ifndef TABLE_H_
#define TABLE_H_


#include <string>
#include <memory>

#include "settings.h"
#include "position.h"
#include "stats.h"


extern const int TYPE_LOWER;
extern const int TYPE_UPPER;
extern const int TYPE_EXACT;


constexpr int const_log2(const int n) {
    return (n <= 1)
        ? 0
        : 1 + const_log2(n / 2);
}


class Entry {
public:
    Entry();
    Entry(board hash, int move, int type, int score);

    inline bool is_empty() const { return data == 0; }
    inline bool is_equal(board hash) const { return data != 0 && (hash & HASH_MASK) == get_partial_hash(); }

    inline uint64_t get_partial_hash() const { return (data >> HASH_SHIFT) & HASH_MASK; }
    inline int get_move() const { return (data >> MOVE_SHIFT) & MOVE_MASK; }
    inline int get_type() const { return (data >> TYPE_SHIFT) & TYPE_MASK; }
    inline int get_score() const { return (data >> SCORE_SHIFT) & SCORE_MASK; }

    static const int HASH_BITS = 44;

private:
    // An entry contains the following information packed in 64 bits.
    //    bits: data
    // 14 - 64: Parital hash
    // 10 - 13: Move
    //  8 -  9: Type
    //  0 -  7: Score
    uint64_t data;

    // The constants below define where information is packed into each 64 bit entry.
    static const uint64_t HASH_MASK = ((uint64_t) 1 << HASH_BITS) - 1;
    static const int HASH_SHIFT = 20;

    static const int MOVE_MASK = (1 << 4) - 1;
    static const int MOVE_SHIFT = 10;

    static const int TYPE_MASK = (1 << 2) - 1;
    static const int TYPE_SHIFT = 8;

    static const int SCORE_MASK = (1 << 8) - 1;
    static const int SCORE_SHIFT = 0;
};


class Table {
public:
    Table();
    Table(const Table &parent, const std::shared_ptr<Stats> stats);
    ~Table();

    Table(const Table &table) = delete;

    void clear();

    void prefetch(board hash);
    bool get(board hash, bool is_mirrored, int &move, int &type, int &value);
    void put(board hash, bool is_mirrored, int move, int type, int value);

    static std::string get_table_size();

private:
    // The table is shared across all threads.
    std::shared_ptr<Entry[]> table;

    // Stats are only shared with other objects on the same thread.
    std::shared_ptr<Stats> stats;

    // This table uses the Chinese Remainer Theorem to reduce the number of bits per entry.
    // For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
    // Some example prime numbers:
    //  * 131101     =  1 MB
    //  * 1048583    =  8 MB
    //  * 8388617    = 64 MB
    //  * 134217757  =  1 GB
    //  * 1073741827 =  8 GB
    static const int NUM_ENTRIES = 134217757;

    // Not all bits of the hash are saved, however the hashing will still by unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    static_assert(const_log2(NUM_ENTRIES) + Entry::HASH_BITS > (BOARD_HEIGHT + 1) * BOARD_WIDTH);
};


#endif
