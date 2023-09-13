#ifndef SETTINGS_H_
#define SETTINGS_H_


// Defines settings which can be tuned for the target machine and target problem.
// In rough order of importance.


inline constexpr int BOARD_WIDTH = 7;
inline constexpr int BOARD_HEIGHT = 6;


inline constexpr int NUM_THREADS = 8;


// The transposition table uses the Chinese Remainer Theorem to reduce the number of bits per entry.
// For this to work, the size of the table must be odd. Use a prime number for fewer collisions.
// Some example prime numbers:
//  * 131101     =  1 MB
//  * 1048583    =  8 MB
//  * 8388617    = 64 MB
//  * 134217757  =  1 GB
//  * 1073741827 =  8 GB
inline constexpr int NUM_TABLE_ENTRIES = 1073741827;


// At depths higher than this value, the search will do a transposition table
// lookup for each child in hopes of tightening bounds or finding a cut off.
inline constexpr int ENHANCED_TABLE_CUTOFF_PLIES = BOARD_WIDTH * BOARD_HEIGHT - 15;


#endif
