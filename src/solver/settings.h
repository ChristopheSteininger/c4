#ifndef SETTINGS_H_
#define SETTINGS_H_

#include <cstdint>

// Defines settings which can be tuned for the target machine and target problem.
// In rough order of importance.

// The shape of the board.
inline constexpr int BOARD_WIDTH = 7;
inline constexpr int BOARD_HEIGHT = 6;

// Number of search threads.
inline constexpr int NUM_THREADS = 4;

// The transposition table uses the Chinese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. The size of the table should be a prime
// number for fewer collisions.
// Some example prime numbers for table sizes, with the memory requirements:
//  * 8388617    : 64 MB
//  * 134217757  :  1 GB
//  * 1073741827 :  8 GB
//  * 4294967311 : 32 GB
//  * 6442450967 : 48 GB
//  * 7247757317 : 54 GB
inline constexpr uint64_t NUM_TABLE_ENTRIES = 134217757;

// Enable 2 MB pages, instead of 4 KB. Not implemented for Macs.
inline constexpr bool ENABLE_HUGE_PAGES = false;

// Restrict each search thread to a single core. Only implemented on Windows.
inline constexpr bool ENABLE_AFFINITY = false;

// At depths higher than this value, the search will do a transposition table
// lookup for each child in hope of tightening bounds or finding a cut off.
inline constexpr int ENHANCED_TABLE_CUTOFF_PLIES = BOARD_WIDTH * BOARD_HEIGHT - 15;

// Determines how much noise to add to move scores near the root of the search tree. This noise helps
// threads to desync.
inline constexpr float MOVE_SCORE_JITTER = (NUM_THREADS > 1) ? 0.3f : 0.0f;

// Whether an opening book should be read into the transposition table before solving any positions.
inline constexpr bool LOAD_BOOK_FILE = false;

// Table files contain significant results (nodes with millions of child nodes) which are used to
// speed up future runs. These settings control the use of these files.
inline constexpr unsigned long long MIN_NODES_FOR_TABLE_FILE = 1 * 1000 * 1000;
inline constexpr bool LOAD_TABLE_FILE = false;
inline constexpr bool UPDATE_TABLE_FILE = false;

static_assert(!(LOAD_BOOK_FILE && LOAD_TABLE_FILE), "Cannot load an opening book and a table file.");

#endif
