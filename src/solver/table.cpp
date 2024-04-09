#include "table.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

#include "position.h"
#include "settings.h"
#include "os.h"

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
    Entry *memory = static_cast<Entry *>(allocate_huge_pages(NUM_TABLE_ENTRIES, sizeof(Entry)));
    auto memory_free = [](Entry *memory) { free_huge_pages(memory); };

    this->table = std::shared_ptr<Entry[]>(memory, memory_free);
    this->stats = std::make_shared<Stats>();

    clear();
}

Table::Table(const Table &parent, const std::shared_ptr<Stats> stats) {
    this->table = parent.table;
    this->stats = stats;
}

void Table::clear() {
    Entry empty{};
    std::fill(table.get(), table.get() + NUM_TABLE_ENTRIES, empty);
}

void Table::prefetch(board hash) const {
    assert(hash != 0);

    uint64_t index = hash % NUM_TABLE_ENTRIES;
    os_prefetch(table.get() + index);
}

Entry Table::get(board hash) const {
    assert(hash != 0);

    uint64_t index = hash % NUM_TABLE_ENTRIES;
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
    assert(hash != 0);
    assert(0 <= move && move < BOARD_WIDTH);
    assert(type == NodeType::EXACT || type == NodeType::LOWER || type == NodeType::UPPER);
    assert(Position::MIN_SCORE <= score && score <= Position::MAX_SCORE);

    uint64_t index = hash % NUM_TABLE_ENTRIES;
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

void Table::save() const {
    if (!SAVE_TABLE_FILE) {
        return;
    }

    std::filesystem::path filename = get_filename(true);
    std::ofstream file(filename, std::ios::binary);

    std::cout << "Saving table to " << filename << " . . ." << std::endl;
    for (int i = 0; i < NUM_TABLE_ENTRIES; i++) {
        file.write(reinterpret_cast<const char *>(&table[i].data), sizeof(Entry));
    }

    std::cout << "Done." << std::endl << std::endl;
}

void Table::load() {
    if (!LOAD_TABLE_FILE) {
        return;
    }

    std::filesystem::path filename = get_filename(false);
    std::ifstream file(filename, std::ios::binary);

    // Do nothing if the table cannot be found.
    if (!file) {
        std::cerr << "Failed to open the table file " << filename << ". No table will be loaded." << std::endl;
        return;
    }

    // Do nothing if the table has the wrong number of entries as this will give incorrect results.
    uintmax_t actual_size = std::filesystem::file_size(filename);
    uintmax_t expected_size = NUM_TABLE_ENTRIES * sizeof(Entry);
    if (actual_size != expected_size) {
        std::cerr << "The table file " << filename << " has size " << actual_size
            << " B, but expected " << expected_size << " B. No table will be loaded." << std::endl;
        return;
    }

    // Load the table into memory.
    std::cout << "Loading table " << filename << " . . ." << std::endl;
    for (int i = 0; i < NUM_TABLE_ENTRIES; i++) {
        uint64_t val;

        // This does not take the endianness of the machine into account. Assumes tables will be saved
        // and loaded by machines with the same endianness.
        file.read(reinterpret_cast<char *>(&val), sizeof(uint64_t));
        table[i].data = val;
    }

    std::cout << "Done." << std::endl << std::endl;
}

std::filesystem::path Table::get_filename(bool add_timestamp) const {
    std::string name = "table-" + std::to_string(BOARD_WIDTH) + "x" + std::to_string(BOARD_HEIGHT)
        + "-" + std::to_string(NUM_TABLE_ENTRIES) + "-v1";

    if (add_timestamp) {
        auto now = std::chrono::system_clock::now();
        name +=  std::format("-created-{:%y-%m-%dT%H-%M}", now);
    }

    return std::filesystem::path(name);
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
        result << kb << " KB";
    } else if (gb < 1) {
        result << mb << " MB";
    } else {
        result << gb << " GB";
    }

    return result.str();
}
