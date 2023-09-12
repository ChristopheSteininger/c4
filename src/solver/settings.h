#ifndef SETTINGS_H_
#define SETTINGS_H_

#define BOARD_WIDTH 7
#define BOARD_HEIGHT 6

#define NUM_THREADS 8

// At depths higher than this value, the search will do a transposition table
// lookup for each child in hopes of tightening bounds or finding a cut off.
inline constexpr int ENHANCED_TABLE_CUTOFF_PLIES = 30;

#endif
