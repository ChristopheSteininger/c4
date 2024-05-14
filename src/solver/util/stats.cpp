#include "stats.h"

#include <cassert>
#include <iomanip>
#include <sstream>

void Stats::merge(const Stats &other) {
    search_time_ms += other.search_time_ms;
    num_nodes += other.num_nodes;
    num_best_moves_guessed += other.num_best_moves_guessed;
    num_worst_moves_guessed += other.num_worst_moves_guessed;

    num_exact_nodes += other.num_exact_nodes;
    num_lower_nodes += other.num_lower_nodes;
    num_upper_nodes += other.num_upper_nodes;

    num_lookup_success += other.num_lookup_success;
    num_lookup_miss += other.num_lookup_miss;

    num_store_entries += other.num_store_entries;
    num_store_overwrites += other.num_store_overwrites;
    num_store_rewrites += other.num_store_rewrites;
}

void Stats::reset() {
    search_time_ms = 0;
    num_nodes = 0;
    num_best_moves_guessed = 0;
    num_worst_moves_guessed = 0;

    num_exact_nodes = 0;
    num_lower_nodes = 0;
    num_upper_nodes = 0;

    num_lookup_success = 0;
    num_lookup_miss = 0;

    num_store_entries = 0;
    num_store_overwrites = 0;
    num_store_rewrites = 0;
}

void Stats::completed_search(std::chrono::steady_clock::time_point search_start_time) noexcept {
    assert(this->search_time_ms == 0);

    auto duration = std::chrono::steady_clock::now() - search_start_time;
    this->search_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
}

void Stats::new_interior_node(NodeType type) noexcept {
    switch (type) {
        case NodeType::EXACT:
            num_exact_nodes++;
            break;

        case NodeType::LOWER:
            num_lower_nodes++;
            break;

        case NodeType::UPPER:
            num_upper_nodes++;
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
           << "Nodes: " << std::endl
           << "    Exact           = " << num_exact_nodes << std::endl
           << "    Lower           = " << num_lower_nodes << std::endl
           << "    Upper           = " << num_upper_nodes << std::endl
           << "    Leaf            = " << num_nodes - get_num_interior_nodes() << std::endl
           << "    Total           = " << num_nodes << std::endl
           << "Table:" << std::endl
           << "    Hit rate        = " << get_hit_rate() * 100 << "%" << std::endl
           << "    New write rate  = " << get_new_write_rate() * 100 << "%" << std::endl
           << "    Rewrite rate    = " << get_rewrite_rate() * 100 << "%" << std::endl
           << "    Overwrite rate  = " << get_overwrite_rate() * 100 << "%" << std::endl
           << "Best moves guessed  = " << get_best_move_guess_rate() * 100 << "%" << std::endl
           << "Worst moves guessed = " << get_worst_move_guess_rate() * 100 << "%" << std::endl
           << std::endl;

    return result.str();
}
