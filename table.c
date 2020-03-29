#include <stdlib.h>
#include <assert.h>

#include "table.h"
#include "settings.h"
#include "hashing.h"


struct entry {
    u_int8_t type_and_value;  // 2 bits for the type, 6 bits for the value.
    board hash;
};


static const int TYPE_NOT_PRESENT = 0;
const int TYPE_UPPER_BOUND = 1;
const int TYPE_LOWER_BOUND = 2;
const int TYPE_EXACT = 3;

// Affects performance. Use a prime number for fewer collisions.
static const int TABLE_SIZE = 100000007;
static struct entry *table = NULL;


int allocate_table() {
    table = calloc(TABLE_SIZE, sizeof(struct entry));
    
    return table != NULL;
}

void free_table() {
    assert(table != NULL);
    
    free(table);
    table = NULL;
}

int table_lookup(board player, board opponent, int *type, int *value) {
    assert(table != NULL);
    
    board hash = hash_state(player, opponent);
    int index = hash % TABLE_SIZE;

    struct entry result = table[index];

    // If this state has not been seen.
    if (result.type_and_value == TYPE_NOT_PRESENT) {
        return 0;
    }

    // If this is a hash collision.
    if (result.hash != hash) {
        return 0;
    }

    // Otherwise reconstruct the type and value of the entry.
    *type = result.type_and_value >> 6;
    *value = (result.type_and_value & 63) - 10;

    return 1;
}

void table_store(board player, board opponent, int type, int value) {
    assert(table != NULL);
    assert(type == TYPE_UPPER_BOUND || type == TYPE_LOWER_BOUND || type == TYPE_EXACT);
    assert(-10 <= value && value <= 10);
    
    board hash = hash_state(player, opponent);
    int index = hash % TABLE_SIZE;

    struct entry new_entry = {
        .type_and_value = (type << 6) | (value + 10),
        .hash = hash
    };

    table[index] = new_entry;
}
