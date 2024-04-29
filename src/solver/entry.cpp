#include "entry.h"

#include <cassert>

Entry::Entry(board hash, int move, NodeType type, int score, unsigned long long num_nodes) noexcept {
    assert(0 <= move && move < BOARD_WIDTH);
    assert(type == NodeType::EXACT || type == NodeType::LOWER || type == NodeType::UPPER);
    assert(Position::MIN_SCORE <= score && score <= Position::MAX_SCORE);

    int int_type = static_cast<int>(type);

    // Shift so we don't store negative numbers in the table.
    int shifted_score = score - Position::MIN_SCORE;

    // Compress number of nodes into the space availiable.
    int work = num_nodes_to_work(num_nodes);

    // Only the partial hash needs to be stored. This is equivalent to:
    // hash % 2^HASH_BITS.
    // clang-format off
    data
        = static_cast<uint64_t>(hash << HASH_SHIFT)  
        | (move << MOVE_SHIFT)
        | (int_type << TYPE_SHIFT)
        | (work << WORK_SHIFT)
        | (shifted_score << SCORE_SHIFT);
    // clang-format on
}

int Entry::get_move(bool is_mirrored) const noexcept {
    int bits = (data >> MOVE_SHIFT) & MOVE_MASK;

    // The position may have been mirrored for the table lookup,
    // so mirror the best move if necessary.
    return (is_mirrored) ? BOARD_WIDTH - bits - 1 : bits;
}

int Entry::get_score() const noexcept {
    int bits = (data >> SCORE_SHIFT) & SCORE_MASK;

    // We don't store negative numbers in the table, so scores
    // are shifted by the minimum possible score.
    return bits + Position::MIN_SCORE;
}

NodeType Entry::get_type() const noexcept {
    int bits = (data >> TYPE_SHIFT) & TYPE_MASK;

    return static_cast<NodeType>(bits);
}

int Entry::get_work() const noexcept { return (data >> WORK_SHIFT) & WORK_MASK; }

int Entry::num_nodes_to_work(unsigned long long num_nodes) const noexcept {
    int work = 0;

    while (num_nodes > 1) {
        work++;
        num_nodes >>= 3;
    }

    return std::min(work, WORK_MASK);
}
