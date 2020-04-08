#ifndef TABLE_H_
#define TABLE_H_

#include "board.h"


extern const int TYPE_LOWER_BOUND;
extern const int TYPE_UPPER_BOUND;
extern const int TYPE_EXACT;


int allocate_table();

void free_table();

int table_lookup(board, board, int *, int *);

void table_store(board, board, int, int);

double get_table_size_in_gigabytes();

double get_table_hit_rate();

double get_table_collision_rate();

double get_table_density();

double get_table_overwrite_rate();

#endif
