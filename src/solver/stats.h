#ifndef STATS_H_
#define STATS_H_

#include <cassert>

#include "types.h"

// Used to track the performance of the solver. Not thread safe.
class Stats {
   public:
    void merge(const Stats &other);
    void reset();

    // Search stats getters.
    unsigned long get_num_nodes() const { return num_nodes; }
    unsigned long get_num_exact_nodes() const { return num_exact_nodes; }
    unsigned long get_num_lower_nodes() const { return num_lower_nodes; }
    unsigned long get_num_upper_nodes() const { return num_upper_nodes; }
    double get_best_move_guess_rate() const { return (double)num_best_moves_guessed / get_num_interior_nodes(); }
    double get_worst_move_guess_rate() const { return (double)num_worst_moves_guessed / get_num_interior_nodes(); }
    unsigned long get_num_interior_nodes() const { return num_exact_nodes + num_lower_nodes + num_upper_nodes; }

    // Lookup stats getters.
    double get_hit_rate() const {
        return (double)num_lookup_success / (num_lookup_success + num_lookup_miss + num_lookup_collision);
    }
    double get_collision_rate() const {
        return (double)num_lookup_collision / (num_lookup_success + num_lookup_miss + num_lookup_collision);
    }

    // Store stats getters.
    double get_new_write_rate() const {
        return (double)num_store_entries / (num_store_entries + num_store_rewrites + num_store_overwrites);
    }
    double get_rewrite_rate() const {
        return (double)num_store_rewrites / (num_store_entries + num_store_rewrites + num_store_overwrites);
    }
    double get_overwrite_rate() const {
        return (double)num_store_overwrites / (num_store_entries + num_store_rewrites + num_store_overwrites);
    }

    // Search stats increments.
    void new_node() { num_nodes++; }
    void node_type(NodeType type) {
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
    void best_move_guessed() { num_best_moves_guessed++; }
    void worst_move_guessed() { num_worst_moves_guessed++; }

    // Lookup stats increments.
    void lookup_success() { num_lookup_success++; }
    void lookup_collision() { num_lookup_collision++; }
    void lookup_miss() { num_lookup_miss++; }

    // Store stats increments.
    void store_new_entry() { num_store_entries++; }
    void store_overwrite() { num_store_overwrites++; }
    void store_rewrite() { num_store_rewrites++; }

   private:
    // Search stats.
    unsigned long num_nodes{0};
    unsigned long num_exact_nodes{0};
    unsigned long num_lower_nodes{0};
    unsigned long num_upper_nodes{0};
    unsigned long num_best_moves_guessed{0};
    unsigned long num_worst_moves_guessed{0};

    // Lookup stats.
    unsigned long num_lookup_success{0};
    unsigned long num_lookup_collision{0};
    unsigned long num_lookup_miss{0};

    // Store stats.
    unsigned long num_store_entries{0};
    unsigned long num_store_overwrites{0};
    unsigned long num_store_rewrites{0};
};

#endif
