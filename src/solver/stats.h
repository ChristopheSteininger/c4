#ifndef STATS_H_
#define STATS_H_

#include <chrono>
#include <string>

#include "types.h"

// Used to track the performance of the solver. Not thread safe.
class Stats {
   public:
    void merge(const Stats &other);
    void reset();

    // Search stats getters.
    unsigned long long get_search_time_ms() const { return search_time_ms; }
    unsigned long long get_nodes_per_ms() const { return num_nodes / std::max(1ULL, search_time_ms); }
    unsigned long long get_num_nodes() const { return num_nodes; }
    double get_best_move_guess_rate() const { return (double)num_best_moves_guessed / get_num_interior_nodes(); }
    double get_worst_move_guess_rate() const { return (double)num_worst_moves_guessed / get_num_interior_nodes(); }

    // Lookup stats getters.
    double get_hit_rate() const { return (double)num_lookup_success / (num_lookup_success + num_lookup_miss); }

    // Store stats getters.
    double get_new_write_rate() const { return (double)num_store_entries / get_num_stores(); }
    double get_rewrite_rate() const { return (double)num_store_rewrites / get_num_stores(); }
    double get_overwrite_rate() const { return (double)num_store_overwrites / get_num_stores(); }

    // Search stats increments.
    void completed_search(std::chrono::steady_clock::time_point search_start_time);
    void new_node() { num_nodes++; }
    void new_interior_node(NodeType type, int num_moves);

    void best_move_guessed() { num_best_moves_guessed++; }
    void worst_move_guessed() { num_worst_moves_guessed++; }

    // Lookup stats increments.
    void lookup_success() { num_lookup_success++; }
    void lookup_miss() { num_lookup_miss++; }

    // Store stats increments.
    void store_new_entry() { num_store_entries++; }
    void store_overwrite() { num_store_overwrites++; }
    void store_rewrite() { num_store_rewrites++; }

    std::string display_all_stats() const;

   private:
    // Search stats.
    unsigned long long search_time_ms{0};
    unsigned long long num_nodes{0};
    unsigned long long num_best_moves_guessed{0};
    unsigned long long num_worst_moves_guessed{0};

    // Depth stats.
    unsigned long long num_exact_nodes[BOARD_WIDTH * BOARD_HEIGHT]{0};
    unsigned long long num_lower_nodes[BOARD_WIDTH * BOARD_HEIGHT]{0};
    unsigned long long num_upper_nodes[BOARD_WIDTH * BOARD_HEIGHT]{0};

    // Lookup stats.
    unsigned long long num_lookup_success{0};
    unsigned long long num_lookup_miss{0};

    // Store stats.
    unsigned long long num_store_entries{0};
    unsigned long long num_store_overwrites{0};
    unsigned long long num_store_rewrites{0};

    unsigned long long sum_over_depth(const unsigned long long *depths) const;
    unsigned long long get_num_interior_nodes() const {
        return sum_over_depth(num_exact_nodes) + sum_over_depth(num_lower_nodes) + sum_over_depth(num_upper_nodes);
    }
    unsigned long long get_num_stores() const { return num_store_entries + num_store_rewrites + num_store_overwrites; }
};

#endif
