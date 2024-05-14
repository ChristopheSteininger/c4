#ifndef STATS_H_
#define STATS_H_

#include <chrono>
#include <string>

#include "../types.h"

// Used to track the performance of the solver. Not thread safe.
class Stats {
   public:
    void merge(const Stats &other);
    void reset();

    // Search stats getters.
    unsigned long long get_search_time_ms() const noexcept { return search_time_ms; }
    unsigned long long get_nodes_per_ms() const noexcept { return num_nodes / std::max(1ULL, search_time_ms); }
    unsigned long long get_num_nodes() const noexcept { return num_nodes; }
    double get_best_move_guess_rate() const noexcept { return (double)num_best_moves_guessed / get_num_interior_nodes(); }
    double get_worst_move_guess_rate() const noexcept { return (double)num_worst_moves_guessed / get_num_interior_nodes(); }

    // Lookup stats getters.
    double get_hit_rate() const noexcept { return (double)num_lookup_success / (num_lookup_success + num_lookup_miss); }

    // Store stats getters.
    double get_new_write_rate() const noexcept { return (double)num_store_entries / get_num_stores(); }
    double get_rewrite_rate() const noexcept { return (double)num_store_rewrites / get_num_stores(); }
    double get_overwrite_rate() const noexcept { return (double)num_store_overwrites / get_num_stores(); }

    // Search stats increments.
    void completed_search(std::chrono::steady_clock::time_point search_start_time) noexcept;
    void new_node() noexcept { num_nodes++; }
    void new_interior_node(NodeType type) noexcept;

    void best_move_guessed() noexcept { num_best_moves_guessed++; }
    void worst_move_guessed() noexcept { num_worst_moves_guessed++; }

    // Lookup stats increments.
    void lookup_success() noexcept { num_lookup_success++; }
    void lookup_miss() noexcept { num_lookup_miss++; }

    // Store stats increments.
    void store_new_entry() noexcept { num_store_entries++; }
    void store_overwrite() noexcept { num_store_overwrites++; }
    void store_rewrite() noexcept { num_store_rewrites++; }

    std::string display_all_stats() const;

   private:
    // Search stats.
    unsigned long long search_time_ms{0};
    unsigned long long num_nodes{0};
    unsigned long long num_best_moves_guessed{0};
    unsigned long long num_worst_moves_guessed{0};

    // Type stats.
    unsigned long long num_exact_nodes{0};
    unsigned long long num_lower_nodes{0};
    unsigned long long num_upper_nodes{0};

    // Lookup stats.
    unsigned long long num_lookup_success{0};
    unsigned long long num_lookup_miss{0};

    // Store stats.
    unsigned long long num_store_entries{0};
    unsigned long long num_store_overwrites{0};
    unsigned long long num_store_rewrites{0};

    unsigned long long get_num_interior_nodes() const noexcept { return num_exact_nodes + num_lower_nodes + num_upper_nodes; }
    unsigned long long get_num_stores() const noexcept { return num_store_entries + num_store_rewrites + num_store_overwrites; }
};

#endif
