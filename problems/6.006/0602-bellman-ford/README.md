---
title: Bellman-Ford (detecting and reporting negative cycles)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 Lecture 12, Recitation 12
pdf: materials/6.006/lectures/lec12.pdf
read: materials/6.006/recitations/rec12.pdf
kind: code
minutes: 110
---

# 0602 · Bellman-Ford

**Based on:** [Lecture 12: Bellman-Ford notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/2430d7903a5529451d80c17f89a41fe8_MIT6_006S20_lec12.pdf) · **Read first:** [Recitation 12 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1a34924a2ee4e08c580c2268f8e854ee_MIT6_006S20_r12.pdf)

> **Why this comes up in practice:** the convergence of (distance-vector) routing protocols, detecting currency arbitrage, and solving a system of difference constraints all have Bellman-Ford at their core. When real data comes with negative weights, Dijkstra simply can't be used.

## Your task

The input is a list of edges. Vertices are numbered `0 … n-1`, and weights may be negative. Throw `std::out_of_range` when the source doesn't exist.

```cpp
struct Edge { int from; int to; std::int64_t weight; };
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kNegativeInfinity = std::numeric_limits<std::int64_t>::min();
```

You implement Bellman-Ford, extend it to mark every vertex that a negative cycle can reach, extract a negative cycle, and then use it to find currency arbitrage.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Shortest distances

```cpp
std::vector<std::int64_t> bellman_ford(int n, std::span<const Edge> edges, int source);
```

`bellman_ford(n, edges, s)`: return `dist`, with `kUnreachable` for unreachable vertices. In this stage, no negative cycle is reachable from `s`.

Must run in O(V · E), and must hold up even when the edges come in a terrible order and the graph is very long. The graph may have parallel edges and self-loops (a self-loop with a nonnegative weight is not a negative cycle).

## Stage 2 — How far a negative cycle reaches

Drop the previous stage's guarantee: negative cycles may now be reachable from `s`. Change `bellman_ford(n, edges, s)` so that:

- Whenever `v` can be reached through some negative cycle (`s` can reach that cycle, and the cycle can reach `v`), `dist[v] = kNegativeInfinity`.
- Every other vertex keeps its correct finite `dist`, or `kUnreachable`.

A negative cycle that `s` can't reach doesn't affect the answer.

## Stage 3 — Producing the negative cycle

```cpp
std::optional<std::vector<int>> find_negative_cycle(int n, std::span<const Edge> edges,
                                                    int source);
```

`find_negative_cycle(n, edges, s)`: return a negative cycle reachable from `s`, or `std::nullopt` if there is none. The cycle is a sequence of distinct vertices with an edge between every two adjacent ones and from the last back to the first, and its total weight is < 0, taking the lightest of any parallel edges.

Must run in O(V · E).

## Stage 4 — Real-world extension: currency arbitrage

```cpp
struct Quote { int from; int to; double rate; };  // 1 unit of from buys `rate` units of to
```

```cpp
std::optional<std::vector<int>> find_arbitrage(int currencies, std::span<const Quote> quotes);
```

`find_arbitrage(currencies, quotes)`: find a sequence of currencies such that exchanging once around the loop along `quotes` leaves you with more than you started with (the product of the rates is > 1). Return the sequence of currencies (distinct, exchanged in order, with the last one converting back to the first), or `std::nullopt` if there is no arbitrage opportunity. Throw `std::invalid_argument` if a quote has `rate <= 0`.

- The market isn't necessarily connected, and an opportunity can hide in any corner.
- Rates are floating-point numbers. Some quote tables multiply to exactly 1 in theory, and you must not report a false opportunity because of rounding error; an opportunity whose product exceeds 1 + 1e-6 must be reported. Cases in between are not tested.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why are V - 1 rounds enough for Bellman-Ford? How does that relate to shortest paths being simple paths?
2. Why isn't a vertex that gets updated in round V necessarily on the negative cycle?
3. If you wanted the most profitable arbitrage rather than just any arbitrage, could you still solve this with Bellman-Ford?
