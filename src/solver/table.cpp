#include "table.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <stdexcept>

#include "position.h"
#include "settings.h"
#include "util/os.h"

static std::filesystem::path get_table_filepath() {
    std::string name = "table-" + std::to_string(BOARD_WIDTH) + "x" + std::to_string(BOARD_HEIGHT) + ".csv";

    return "data" / std::filesystem::path(name);
}

static std::filesystem::path get_book_filepath() {
    std::string name = "book-" + std::to_string(BOARD_WIDTH) + "x" + std::to_string(BOARD_HEIGHT) + ".csv";

    return "data" / std::filesystem::path(name);
}

Table::Table() {
    // Need to allocate +1 entries since each entry can access the next entry.
    Entry *memory = static_cast<Entry *>(allocate_huge_pages(NUM_TABLE_ENTRIES + 1, sizeof(Entry)));
    auto memory_free = [](Entry *memory) { free_huge_pages(memory); };

    this->table = std::shared_ptr<Entry[]>(memory, memory_free);
    this->stats = std::make_shared<Stats>();
    
    // Create the writer which will share significant results found by any search thread.
    this->table_writer = std::make_shared<Writer>(get_table_filepath());

    // Set all entries to empty.
    clear();
}

void Table::clear() {
    Entry empty{};
    std::fill(table.get(), table.get() + NUM_TABLE_ENTRIES + 1, empty);
}

void Table::prefetch(board hash) const noexcept {
    assert(hash != 0);

    uint64_t index = static_cast<uint64_t>(hash % NUM_TABLE_ENTRIES);
    os_prefetch(table.get() + index);
}

Entry Table::get(board hash) const noexcept {
    assert(hash != 0);

    uint64_t index = static_cast<uint64_t>(hash % NUM_TABLE_ENTRIES);

    // Check if either of the two entries contain the position.
    Entry entry_1 = table[index];
    if (entry_1.is_equal(hash)) {
        stats->lookup_success();
        return entry_1;
    }

    Entry entry_2 = table[index + 1];
    if (entry_2.is_equal(hash)) {
        stats->lookup_success();
        return entry_2;
    }

    // Otherwise we don't have the position in the table.
    stats->lookup_miss();
    return Entry();
}

void Table::put(board hash, bool is_mirrored, int move, NodeType type, int score, unsigned long long num_nodes) noexcept {
    assert(hash != 0);
    assert(0 <= move && move < BOARD_WIDTH);
    assert(type == NodeType::EXACT || type == NodeType::LOWER || type == NodeType::UPPER);
    assert(Position::MIN_SCORE <= score && score <= Position::MAX_SCORE);
    assert(num_nodes > 0);

    // Move needs to be mirrored as well if we are storing the mirrored position.
    if (is_mirrored) {
        move = BOARD_WIDTH - move - 1;
    }

    // Store.
    store(hash, Entry(hash, move, type, score, num_nodes));

    // Save significant results to the table file.
    if constexpr (UPDATE_TABLE_FILE) {
        if (num_nodes > MIN_NODES_FOR_TABLE_FILE) {
            table_writer->add_line(std::to_string(hash) + ","
                + std::to_string(move) + ","
                + std::to_string(static_cast<int>(type)) + ","
                + std::to_string(score) + ","
                + std::to_string(num_nodes));
        }
    }
}

void Table::load_table_file() {
    if (!LOAD_TABLE_FILE) {
        return;
    }

    std::filesystem::path path = get_table_filepath();
    std::ifstream file(path, std::ios::binary);

    // Do nothing if the table cannot be found.
    if (!file) {
        std::cerr << "Failed to open the table file " << path << ". No table will be loaded." << std::endl;
        return;
    }

    // Load the table into memory.
    std::cout << "Loading table " << path << " . . ." << std::endl;

    int num_entries = 0;
    std::string hash_string, move_string, type_string, score_string, num_nodes_string;
    while (std::getline(file, hash_string, ',')) {
        std::getline(file, move_string, ',');
        std::getline(file, type_string, ',');
        std::getline(file, score_string, ',');
        std::getline(file, num_nodes_string);

        board hash = std::stoull(hash_string);

        Entry entry(hash,
            std::stoi(move_string),
            static_cast<NodeType>(std::stoi(type_string)),
            std::stoi(score_string),
            std::stoull(num_nodes_string));

        store(hash, entry);
        num_entries++;
    }

    std::cout << "Done. Read " << num_entries << " table entries." << std::endl << std::endl;
}

void Table::load_book_file() {
    if (!LOAD_BOOK_FILE) {
        return;
    }

    std::filesystem::path path = get_book_filepath();
    std::ifstream file(path);

    // Do nothing if the book cannot be found.
    if (!file) {
        std::cerr << "Failed to open the book file " << path << ". No book will be loaded." << std::endl;
        return;
    }

    std::string line;
    std::getline(file, line);

    // Load the book into memory.
    std::cout << "Loading book " << path << " . . ." << std::endl;
    std::string hash_string, move_string, score_string;
    while (std::getline(file, hash_string, ',')) {
        std::getline(file, move_string, ',');
        std::getline(file, score_string);

        board hash = std::stoull(hash_string);
        int move = std::stoi(move_string);
        int score = std::stoi(score_string);

        store(hash, Entry(hash, move, NodeType::EXACT, score, 1ull << 30));
    }

    std::cout << "Done." << std::endl << std::endl;
}

void Table::store(board hash, Entry entry) noexcept {
    // Overwrite the entry which required the least amount of work to compute.
    uint64_t index = static_cast<uint64_t>(hash % NUM_TABLE_ENTRIES);
    int offset = (table[index + 1].is_equal(hash) || table[index + 1].get_work() < table[index].get_work())
        && !table[index].is_equal(hash);

    Entry current = table[index + offset];

    // Update table statistics.
    if (current.is_empty()) {
        stats->store_new_entry();
    } else if (current.is_equal(hash)) {
        stats->store_rewrite();
    } else {
        stats->store_overwrite();
    }

    // Store.
    table[index + offset] = entry;
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
