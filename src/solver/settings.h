#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <cstdint>

// Defines settings which can be tuned for the target machine and target problem.
// In rough order of importance.

inline constexpr int BOARD_WIDTH = 7;
inline constexpr int BOARD_HEIGHT = 6;

inline constexpr int NUM_THREADS = 4;

// The transposition table uses the Chinese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
// Some example prime numbers:
//  * 8388617    = 64 MB
//  * 134217757  =  1 GB
//  * 1073741827 =  8 GB
//  * 4294967311 = 32 GB
//  * 6442450967 = 48 GB
inline constexpr uint64_t NUM_TABLE_ENTRIES = 134217757;

inline constexpr bool ENABLE_HUGE_PAGES = false;

inline constexpr bool ENABLE_AFFINITY = false;

// At depths higher than this value, the search will do a transposition table
// lookup for each child in hope of tightening bounds or finding a cut off.
inline constexpr int ENHANCED_TABLE_CUTOFF_PLIES = BOARD_WIDTH * BOARD_HEIGHT - 12;

// Determines how much noise to add to move scores near the root of the search tree. This noise helps
// threads to desync. Increase jitter with number of threads.
inline constexpr float MOVE_SCORE_JITTER = (NUM_THREADS > 1) ? 0.3f : 0.0f;

#endif
