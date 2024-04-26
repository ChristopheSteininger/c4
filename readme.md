[![Build](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml/badge.svg?branch=master)](https://github.com/ChristopheSteininger/c4/actions/workflows/build-and-test.yml?query=branch%3Amaster)

 ## Results

 The table below shows the outcome of a game of Connect 4 if both players play
 perfectly on different board sizes. An entry like "1<sup>st</sup> (53)"
 for the 8x7 board means the 1<sup>st</sup> player can force a win on
 move #53 if the second player plays perfectly. If the second player in
 an 8x7 game is not perfect, then the first player can force a win before move
 #53.

 All results below were found by running the solver in this repo. The solution of
 board sizes where $width + height \leq 15$ as well as the 8x8 board has already
 been found by [another solver](https://tromp.github.io/c4/c4.html), but the solution
 to all other board sizes is new.

 The exact move on which the game will end assuming perfect play is a new result
 for all board sizes as previous solvers output only a win/draw/loss outcome.

| Width &rarr; <br> Height &darr; |    4 |    5 |                   6 |                   7 |                   8 |                   9 |                  10 |                  11 |                  12 |
| ------------------------------- | ---- | ---- | ------------------- | ------------------- | ------------------- | ------------------- | ------------------- | ------------------- | ------------------- |
|                           **4** | Draw | Draw | 2<sup>nd</sup> (24) |                Draw | 2<sup>nd</sup> (32) | 2<sup>nd</sup> (36) | 2<sup>nd</sup> (40) | 2<sup>nd</sup> (44) | 2<sup>nd</sup> (48) |
|                           **5** | Draw | Draw |                Draw |                Draw | 1<sup>st</sup> (39) | 1<sup>st</sup> (41) | 1<sup>st</sup> (47) |
|                           **6** | Draw | Draw | 2<sup>nd</sup> (36) | 1<sup>st</sup> (41) | 2<sup>nd</sup> (48) | 2<sup>nd</sup> (52)
|                           **7** | Draw | Draw | 1<sup>st</sup> (41) |                Draw | 1<sup>st</sup> (53) |
|                           **8** | Draw | Draw | 2<sup>nd</sup> (48) | 1<sup>st</sup> (55) |
|                           **9** | Draw | Draw | 1<sup>st</sup> (53) |                Draw |
|                          **10** | Draw | Draw | 2<sup>nd</sup> (60) |
|                          **11** | Draw | Draw |
|                          **12** | Draw |
|                          **13** | Draw |

## Credits

This solver expands on the work of two others:
1. [John Tromp](https://tromp.github.io/c4/c4.html)
1. [Pascal Pons](http://blog.gamesolver.org/solving-connect-four/01-introduction/)
