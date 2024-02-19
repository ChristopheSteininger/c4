#include "table.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iomanip>
#include <memory>
#include <sstream>
#include <cstdint>
#include <stdexcept>

#include "Tracy.hpp"
#include "position.h"
#include "settings.h"

#ifdef _MSC_VER
#include <xmmintrin.h>
#include <mmintrin.h>
#endif

Entry::Entry() { data = 0; }

Entry::Entry(board hash, int move, NodeType type, int score) {
    int int_type = static_cast<int>(type);

    // Shift so we don't store negative numbers in the table.
    int shifted_score = score - Position::MIN_SCORE;

    // Only the partial hash needs to be stored. This is equivalent to:
    // hash % 2^HASH_BITS.
    // clang-format off
    data
        = (hash << HASH_SHIFT)  
        | (move << MOVE_SHIFT)
        | (int_type << TYPE_SHIFT)
        | (shifted_score << SCORE_SHIFT);
    // clang-format on
}

int Entry::get_move(bool is_mirrored) const {
    int bits = (data >> MOVE_SHIFT) & MOVE_MASK;

    // The position may have been mirrored for the table lookup,
    // so mirror the best move if necessary.
    return (is_mirrored) ? BOARD_WIDTH - bits - 1 : bits;
}

int Entry::get_score() const {
    int bits = (data >> SCORE_SHIFT) & SCORE_MASK;

    // We don't store negative numbers in the table, so scores
    // are shifted by the minimum possible score.
    return bits + Position::MIN_SCORE;
}

NodeType Entry::get_type() const {
    int bits = (data >> TYPE_SHIFT) & TYPE_MASK;

    return static_cast<NodeType>(bits);
}

Table::Table() {
    this->table = std::shared_ptr<Entry[]>(new Entry[NUM_TABLE_ENTRIES], std::default_delete<Entry[]>());
    this->stats = std::make_shared<Stats>();

    TracyAlloc(table.get(), NUM_TABLE_ENTRIES * sizeof(Entry));

    clear();
}

Table::Table(const Table &parent, const std::shared_ptr<Stats> stats) {
    this->table = parent.table;
    this->stats = stats;
}

Table::~Table() { TracyFree(table.get()); }

void Table::clear() {
    Entry empty;
    std::fill(table.get(), table.get() + NUM_TABLE_ENTRIES, empty);
}

void Table::prefetch(board hash) {
    ZoneScoped;

    int index = hash % NUM_TABLE_ENTRIES;

#if defined(_MSC_VER)
    _mm_prefetch((const char *) (table.get() + index), _MM_HINT_T0);
#else
    // void __builtin_prefetch(const void *addr, int rw=0, int locality=3)
    // rw       = read/write flag. 0 for read, 1 for write & read/write.
    // locality = persistance in cache.
    __builtin_prefetch(table.get() + index, 1, 3);
#endif
}

Entry Table::get(board hash) {
    ZoneScoped;

    int index = hash % NUM_TABLE_ENTRIES;
    Entry entry = table[index];

    // If this state has not been seen.
    if (entry.is_empty()) {
        stats->lookup_miss();
        return Entry();
    }

    // If this is a hash collision.
    if (!entry.is_equal(hash)) {
        stats->lookup_collision();
        return Entry();
    }

    // Otherwise we have a hit.
    stats->lookup_success();
    return entry;
}

void Table::put(board hash, bool is_mirrored, int move, NodeType type, int score) {
    ZoneScoped;

    assert(0 <= move && move <= BOARD_WIDTH);
    assert(type == NodeType::EXACT || type == NodeType::LOWER || type == NodeType::UPPER);
    assert(Position::MIN_SCORE <= score && score <= Position::MAX_SCORE);

    int index = hash % NUM_TABLE_ENTRIES;
    Entry current_entry = table[index];

    // Move needs to be mirrored as well if we are storing the mirrored position.
    if (is_mirrored) {
        move = BOARD_WIDTH - move - 1;
    }

    // Update table statistics.
    if (current_entry.is_empty()) {
        stats->store_new_entry();
    } else if (current_entry.is_equal(hash)) {
        stats->store_rewrite();
    } else {
        stats->store_overwrite();
    }

    // Store.
    table[index] = Entry(hash, move, type, score);
}

std::string Table::get_table_size() {
    std::stringstream result;
    result << std::fixed << std::setprecision(2);

    uint64_t bytes = NUM_TABLE_ENTRIES * sizeof(Entry);
    double kb = bytes / 1024.0;
    double mb = kb / 1024.0;
    double gb = mb / 1024.0;

    if (kb < 1) {
        result << bytes << " B";
    } else if (mb < 1) {
        result << kb << " kB";
    } else if (gb < 1) {
        result << mb << " MB";
    } else {
        result << gb << " GB";
    }

    return result.str();
}
