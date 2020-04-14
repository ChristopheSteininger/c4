#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "table.h"
#include "settings.h"
#include "hashing.h"


unsigned long stat_num_lookups = 0;
unsigned long stat_num_successful_lookups = 0;
unsigned long stat_num_hash_collisions = 0;

unsigned long stat_num_entries = 0;
unsigned long stat_num_overwrites = 0;


struct entry {
    u_int8_t type_and_value;  // 2 bits for the type, 6 bits for the value.
    board hash;
};


static const int TYPE_NOT_PRESENT = 0;
const int TYPE_UPPER_BOUND = 1;
const int TYPE_LOWER_BOUND = 2;
const int TYPE_EXACT = 3;

// Affects performance. Use a prime number for fewer collisions.
static const unsigned long TABLE_SIZE = 1000007;
static struct entry *table = NULL;


int allocate_table() {
    assert(table == NULL);
    
    table = calloc(TABLE_SIZE, sizeof(struct entry));
    
    return table != NULL;
}


void free_table() {
    assert(table != NULL);
    
    free(table);
    table = NULL;
}


void clear_table() {
    assert(table != NULL);
    
    memset(table, 0, sizeof(struct entry));
}


int table_lookup(board player, board opponent, int *type, int *value) {
    assert(table != NULL);

    stat_num_lookups++;
    
    board hash = hash_state(player, opponent);
    int index = hash % TABLE_SIZE;

    struct entry result = table[index];

    // If this state has not been seen.
    if (result.type_and_value == TYPE_NOT_PRESENT) {
        return 0;
    }

    // If this is a hash collision.
    if (result.hash != hash) {
        stat_num_hash_collisions++;
        return 0;
    }

    // Otherwise reconstruct the type and value of the entry.
    *type = result.type_and_value >> 6;
    *value = (result.type_and_value & 63) - 1;

    stat_num_successful_lookups++;
    return 1;
}

void table_store(board player, board opponent, int type, int value) {
    assert(table != NULL);
    assert(type == TYPE_UPPER_BOUND || type == TYPE_LOWER_BOUND || type == TYPE_EXACT);
    assert(-1 <= value && value <= 1);
    
    board hash = hash_state(player, opponent);
    int index = hash % TABLE_SIZE;

    if (table[index].type_and_value == TYPE_NOT_PRESENT) {
        stat_num_entries++;
    } else {
        stat_num_overwrites++;
    }

    struct entry new_entry = {
        .type_and_value = (type << 6) | (value + 1),
        .hash = hash
    };

    table[index] = new_entry;
}


double get_table_size_in_gigabytes() {
    return (double) TABLE_SIZE * sizeof(struct entry) / 1024 / 1024 / 1024;
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
