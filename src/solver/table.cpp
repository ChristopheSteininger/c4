#include <stdexcept>
#include <cassert>
#include <algorithm>
#include <cmath>

#include "table.h"
#include "settings.h"
#include "position.h"


const int TYPE_UPPER = 1;
const int TYPE_LOWER = 2;
const int TYPE_EXACT = 3;

// This table uses the Chineese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
// Some example prime numbers:
//  * 8388617    = 64 MB
//  * 134217757  = 1 GB
//  * 1073741827 = 8 GB
static const unsigned int TABLE_SIZE = 134217757;

// The number of bits of the hash stored in each entry.
static const unsigned int KEY_SIZE = 56;
static const board KEY_MASK = ((board) 1 << KEY_SIZE) - 1;

// The number of bits stored against each hash.
static const unsigned int VALUE_SIZE = 8;
static const board VALUE_MASK = (1 << VALUE_SIZE) - 1;


Table::Table() {
    // Not all bits of the hash are saved, however the hashing will still by unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    if (std::log2(TABLE_SIZE) + KEY_SIZE < BOARD_HEIGHT_1 * BOARD_WIDTH) {
        throw std::runtime_error("The table is too small which may result in invalid results. Increase table size.");
    }

    table = new board[TABLE_SIZE]();
}


Table::~Table() {
    delete [] table;
}


void Table::clear() {
    std::fill(table, table + TABLE_SIZE, 0);

    stat_num_lookups = 0;
    stat_num_successful_lookups = 0;
    stat_num_hash_collisions = 0;

    stat_num_entries = 0;
    stat_num_overwrites = 0;
    stat_num_rewrites = 0;
}


int Table::get(Position &pos, int &best_move, int &type, int &value) {
    stat_num_lookups++;
    
    bool is_mirrored;
    board hash = pos.hash(is_mirrored);

    int index = hash % TABLE_SIZE;
    board result = table[index];

    // If this state has not been seen.
    if (result == 0) {
        return 0;
    }

    // If this is a hash collision.
    if ((result >> VALUE_SIZE) != (hash & KEY_MASK)) {
        stat_num_hash_collisions++;
        return 0;
    }

    // Otherwise reconstruct the type and value of the entry.
    board entry = result & VALUE_MASK;
    best_move = entry >> 4;
    type = (entry >> 2) & 3;
    value = (entry & 3) - 1;

    // If we looked up a mirrored move, mirror the best move as well.
    if (is_mirrored) {
        best_move = BOARD_WIDTH - best_move - 1;
    }

    stat_num_successful_lookups++;
    return 1;
}


void Table::put(Position &pos, int best_move, int type, int value) {
    assert(best_move == BOARD_WIDTH || pos.is_move_valid(best_move));
    assert(type == TYPE_UPPER || type == TYPE_LOWER || type == TYPE_EXACT);
    assert(-1 <= value && value <= 1);

    bool is_mirrored;
    board hash = pos.hash(is_mirrored);
    
    int index = hash % TABLE_SIZE;
    board current_entry = table[index];

    // Only the partial hash needs to be stored. This is equivalent to
    // hash % 2^KEY_SIZE.
    board stored_hash = hash & KEY_MASK;

    // Best move needs to be mirrored as well if we are storing the mirrored position.
    if (is_mirrored) {
        best_move = BOARD_WIDTH - best_move - 1;
    }

    // Update table statistics.
    if (current_entry == 0) {
        stat_num_entries++;
    } else if ((current_entry >> VALUE_SIZE) == stored_hash) {
        stat_num_rewrites++;
    } else {
        stat_num_overwrites++;
    }

    // Store.
    table[index]
        = (stored_hash << VALUE_SIZE)
        | (best_move << 4)
        | (type << 2)
        | (value + 1);
}


double Table::get_size_in_gigabytes() const {
    return (double) TABLE_SIZE * sizeof(board) / 1024 / 1024 / 1024;
}


double Table::get_hit_rate() const {
    return (double) stat_num_successful_lookups / stat_num_lookups;
}


double Table::get_collision_rate() const {
    return (double) stat_num_hash_collisions / stat_num_lookups;
}


double Table::get_density() const {
    return (double) stat_num_entries / TABLE_SIZE;
}


double Table::get_overwrite_rate() const {
    return (double) stat_num_overwrites / (stat_num_entries + stat_num_rewrites + stat_num_overwrites);
}


double Table::get_rewrite_rate() const {
    return (double) stat_num_rewrites / (stat_num_entries + stat_num_rewrites + stat_num_overwrites);
}