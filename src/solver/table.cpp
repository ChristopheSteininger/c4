#include "table.h"

#include <algorithm>
#include <cassert>
#include <chrono>
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
#include "os.h"

static std::filesystem::path get_table_filepath(bool add_timestamp) {
    std::stringstream result;
    result << "table-" << BOARD_WIDTH << "x" << BOARD_HEIGHT << "-" << NUM_TABLE_ENTRIES << "-v1";

    if (add_timestamp) {
        auto now = std::chrono::system_clock::now();
        auto now_c = std::chrono::system_clock::to_time_t(now);

        result << "-created-" << std::put_time(std::localtime(&now_c), "%y-%m-%dT%H-%M");
    }

    return std::filesystem::path(result.str());
}

static std::filesystem::path get_book_filepath() {
    std::string name = "book-" + std::to_string(BOARD_WIDTH) + "x" + std::to_string(BOARD_HEIGHT) + ".csv";

    return "book" / std::filesystem::path(name);
}

Entry::Entry() { data = 0; }

Entry::Entry(board hash, int move, NodeType type, int score, int work) {
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
        | (work << WORK_SHIFT)
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

int Entry::get_work() const {
    return (data >> WORK_SHIFT) & WORK_MASK;
}

Table::Table() {
    Entry *memory = static_cast<Entry *>(allocate_huge_pages(NUM_TABLE_ENTRIES, sizeof(Entry)));
    auto memory_free = [](Entry *memory) { free_huge_pages(memory); };

    this->table = std::shared_ptr<Entry[]>(memory, memory_free);
    this->stats = std::make_shared<Stats>();

    clear();
}

Table::Table(const Table &parent, std::shared_ptr<Stats> stats)
    : table(parent.table), stats(std::move(stats))
{
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

void Table::put(board hash, bool is_mirrored, int move, NodeType type, int score, unsigned long long num_nodes) {
    assert(hash != 0);
    assert(0 <= move && move < BOARD_WIDTH);
    assert(type == NodeType::EXACT || type == NodeType::LOWER || type == NodeType::UPPER);
    assert(Position::MIN_SCORE <= score && score <= Position::MAX_SCORE);
    assert(num_nodes > 0);

    // Move needs to be mirrored as well if we are storing the mirrored position.
    if (is_mirrored) {
        move = BOARD_WIDTH - move - 1;
    }

    // Overwrite the entry which required the least amount of work to compute.
    uint64_t index = hash % NUM_TABLE_ENTRIES;
    int offset = table[index].get_work() > table[index + 1].get_work();

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
    int work = num_nodes_to_work(num_nodes);
    table[index + offset] = Entry(hash, move, type, score, work);
}

void Table::save() const {
    if (!SAVE_TABLE_FILE) {
        return;
    }

    std::filesystem::path path = get_table_filepath(true);
    std::ofstream file(path, std::ios::binary);

    std::cout << "Saving table to " << path << " . . ." << std::endl;
    for (uint64_t i = 0; i < NUM_TABLE_ENTRIES; i++) {
        file.write(reinterpret_cast<const char *>(&table[i].data), sizeof(Entry));
    }

    std::cout << "Done." << std::endl << std::endl;
}

void Table::load_table_file() {
    if (!LOAD_TABLE_FILE) {
        return;
    }

    std::filesystem::path path = get_table_filepath(false);
    std::ifstream file(path, std::ios::binary);

    // Do nothing if the table cannot be found.
    if (!file) {
        std::cerr << "Failed to open the table file " << path << ". No table will be loaded." << std::endl;
        return;
    }

    // Do nothing if the table has the wrong number of entries as this will give incorrect results.
    uintmax_t actual_size = std::filesystem::file_size(path);
    uintmax_t expected_size = NUM_TABLE_ENTRIES * sizeof(Entry);
    if (actual_size != expected_size) {
        std::cerr << "The table file " << path << " has size " << actual_size
            << " B, but expected " << expected_size << " B. No table will be loaded." << std::endl;
        return;
    }

    // Load the table into memory.
    std::cout << "Loading table " << path << " . . ." << std::endl;
    for (uint64_t i = 0; i < NUM_TABLE_ENTRIES; i++) {
        uint64_t val;
        file.read(reinterpret_cast<char *>(&val), sizeof(uint64_t));

        table[i].data = val;
    }

    std::cout << "Done." << std::endl << std::endl;
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

        // Opening books will only contain moves which do not need to be mirrored.
        put(hash, false, move, NodeType::EXACT, score, Entry::WORK_MASK);
    }

    std::cout << "Done." << std::endl << std::endl;
}

int Table::num_nodes_to_work(unsigned long long num_nodes) {
    int work = 0;

    while (num_nodes > 1) {
        work++;
        num_nodes >>= 1;
    }

    return std::min(work, Entry::WORK_MASK);
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
