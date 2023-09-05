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


const int TYPE_MISS = 0;
const int TYPE_UPPER = 1;
const int TYPE_LOWER = 2;
const int TYPE_EXACT = 3;


Entry::Entry() {
    data = 0;
}


Entry::Entry(board hash, int move, int type, int score) {
    // Only the partial hash needs to be stored. This is equivalent to
    // hash % 2^HASH_BITS.
    data
        = (hash << HASH_SHIFT)
        | (move << MOVE_SHIFT)
        | (type << TYPE_SHIFT)
        | (score << SCORE_SHIFT);
}


Table::Table() {
    this->table = std::shared_ptr<Entry>(new Entry[NUM_ENTRIES], std::default_delete<Entry[]>());
    this->stats = std::make_shared<Stats>();

    TracyAlloc(table.get(), NUM_ENTRIES * sizeof(Entry));

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
    Entry empty;
    std::fill(table.get(), table.get() + NUM_ENTRIES, empty);
}


void Table::prefetch(board hash) {
    ZoneScoped;

    int index = hash % NUM_ENTRIES;

    // void __builtin_prefetch(const void *addr, int rw=0, int locality=3)
    // rw       = read/write flag. 0 for read, 1 for write & read/write.
    // locality = persistance in cache.
    __builtin_prefetch(table.get() + index, 1, 2);
}


bool Table::get(board hash, bool is_mirrored, int &move, int &type, int &score) {
    ZoneScoped;

    int index = hash % NUM_ENTRIES;
    Entry entry = table[index];

    // If this state has not been seen.
    if (entry.is_empty()) {
        stats->lookup_miss();
        move = -1;
        type = TYPE_MISS;

        return false;
    }

    // If this is a hash collision.
    if (!entry.is_equal(hash)) {
        stats->lookup_collision();
        move = -1;
        type = TYPE_MISS;

        return false;
    }

    // Otherwise reconstruct the data stored against the entry.
    move = entry.get_move();
    type = entry.get_type();
    score = entry.get_score() + Position::MIN_SCORE;

    // If we looked up a mirrored move, mirror the best move as well.
    if (is_mirrored) {
        move = BOARD_WIDTH - move - 1;
    }

    stats->lookup_success();
    return true;
}


void Table::put(board hash, bool is_mirrored, int move, int type, int score) {
    ZoneScoped;

    assert(0 <= move && move <= BOARD_WIDTH);
    assert(type == TYPE_UPPER || type == TYPE_LOWER || type == TYPE_EXACT);
    assert(Position::MIN_SCORE <= score && score <= Position::MAX_SCORE);

    int index = hash % NUM_ENTRIES;
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
    table[index] = Entry(hash, move, type, score - Position::MIN_SCORE);
}


std::string Table::get_table_size() {
    std::stringstream result;
    result << std::fixed << std::setprecision(2);

    long bytes = NUM_ENTRIES * sizeof(Entry);
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
