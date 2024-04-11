#include "stats.h"

#include <cassert>
#include <iomanip>
#include <sstream>

void Stats::merge(const Stats &other) {
    this->search_time_ms += other.search_time_ms;
    this->num_nodes += other.num_nodes;
    this->num_best_moves_guessed += other.num_best_moves_guessed;
    this->num_worst_moves_guessed += other.num_worst_moves_guessed;

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        this->num_exact_nodes[i] += other.num_exact_nodes[i];
        this->num_lower_nodes[i] += other.num_lower_nodes[i];
        this->num_upper_nodes[i] += other.num_upper_nodes[i];
    }

    this->num_lookup_success += other.num_lookup_success;
    this->num_lookup_miss += other.num_lookup_miss;

    this->num_store_entries += other.num_store_entries;
    this->num_store_overwrites += other.num_store_overwrites;
    this->num_store_rewrites += other.num_store_rewrites;
}

void Stats::reset() {
    search_time_ms = 0;
    num_nodes = 0;
    num_best_moves_guessed = 0;
    num_worst_moves_guessed = 0;

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        num_exact_nodes[i] = 0;
        num_lower_nodes[i] = 0;
        num_upper_nodes[i] = 0;
    }

    num_lookup_success = 0;
    num_lookup_miss = 0;

    num_store_entries = 0;
    num_store_overwrites = 0;
    num_store_rewrites = 0;
}

void Stats::completed_search(std::chrono::steady_clock::time_point search_start_time) {
    assert(this->search_time_ms == 0);

    auto duration = std::chrono::steady_clock::now() - search_start_time;
    this->search_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void Stats::new_interior_node(NodeType type, int num_moves) {
    switch (type) {
        case NodeType::EXACT:
            num_exact_nodes[num_moves]++;
            break;

        case NodeType::LOWER:
            num_lower_nodes[num_moves]++;
            break;

        case NodeType::UPPER:
            num_upper_nodes[num_moves]++;
            break;

        default:
            assert(0);
    }
}

std::string Stats::display_all_stats() const {
    std::stringstream result;

    result.imbue(std::locale(""));
    result << std::fixed << std::setprecision(2)
           << "Time to solve       = " << search_time_ms / 1000.0 << " s" << std::endl
           << "Nodes per ms        = " << get_nodes_per_ms() << std::endl
           << "Nodes               = " << get_num_nodes() << std::endl
           << "Table:" << std::endl
           << "    Hit rate        = " << get_hit_rate() * 100 << "%" << std::endl
           << "    New write rate  = " << get_new_write_rate() * 100 << "%" << std::endl
           << "    Rewrite rate    = " << get_rewrite_rate() * 100 << "%" << std::endl
           << "    Overwrite rate  = " << get_overwrite_rate() * 100 << "%" << std::endl
           << "Best moves guessed  = " << get_best_move_guess_rate() * 100 << "%" << std::endl
           << "Worst moves guessed = " << get_worst_move_guess_rate() * 100 << "%" << std::endl
           << std::endl
           << "Interior nodes:" << std::endl
           << std::left
           << std::setw(5) << "Depth"
           << std::right
           << std::setw(18) << "Exact"
           << std::setw(18) << "Lower"
           << std::setw(18) << "Upper"
           << std::setw(18) << "Total"
           << std::endl;

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        result << std::left
               << std::setw(5) << i
               << std::right
               << std::setw(18) << num_exact_nodes[i]
               << std::setw(18) << num_lower_nodes[i]
               << std::setw(18) << num_upper_nodes[i]
               << std::setw(18) << num_exact_nodes[i] + num_lower_nodes[i] + num_upper_nodes[i]
               << std::endl;
    }

    result << std::left
            << std::setw(5) << "Total"
            << std::right
            << std::setw(18) << sum_over_depth(num_exact_nodes)
            << std::setw(18) << sum_over_depth(num_lower_nodes)
            << std::setw(18) << sum_over_depth(num_upper_nodes)
            << std::setw(18) << get_num_interior_nodes()
            << std::endl;

    return result.str();
}

unsigned long long Stats::sum_over_depth(const unsigned long long *depths) const {
    unsigned long long total = 0;

    for (int i = 0; i < BOARD_WIDTH * BOARD_HEIGHT; i++) {
        total += depths[i];
    }

    return total;
}
