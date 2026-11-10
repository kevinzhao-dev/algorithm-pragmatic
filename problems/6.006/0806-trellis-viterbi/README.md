---
title: Trellis DP, Viterbi, and Board Placement (state augmentation)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 Lecture 17, Recitation 17
pdf: materials/6.006/lectures/lec17.pdf
read: materials/6.006/lectures/lec17.pdf, materials/6.006/recitations/rec17.pdf
kind: code
minutes: 100
---

# 0806 · Trellis DP, Viterbi, and Board Placement

**Based on:** [Lecture 17: Dynamic Programming, Part 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/665523227a175e9e9ce26ea8d3e5b51c_MIT6_006S20_lec17.pdf) · **Read first:** [Lecture 17: Dynamic Programming, Part 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/665523227a175e9e9ce26ea8d3e5b51c_MIT6_006S20_lec17.pdf), [Recitation 17 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/045bcc2846d6aef7bd77d597d74f213a_MIT6_006S20_r17.pdf)

> **Why this comes up in practice:** Viterbi decoding for an HMM, inference in a CRF, and decoding in speech recognition all turn into a shortest path on a trellis once you lay out "several possible states at each time step, with a cost between adjacent states". Piano fingering from Lecture 17 has the same shape. Write a generic solver first; after that, each problem only needs the right definition of its state.

## Your task

**Trellis:** there are `positions` time steps, and at each time step you pick one of the `states` states, `s_i`. The total cost is

`Σ node(i, s_i) + Σ edge(i, s_i, s_{i+1})` (`i` starts at 0, and `edge` is the cost between time steps `i` and `i+1`).

The cost type is whatever `node` returns (an integer type or `double`).

Lecture 17 and Recitation 17 solve piano fingering and a ship-placement game on a 2×n board, each with its own DP. Here you write one generic trellis solver and then reuse it for Viterbi decoding and for the ship placement.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Minimum cost

```cpp
template <typename NodeCost, typename EdgeCost>
auto min_trellis_cost(std::size_t positions, std::size_t states, NodeCost&& node, EdgeCost&& edge);
```

Return `Cost{}` when `positions == 0`, without calling `node` or `edge`; throw `std::invalid_argument` when `states == 0` (and `positions > 0`). Call each `edge(i, s, t)` at most once: no more than `(positions − 1) · states²` calls in total (the tests count them, so you can't recompute any).

## Stage 2 — Recovering the state sequence

```cpp
template <typename NodeCost, typename EdgeCost>
std::vector<std::size_t> best_trellis_path(std::size_t positions, std::size_t states,
                                           NodeCost&& node, EdgeCost&& edge);
```

`std::vector<std::size_t> best_trellis_path(positions, states, node, edge)`: return the state at each time step. The total cost must be the minimum (break ties however you like), and the same limit on calls applies.

## Stage 3 — Cut `min_trellis_cost` to O(states) memory

The heap high-water mark must not depend on `positions` (with 200,000 positions and 4 states, the tests allow at most 4 KiB).

## Stage 4 — Viterbi

```cpp
struct Hmm {
    std::vector<double> start;                      // start[s] = P(first state is s)
    std::vector<std::vector<double>> transition;    // transition[s][t] = P(t next | s now)
    std::vector<std::vector<double>> emission;      // emission[s][o] = P(observe o | s)
};

std::vector<std::size_t> viterbi_decode(const Hmm& hmm, std::span<const std::size_t> observations);
```

Return the state sequence with the highest joint probability. Return an empty vector for empty `observations`. On long sequences the product of probabilities underflows, so you must compute in the log domain instead, and handle transitions with probability 0 correctly (with the solver from Stages 1–2, this should take only a few lines).

## Stage 5 — Real-world extension: placement on a 2×n board

A `2 × n` board has an integer value in each cell (possibly negative). You may place any number of non-overlapping `1×2` ships (horizontal or vertical); the score is the sum of the values of the covered cells, and placing no ships at all is also allowed.

```cpp
struct Ship { std::size_t row; std::size_t column; bool vertical; };
struct ShipPlan { std::int64_t score; std::vector<Ship> ships; };

ShipPlan best_ship_plan(std::span<const int> top, std::span<const int> bottom);
```

`top` and `bottom` must have the same length (otherwise throw `std::invalid_argument`). A `Ship` covers `(row, column)` and `(row, column + 1)` (horizontal), or `(0, column)` and `(1, column)` (vertical, in which case `row == 0`).

Turn it into the trellis from Stages 1–2: `positions = n + 1`, and the state at time step `x` is "which cells of column `x` are already occupied by horizontal ships coming from the left" (a bitmask from 0 to 3). O(n) time (the number of states is a constant).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Is a problem on a trellis the same thing as "finding a shortest path in a layered DAG"? How does it relate to the DAG relaxation you learned in an earlier unit?
2. Why is the number of `edge` calls proportional to `states²`? If transitions only happened between neighboring states (for example, a finger moving by only one position), what could you save?
3. Why does the state in Stage 5 only need one column? If the board were `10 × n`, how would broken-profile DP extend this idea?
