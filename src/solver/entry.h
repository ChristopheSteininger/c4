#ifndef ENTRY_H_
#define ENTRY_H_

#include <cstdint>

#include "position.h"
#include "settings.h"

constexpr uint64_t const_log2(const uint64_t n) { return (n <= 1) ? 0 : 1 + const_log2(n / 2); }

// Defines a single entry in the transposition table.
class Entry {
   public:
    Entry() noexcept {};
    Entry(board hash, int move, NodeType type, int score, unsigned long long num_nodes) noexcept;

    inline bool is_empty() const noexcept { return data == 0; }
    inline bool is_equal(board hash) const noexcept { return data != 0 && (hash & HASH_MASK) == (data >> HASH_SHIFT); }

    int get_move(bool is_mirrored) const noexcept;
    int get_score() const noexcept;
    NodeType get_type() const noexcept;
    int get_work() const noexcept;

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
    
    int num_nodes_to_work(unsigned long long num_nodes) const noexcept;
};

#endif
