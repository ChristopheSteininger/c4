#ifndef TYPES_H_
#define TYPES_H_

#include <cstdint>
#include <type_traits>

#include "settings.h"

static constexpr bool use_128bit = (BOARD_HEIGHT + 1) * BOARD_WIDTH > 64;

#ifdef __SIZEOF_INT128__
// A number wide enough to store one bit for each cell on the board and the column headers.
using board = std::conditional_t<use_128bit, __uint128_t, uint64_t>;
#else
static_assert(!use_128bit, "Board requires 128 bit support.");

using board = uint64_t;
#endif

static_assert(BOARD_WIDTH * (BOARD_HEIGHT + 1) <= 8 * sizeof(board),
    "The board must be small enough to fit inside the chosen int type");

// Every position searched will return either an exact score, or a lower/upper bound on the score. Positions which have
// not been searched yet are a miss.
enum class NodeType {
    MISS = 0,
    LOWER = 1,
    UPPER = 2,
    EXACT = 3,
};

#endif
