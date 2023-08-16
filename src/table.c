#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "table.h"
#include "settings.h"
#include "hashing.h"


const int TYPE_UPPER_BOUND = 1;
const int TYPE_LOWER_BOUND = 2;
const int TYPE_EXACT = 3;

// This table uses the Chineese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
// Some example prime numbers:
//  * 8388617    = 64 MB
//  * 134217757  = 1 GB
//  * 1073741827 = 8 GB
static const unsigned int TABLE_SIZE = 134217757;

static board *table = NULL;

// The number of bits stored against each hash.
static const unsigned int KEY_SIZE = 56;
static const unsigned int VALUE_SIZE = 8;
static const board VALUE_MASK = (1 << VALUE_SIZE) - 1;

static unsigned long stat_num_lookups = 0;
static unsigned long stat_num_successful_lookups = 0;
static unsigned long stat_num_hash_collisions = 0;

static unsigned long stat_num_entries = 0;
static unsigned long stat_num_overwrites = 0;


int allocate_table() {
    assert(table == NULL);
    
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

int table_lookup(board player, board opponent, int *type, int *value) {
    assert(table != NULL);

    stat_num_lookups++;

    board hash = hash_state(player, opponent);
    int index = hash % TABLE_SIZE;

    board result = table[index];

    // If this state has not been seen.
    if (result == 0) {
        return 0;
    }

    // If this is a hash collision.
    if ((result & ~VALUE_MASK) != (hash & ~VALUE_MASK)) {
        stat_num_hash_collisions++;
        return 0;
    }

    // Otherwise reconstruct the type and value of the entry.
    *type = (result & VALUE_MASK) >> 6;
    *value = (result & 63) - 1;

    stat_num_successful_lookups++;
    return 1;
}

void table_store(board player, board opponent, int type, int value) {
    assert(table != NULL);
    assert(type == TYPE_UPPER_BOUND || type == TYPE_LOWER_BOUND || type == TYPE_EXACT);
    assert(-1 <= value && value <= 1);
    
    board hash = hash_state(player, opponent);
    int index = hash % TABLE_SIZE;

    if (table[index] == 0) {
        stat_num_entries++;
    } else {
        stat_num_overwrites++;
    }

    table[index] = (hash & ~VALUE_MASK) | (type << 6) | (value + 1);
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
    return (double) stat_num_overwrites / (stat_num_entries + stat_num_overwrites);
}
