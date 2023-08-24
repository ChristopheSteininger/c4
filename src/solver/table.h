#ifndef TABLE_H_
#define TABLE_H_

#include "position.h"


extern const int TYPE_LOWER;
extern const int TYPE_UPPER;
extern const int TYPE_EXACT;

class Table {
public:
    Table();
    ~Table();

    void clear();

    bool get(Position &pos, int &best_move, int &type, int &value);
    void put(Position &pos, int best_move, int type, int value);

    double get_size_in_gigabytes() const;
    double get_hit_rate() const;
    double get_collision_rate() const;
    double get_density() const;
    double get_rewrite_rate() const;
    double get_overwrite_rate() const;

private:
    board *table;

    unsigned long stat_num_lookups = 0;
    unsigned long stat_num_successful_lookups = 0;
    unsigned long stat_num_hash_collisions = 0;

    unsigned long stat_num_entries = 0;
    unsigned long stat_num_overwrites = 0;
    unsigned long stat_num_rewrites = 0;
};


#endif
