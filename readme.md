[![Build](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml/badge.svg?branch=master)](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml?query=branch%3Amaster)

 ## Results

 The table below shows the outcome of a game of Connect 4 if both players play
 perfectly on different board sizes. An entry like "1<sup>st</sup>, move 53"
 for the 8x7 board means the 1<sup>st</sup> player can force a win on
 move #53 if the second player plays perfectly. If the second player in
 a 8x7 game is not perfect, then the first player can force a win before move
 #53.

 All results below were found by running the solver in this repo. The solution of
 board sizes where $width + height \leq 15$, as well as the 8x8 board was already
 found by [previous solvers](https://tromp.github.io/c4/c4.html), but the solution
 to all other board sizes is new.

| Width &rarr; <br> Height &darr; |    4 |    5 |                            6 |                            7 |                            8 |                            9 |                           10 |                           11 |                           12 |
| ------------------------------- | ---- | ---- | ---------------------------- | ---------------------------- | ---------------------------- | ---------------------------- | ---------------------------- | ---------------------------- | ---------------------------- |
|                           **4** | Draw | Draw | 2<sup>nd</sup>, <br> move 24 |                         Draw | 2<sup>nd</sup>, <br> move 32 | 2<sup>nd</sup>, <br> move 36 | 2<sup>nd</sup>, <br> move 40 | 2<sup>nd</sup>, <br> move 44 | 2<sup>nd</sup>, <br> move 48 |
|                           **5** | Draw | Draw |                         Draw |                         Draw | 1<sup>st</sup>, <br> move 39 | 1<sup>st</sup>, <br> move 41 | 1<sup>st</sup>, <br> move 47 |
|                           **6** | Draw | Draw | 2<sup>nd</sup>, <br> move 36 | 1<sup>st</sup>, <br> move 41 | 2<sup>nd</sup>, <br> move 48 | 2<sup>nd</sup>, <br> move 52
|                           **7** | Draw | Draw | 1<sup>st</sup>, <br> move 41 |                         Draw | 1<sup>st</sup>, <br> move 53 |
|                           **8** | Draw | Draw | 2<sup>nd</sup>, <br> move 48 | 1<sup>st</sup>, <br> move 55 |
|                           **9** | Draw | Draw | 1<sup>st</sup>, <br> move 53 |                         Draw |
|                          **10** | Draw | Draw | 2<sup>nd</sup>, <br> move 60 |
|                          **11** | Draw | Draw |
|                          **12** | Draw |
|                          **13** | Draw |

## Credits

This solver expands on the work of two others:
1. [John Tromp](https://tromp.github.io/c4/c4.html)
1. [Pascal Pons](http://blog.gamesolver.org/solving-connect-four/01-introduction/)
