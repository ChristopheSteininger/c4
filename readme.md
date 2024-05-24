[![Build](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml/badge.svg?branch=master)](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml?query=branch%3Amaster)

Connect 4 on a 7x6 board was first
[solved](https://en.wikipedia.org/wiki/Solved_game) in 1988 independently by
James D. Allen and Victor Allis. John Tromp solved the game for all board sizes where
width + height $\leq$ 15 in 2006, and solved the 8x8 case in 2015.

I've extended these results by solving the game on all board sizes where $w$ + $h$
= 16, as well as some board sizes where $w$ + $h$ $\geq$ 17.

The results below include the exact number of moves until the end of the game assuming
perfect play for each board size, which was not previously known to the best of my knowledge.

## Results

 The table below shows the outcome of a game of Connect 4 if both players play
 perfectly on different board sizes. An entry such as "1<sup>st</sup> (53)"
 for the 8x7 board means the 1<sup>st</sup> player can force a win on
 move #53 if the second player plays perfectly. If the second player in
 an 8x7 game is not perfect, then the first player can always force a win before
 move #53.

 All results below were found by running the solver in this repo.

| Width &rarr; <br> Height &darr; |    4 |    5 |                   6 |                   7 |                   8 |                   9 |                  10 |                  11 |                  12 |
| ------------------------------- | ---- | ---- | ------------------- | ------------------- | ------------------- | ------------------- | ------------------- | ------------------- | ------------------- |
|                           **4** | Draw | Draw | 2<sup>nd</sup> (24) |                Draw | 2<sup>nd</sup> (32) | 2<sup>nd</sup> (36) | 2<sup>nd</sup> (40) | 2<sup>nd</sup> (44) | 2<sup>nd</sup> (48) |
|                           **5** | Draw | Draw |                Draw |                Draw | 1<sup>st</sup> (39) | 1<sup>st</sup> (41) | 1<sup>st</sup> (47) | 1<sup>st</sup> (51) |
|                           **6** | Draw | Draw | 2<sup>nd</sup> (36) | 1<sup>st</sup> (41) | 2<sup>nd</sup> (48) | 2<sup>nd</sup> (52) | 1<sup>st</sup> (59)
|                           **7** | Draw | Draw | 1<sup>st</sup> (41) |                Draw | 1<sup>st</sup> (53) | 1<sup>st</sup> (55) |
|                           **8** | Draw | Draw | 2<sup>nd</sup> (48) | 1<sup>st</sup> (55) | 2<sup>nd</sup> (62)
|                           **9** | Draw | Draw | 1<sup>st</sup> (53) |                Draw |
|                          **10** | Draw | Draw | 2<sup>nd</sup> (60) |
|                          **11** | Draw | Draw | 1<sup>st</sup> (63) |
|                          **12** | Draw | Draw | 2<sup>nd</sup> (72) |
|                          **13** | Draw | Draw |

## Building

Build using CMake. Run these commands in the root dir to compile and solve
Connect 4 on the standard 7x6 board:

```
$ cmake --preset optimise
$ cmake --build --preset optimise
$ out/optimise/c4
```

On Windows, the repo can be imported as a Visual Studio CMake project.

Compiling will generate five executables:
1. **c4**: Solves a single position then prints the result and search statistics.
2. **play**: Interactive program to play against the solver.
3. **test**: Runs unit tests, then tests and benchmarks the solver using positions
with independently verified scores.
4. **book**: Generates an opening book by solving all positions up to a set depth.
5. **random**: Generates random games for testing and benchmarking.

Adjust the values in [src/solver/settings.h](./src/solver/settings.h) to set the
size of the board, number of threads, memory usage, and more.

## Credits

This solver expands on the work of two others:
* [John Tromp](https://tromp.github.io/c4/c4.html)
* [Pascal Pons](https://github.com/PascalPons/connect4)
