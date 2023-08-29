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
const int TABLE_SIZE = 134217757;

// The number of bits of the hash stored in each entry.
static const unsigned int KEY_SIZE = 50;
static const board KEY_MASK = ((board) 1 << KEY_SIZE) - 1;

// The number of bits stored against each hash.
static const unsigned int VALUE_SIZE = 14;
static const board VALUE_MASK = (1 << VALUE_SIZE) - 1;


Table::Table() {
    // Not all bits of the hash are saved, however the hashing will still by unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    if (std::log2(TABLE_SIZE) + KEY_SIZE < BOARD_HEIGHT_1 * BOARD_WIDTH) {
        throw std::runtime_error("The table is too small which may result in invalid results. Increase table size.");
    }

    this->table = std::shared_ptr<board>(new board[TABLE_SIZE], std::default_delete<board[]>());
    this->stats = std::make_shared<Stats>();

    TracyAlloc(table.get(), TABLE_SIZE * sizeof(board));

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


bool Table::get(board hash, bool is_mirrored, int &best_move, int &type, int &value) {
    ZoneScoped;

    int index = hash % TABLE_SIZE;
    board result = table[index];

    // If this state has not been seen.
    if (result == 0) {
        stats->lookup_miss();
        return false;
    }

    // If this is a hash collision.
    if ((result >> VALUE_SIZE) != (hash & KEY_MASK)) {
        stats->lookup_collision();
        return false;
    }

    // Otherwise reconstruct the type and value of the entry.
    board entry = result & VALUE_MASK;
    best_move = entry >> 10;
    type = (entry >> 8) & 3;
    value = (entry & 255);

    // If we looked up a mirrored move, mirror the best move as well.
    if (is_mirrored) {
        best_move = BOARD_WIDTH - best_move - 1;
    }

    stats->lookup_success();
    return true;
}


void Table::put(board hash, bool is_mirrored, int best_move, int type, int value) {
    ZoneScoped;

    assert(0 <= best_move && best_move <= BOARD_WIDTH);
    assert(type == TYPE_UPPER || type == TYPE_LOWER || type == TYPE_EXACT);
    assert(0 <= value && value < (1 << 14));

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
        stats->store_new_entry();
    } else if ((current_entry >> VALUE_SIZE) == stored_hash) {
        stats->store_rewrite();
    } else {
        stats->store_overwrite();
    }

    // Store.
    table[index]
        = (stored_hash << VALUE_SIZE)
        | (best_move << 10)
        | (type << 8)
        | (value);
}


std::string get_table_size() {
    std::stringstream result;
    result << std::fixed << std::setprecision(2);

    long bytes = TABLE_SIZE * sizeof(board);
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
