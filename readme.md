**Web UI: https://christophesteininger.github.io/c4**

[![Build](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml/badge.svg?branch=master)](https://github.com/ChristopheSteininger/c4/actions/workflows/ci.yaml?query=branch%3Amaster)

Connect 4 on a 7x6 board was first
[solved](https://en.wikipedia.org/wiki/Solved_game) in 1988 independently by
James D. Allen and Victor Allis. John Tromp solved the game for all board sizes where
width + height $\leq$ 15 in 2006, and solved the 8x8 case in 2015.

I've extended these results by solving the game on all board sizes where $w$ + $h$
= 16, as well as some board sizes where $w$ + $h$ $\geq$ 17.

## Results

### Outcome

The table below shows which player will win of a game of Connect 4 if both players play
perfectly on different board sizes. 1<sup>st</sup> means the first player to move can
guarantee a win, while 2<sup>nd</sup> means the second player to move can guarantee a win.
On some board sizes, neither player will be able to force a win, in which case the outcome
will be a draw.

| Width &rarr; <br> Height &darr; |    4 |    5 |              6 |              7 |              8 |              9 |             10 |             11 |             12 |
| ------------------------------- | ---- | ---- | -------------- | -------------- | -------------- | -------------- | -------------- | -------------- | -------------- |
|                           **4** | Draw | Draw | 2<sup>nd</sup> | Draw           | 2<sup>nd</sup> | 2<sup>nd</sup> | 2<sup>nd</sup> | 2<sup>nd</sup> | 2<sup>nd</sup> |
|                           **5** | Draw | Draw | Draw           | Draw           | 1<sup>st</sup> | 1<sup>st</sup> | 1<sup>st</sup> | 1<sup>st</sup> |
|                           **6** | Draw | Draw | 2<sup>nd</sup> | 1<sup>st</sup> | 2<sup>nd</sup> | 2<sup>nd</sup> | 1<sup>st</sup> |
|                           **7** | Draw | Draw | 1<sup>st</sup> | Draw           | 1<sup>st</sup> | 1<sup>st</sup> |
|                           **8** | Draw | Draw | 2<sup>nd</sup> | 1<sup>st</sup> | 2<sup>nd</sup> |
|                           **9** | Draw | Draw | 1<sup>st</sup> | Draw           | 1<sup>st</sup> |
|                          **10** | Draw | Draw | 2<sup>nd</sup> | 1<sup>st</sup> |
|                          **11** | Draw | Draw | 1<sup>st</sup> |
|                          **12** | Draw | Draw | 2<sup>nd</sup> |
|                          **13** | Draw | Draw |

### Last Move

The table below shows the *move* on which the game will end if both players are perfect.
An entry such as "53" for the 8x7 board means the game will end on the 53<sup>rd</sup>
move. With the results from the above table, this means the 1<sup>st</sup>
player can always force a win on move #53 if the second player plays perfectly. If the second
player in an 8x7 game is not perfect, then the first player can always force a win
before move #53.

| Width &rarr; <br> Height &darr; |  4 |  5 |  6 |  7 |  8 |  9 | 10 | 11 | 12 |
| ------------------------------- | -- | -- | -- | -- | -- | -- | -- | -- | -- |
|                           **4** | 16 | 20 | 24 | 28 | 32 | 36 | 40 | 44 | 48 |
|                           **5** | 20 | 25 | 30 | 35 | 39 | 41 | 47 | 51 |
|                           **6** | 24 | 30 | 36 | 41 | 48 | 52 | 59 |
|                           **7** | 28 | 35 | 41 | 49 | 53 | 55 |
|                           **8** | 32 | 40 | 48 | 55 | 62 |
|                           **9** | 36 | 45 | 53 | 63 |
|                          **10** | 40 | 50 | 60 | 69 |
|                          **11** | 44 | 55 | 63 |
|                          **12** | 48 | 60 | 72 |
|                          **13** | 52 | 65 |

## Building

Build using CMake. Run these commands in the root dir to compile and solve
Connect 4 on the standard 7x6 board from scratch (i.e. no precomputed data).
This will only take a few seconds.

```
$ cmake --preset optimise
$ cmake --build --preset optimise
$ out/optimise/c4
```

On Windows, the repo can be imported as a Visual Studio CMake project.

Adjust the values in [src/solver/settings.h](./src/solver/settings.h) then recompile
to set the size of the board, number of threads, memory usage, and more. Increasing
number of threads and memory usage to the maximum available on your machine will reduce
solve time significantly.

Compiling will generate five executables:
1. **c4**: Solves a single position then prints the result and search statistics. Used
to generate the table above.
2. **play**: Interactive program to play against the solver.
3. **test**: Runs unit tests, then tests and benchmarks the solver using positions
with independently verified scores.
4. **book**: Generates an opening book by solving all positions up to a set depth.
5. **random**: Generates random games for testing and benchmarking.

## Credits

This solver expands on the work of two others:
* [John Tromp](https://tromp.github.io/c4/c4.html)
* [Pascal Pons](https://github.com/PascalPons/connect4)
* [Guido Zuidhof](https://github.com/gzuidhof/coi-serviceworker) - COOP and COEP workaround for GitHub pages.
