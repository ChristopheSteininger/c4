#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <memory>

#include "Tracy.hpp"

#include "table.h"
#include "settings.h"
#include "position.h"


const int TYPE_UPPER = 1;
const int TYPE_LOWER = 2;
const int TYPE_EXACT = 3;

// This table uses the Chinese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
// Some example prime numbers:
//  * 131101     =  1 MB
//  * 1048583    =  8 MB
//  * 8388617    = 64 MB
//  * 134217757  =  1 GB
//  * 1073741827 =  8 GB
const int TABLE_SIZE = 1073741827;

// An entry contains the following information packed in 64 bits.
//    bits: data
// 14 - 64: Parital hash
// 10 - 13: Move
//  8 -  9: Type
//  0 -  7: Score
typedef uint64_t entry_t;

static const int HASH_BITS = 50;
static const int HASH_SHIFT = 14;
static const entry_t HASH_MASK = ((entry_t) 1 << HASH_BITS) - 1;

static const int MOVE_BITS = 4;
static const int MOVE_SHIFT = 10;

static const int TYPE_BITS = 2;
static const int TYPE_SHIFT = 8;

static const int SCORE_BITS = 8;
static const int SCORE_SHIFT = 0;


inline static entry_t unpack(const entry_t entry, const int bits, const int shift) {
    entry_t mask = ((entry_t) 1 << bits) - 1;

    return (entry >> shift) & mask;
}


inline static entry_t pack(entry_t &entry, const int bits, const int shift, const entry_t data) {
    entry_t mask = ((entry_t) 1 << bits) - 1;
    assert(0 <= data && data <= mask);
    assert((entry & mask) == 0);

    return entry | (data << shift);
}


Table::Table() {
    // Not all bits of the hash are saved, however the hashing will still by unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    if (std::log2(TABLE_SIZE) + HASH_BITS < BOARD_HEIGHT_1 * BOARD_WIDTH) {
        throw std::runtime_error("The table is too small which may result in invalid results. Increase table size.");
    }

    this->table = std::shared_ptr<entry_t>(new entry_t[TABLE_SIZE], std::default_delete<entry_t[]>());
    this->stats = std::make_shared<Stats>();

    TracyAlloc(table.get(), TABLE_SIZE * sizeof(entry_t));

    clear();
}


Table::Table(const Table &parent, const std::shared_ptr<Stats> stats) {
    this->table = parent.table;
    this->stats = stats;
}


Table::~Table() {
    TracyFree(table.get());
}


void Table::clear() {
    std::fill(table.get(), table.get() + TABLE_SIZE, 0);
}


void Table::prefetch(board hash) {
    ZoneScoped;

    int index = hash % TABLE_SIZE;

    // void __builtin_prefetch(const void *addr, int rw=0, int locality=3)
    // rw       = read/write flag. 0 for read, 1 for write & read/write.
    // locality = persistance in cache.
    __builtin_prefetch(table.get() + index, 1, 2);
}


bool Table::get(board hash, bool is_mirrored, int &move, int &type, int &score) {
    ZoneScoped;

    int index = hash % TABLE_SIZE;
    entry_t entry = table[index];

    // If this state has not been seen.
    if (entry == 0) {
        stats->lookup_miss();
        return false;
    }

    // If this is a hash collision.
    entry_t partial_hash = hash & HASH_MASK;
    if (partial_hash != unpack(entry, HASH_BITS, HASH_SHIFT)) {
        stats->lookup_collision();
        return false;
    }

    // Otherwise reconstruct the data stored against the entry.
    move = unpack(entry, MOVE_BITS, MOVE_SHIFT);
    type = unpack(entry, TYPE_BITS, TYPE_SHIFT);
    score = unpack(entry, SCORE_BITS, SCORE_SHIFT);

    // If we looked up a mirrored move, mirror the best move as well.
    if (is_mirrored) {
        move = BOARD_WIDTH - move - 1;
    }

    stats->lookup_success();
    return true;
}


void Table::put(board hash, bool is_mirrored, int best_move, int type, int score) {
    ZoneScoped;

    assert(0 <= best_move && best_move <= BOARD_WIDTH);
    assert(type == TYPE_UPPER || type == TYPE_LOWER || type == TYPE_EXACT);
    assert(0 <= score && score < (1 << 14));

    int index = hash % TABLE_SIZE;
    entry_t current_entry = table[index];

    // Only the partial hash needs to be stored. This is equivalent to
    // hash % 2^HASH_BITS.
    entry_t partial_hash = hash & HASH_MASK;

    // Best move needs to be mirrored as well if we are storing the mirrored position.
    if (is_mirrored) {
        best_move = BOARD_WIDTH - best_move - 1;
    }

    // Update table statistics.
    if (current_entry == 0) {
        stats->store_new_entry();
    } else if (partial_hash == unpack(current_entry, HASH_BITS, HASH_SHIFT)) {
        stats->store_rewrite();
    } else {
        stats->store_overwrite();
    }

    entry_t entry = 0;
    entry = pack(entry, HASH_BITS, HASH_SHIFT, partial_hash);
    entry = pack(entry, MOVE_BITS, MOVE_SHIFT, best_move);
    entry = pack(entry, TYPE_BITS, TYPE_SHIFT, type);
    entry = pack(entry, SCORE_BITS, SCORE_SHIFT, score);

    // Store.
    table[index] = entry;
}


std::string get_table_size() {
    std::stringstream result;
    result << std::fixed << std::setprecision(2);

    long bytes = TABLE_SIZE * sizeof(entry_t);
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
