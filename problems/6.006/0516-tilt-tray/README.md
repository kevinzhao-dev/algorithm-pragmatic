---
title: Tilt Tray (state-space search)
unit: 05 · Graph Search (PS5)
source: 6.006 PS5 Problem 5-6
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 120
---

# 0516 · Tilt Tray

**Original problem:** [Problem Set 5 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/8c155b28c87cc1fe3f506de2036bdf1b_MIT6_006S20_ps5_questions.pdf) · **Read first:** [Lecture 9: Breadth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/196a95604877d326c6586e60477b59d4_MIT6_006S20_lec9.pdf), [Recitation 9 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6afd7e9a85d7d36e204533569f9fccf6_MIT6_006S20_r09.pdf)

> **Why this comes up in practice:** model checkers, test-case generators, and planning problems all look like this: every state of the system is a vertex and every action is an edge, and the graph is far too large to build up front, so you can only generate it as you go.

## Original problem

6.006 PS5 Problem 5-6 studies a puzzle called 6.006 Tilt, played on an n × n board where each cell is empty or holds either an immovable obstacle or a slider. A move tilts the whole board in one of the four directions; each slider travels as far as possible, halting against the border, an obstacle, or a slider that has already stopped, which yields a new configuration. A puzzle (B, t) consists of a board B and a target cell t, and a sequence of k moves solves it if it leaves a slider on t. The handout illustrates this with an example whose optimal solution needs k = 8 moves. The Python code stores a board as n tuples of n characters, indexed B[y][x] with y counting rows from the top, where `'#'` is an obstacle, `'o'` a slider, and `'.'` an empty cell; the target is given as (x, y), and the template supplies `move(B, d)`, which applies one move in O(n²) time.

- (a) Show that a starting board with b obstacles and s sliders can reach no more than C(n, b, s) = (1/s!) · (n² − b)(n² − b − 1)⋯(n² − b − s + 1) configurations.
- (b) If a single move takes B to B′, then B′ is one of B's successors and B is one of B′'s predecessors. Show that a configuration has only r = O(1) successors, yet may have Ω(nˢ) predecessors.
- (c) Give an algorithm that finds a minimum-length solution or reports that the puzzle is unsolvable, in O(n² · min{rᵏ, C(n, b, s)}) time, where k is the optimal number of moves (∞ if no solution exists).
- (d) Implement it as `solve_tilt(B, t)` in the Python template.

## Your task

A tray in an automated warehouse is an `n × n` grid:

```cpp
enum class Tilt { Up, Down, Left, Right };
using Board = std::vector<std::string>;  // n rows of n chars: '#' pillar, 'o' puck, '.' empty
```

A tilt makes **every** puck slide all the way in one direction, until it hits the edge, a pillar, or another puck that has already stopped. Pillars never move. The input is guaranteed to be a valid `n × n` board.

This is the original game with obstacles as pillars and sliders as pucks. Stage 1 implements the move itself, which the original template provides; Stage 2 is the original solver; and Stage 3 counts the configurations and predecessors that parts (a) and (b) bound.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — One tilt

```cpp
Board apply_tilt(const Board& board, Tilt direction);
```

`apply_tilt(board, direction)` returns the new board after the tilt, in O(n²). (n gets large enough that simulating the slide one cell at a time is too slow.)

## Stage 2 — The fewest moves

```cpp
std::optional<std::vector<Tilt>> solve_board(const Board& start, int row, int col);
```

`solve_board(start, row, col)`: return the shortest sequence of tilts that brings **any** puck onto cell `(row, col)` (an empty sequence if that cell of `start` already holds a puck), or `std::nullopt` if it can't be done. Throw `std::out_of_range` if `(row, col)` is not on the board.

When there is more than one shortest sequence, returning any of them is correct.

If the shortest solution takes `k` moves and one move costs `O(n²)`, your running time must be **O(n² · min(4^k, C))**, where `C` is the number of reachable boards. In other words, when the solution is shallow you must not compute the whole state space, and when it is deep or doesn't exist you must still stop in finite time.

## Stage 3 — Analyzing the state space

```cpp
std::size_t reachable_configurations(const Board& start);

std::uint64_t count_predecessors(const Board& board, Tilt direction);
```

- `reachable_configurations(start)`: how many different boards can be reached from `start` with any number of tilts (`start` included).
- `count_predecessors(board, direction)`: how many boards (with the same pillars and the same number of pucks) turn into **exactly** `board` when tilted in `direction`? If tilting `board` itself would change it, the answer is 0. Return a `std::uint64_t`; the tests guarantee that the answer fits.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can the number of reachable boards never exceed `C(n² − b, s)` (where `b` is the number of pillars and `s` the number of pucks)? How far off is that bound in practice?
2. A board has at most 4 successors but can have Ω(nˢ) predecessors. What does that mean for searching backward from the goal board?
3. What should each entry of the visited set store? If boards get large, how would you compress the state, and what would you use as the hash key?

---

**Official solution:** [Problem Set 5 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/23d4cf5421c77bb55b3735f7e8f70dfe_MIT6_006S20_ps5_solutions.pdf). Read it after you finish.
