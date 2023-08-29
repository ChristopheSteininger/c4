#include "stats.h"


void Stats::merge(const Stats &other) {
    this->num_nodes              += other.num_nodes;
    this->num_exact_nodes        += other.num_exact_nodes;
    this->num_lower_nodes        += other.num_lower_nodes;
    this->num_upper_nodes        += other.num_upper_nodes;
    this->num_best_moves_guessed += other.num_best_moves_guessed;

    this->num_lookup_success     += other.num_lookup_success;
    this->num_lookup_collision   += other.num_lookup_collision;
    this->num_lookup_miss        += other.num_lookup_miss;

    this->num_store_entries      += other.num_store_entries;
    this->num_store_overwrites   += other.num_store_overwrites;
    this->num_store_rewrites     += other.num_store_rewrites;
}


void Stats::reset() {
    num_nodes              = 0;
    num_exact_nodes        = 0;
    num_lower_nodes        = 0;
    num_upper_nodes        = 0;
    num_best_moves_guessed = 0;

    num_lookup_success     = 0;
    num_lookup_collision   = 0;
    num_lookup_miss        = 0;

    num_store_entries      = 0;
    num_store_overwrites   = 0;
    num_store_rewrites     = 0;
}
