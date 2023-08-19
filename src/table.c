#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include "table.h"
#include "settings.h"
#include "hashing.h"
#include "board.h"


const int TYPE_UPPER = 1;
const int TYPE_LOWER = 2;
const int TYPE_EXACT = 3;

// This table uses the Chineese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
// Some example prime numbers:
//  * 8388617    = 64 MB
//  * 134217757  = 1 GB
//  * 1073741827 = 8 GB
static const unsigned int TABLE_SIZE = 134217757;

static board *table = NULL;

// The number of bits of the hash stored in each entry.
static const unsigned int KEY_SIZE = 56;
static const board KEY_MASK = ((board) 1 << KEY_SIZE) - 1;

// The number of bits stored against each hash.
static const unsigned int VALUE_SIZE = 8;
static const board VALUE_MASK = (1 << VALUE_SIZE) - 1;

static unsigned long stat_num_lookups = 0;
static unsigned long stat_num_successful_lookups = 0;
static unsigned long stat_num_hash_collisions = 0;

static unsigned long stat_num_entries = 0;
static unsigned long stat_num_overwrites = 0;
static unsigned long stat_num_rewrites = 0;


int allocate_table() {
    assert(table == NULL);
    
    // Not all bits of the hash are saved, however the hashing will still by unique
    // by the Chinese Remainder Theorem as long as the check below passes.
    if (log2(TABLE_SIZE) + KEY_SIZE < BOARD_HEIGHT_1 * BOARD_WIDTH) {
        printf("The table is too small which may result in invalid results. Exiting.\n");

        return 0;
    }

    table = calloc(TABLE_SIZE, sizeof(board));

    return table != NULL;
}


void free_table() {
    assert(table != NULL);
    
    free(table);
    table = NULL;
}


void clear_table() {
    assert(table != NULL);
    
    memset(table, 0, TABLE_SIZE * sizeof(board));
}

int table_lookup(board player, board opponent, int *best_move, int *type, int *value) {
    assert(table != NULL);

    stat_num_lookups++;
    
    int is_mirrored;
    board hash = hash_state(player, opponent, &is_mirrored);
    
    int index = hash % TABLE_SIZE;
    board result = table[index];

    // If this state has not been seen.
    if (result == 0) {
        return 0;
    }

    // If this is a hash collision.
    if ((result >> VALUE_SIZE) != (hash & KEY_MASK)) {
        stat_num_hash_collisions++;
        return 0;
    }

    // Otherwise reconstruct the type and value of the entry.
    board entry = result & VALUE_MASK;
    *best_move = entry >> 4;
    *type = (entry >> 2) & 3;
    *value = (entry & 3) - 1;

    // If we looked up a mirrored move, mirror the best move as well.
    if (is_mirrored) {
        *best_move = BOARD_WIDTH - *best_move - 1;
    }

    stat_num_successful_lookups++;
    return 1;
}

void table_store(board player, board opponent, int best_move, int type, int value) {
    assert(table != NULL);
    assert(best_move == BOARD_WIDTH || is_move_valid(player, opponent, best_move));
    assert(type == TYPE_UPPER || type == TYPE_LOWER || type == TYPE_EXACT);
    assert(-1 <= value && value <= 1);
    
    int is_mirrored;
    board hash = hash_state(player, opponent, &is_mirrored);
    
    int index = hash % TABLE_SIZE;
    board current_entry = table[index];

    // Only the partial hash needs to be stored. This is equivalent to
    // hash % 2^KEY_SIZE.
    board stored_hash = hash & KEY_MASK;

    // Best move needs to be mirrored as well if we are storing the mirrored position.
    if (is_mirrored) {
        best_move = BOARD_WIDTH - best_move - 1;
    }

    // Update table statistics.
    if (current_entry == 0) {
        stat_num_entries++;
    } else if ((current_entry >> VALUE_SIZE) == stored_hash) {
        stat_num_rewrites++;
    } else {
        stat_num_overwrites++;
    }

    // Store.
    table[index] = (stored_hash << VALUE_SIZE) | (best_move << 4) | (type << 2) | (value + 1);
}


double get_table_size_in_gigabytes() {
    return (double) TABLE_SIZE * sizeof(board) / 1024 / 1024 / 1024;
}


double get_table_hit_rate() {
    return (double) stat_num_successful_lookups / stat_num_lookups;
}


double get_table_collision_rate() {
    return (double) stat_num_hash_collisions / stat_num_lookups;
}


double get_table_density() {
    return (double) stat_num_entries / TABLE_SIZE;
}


double get_table_overwrite_rate() {
    return (double) stat_num_overwrites / (stat_num_entries + stat_num_rewrites + stat_num_overwrites);
}


double get_table_rewrite_rate() {
    return (double) stat_num_rewrites / (stat_num_entries + stat_num_rewrites + stat_num_overwrites);
}
