---
title: Distance-Vector Routing (shortest paths that converge in a few hops)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 PS6 Problem 6-2
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec12.pdf, materials/6.006/recitations/rec12.pdf
kind: code
minutes: 70
---

# 0612 · Distance-Vector Routing

**Original problem:** [Problem Set 6 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0cb5c705752261af9da744c22925a136_MIT6_006S20_ps6_questions.pdf) · **Read first:** [Lecture 12: Bellman-Ford notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/2430d7903a5529451d80c17f89a41fe8_MIT6_006S20_lec12.pdf), [Recitation 12 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1a34924a2ee4e08c580c2268f8e854ee_MIT6_006S20_r12.pdf)

> **Why this comes up in practice:** distance-vector protocols (the RIP family) are Bellman-Ford with routing tables exchanged round by round, and real networks have small diameters, so they converge within a few rounds. "The cheapest flight with at most k connections" is the same kind of problem, and writing it a slightly different way gives wrong answers.

## Original problem

6.006 PS6 Problem 6-2 takes a weighted directed graph G = (V, E, w) and a source s ∈ V, and promises that each vertex v ∈ V can be reached from s by a minimum-weight path of at most k edges.

Give an O(|V| + k|E|)-time algorithm that computes δ(s, v) for every vertex v.

## Your task

The input is a list of edges over `n` vertices (`0 … n-1`), and weights may be negative. Throw `std::out_of_range` when the source doesn't exist.

```cpp
struct Edge { int from; int to; std::int64_t weight; };
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
```

Stage 1 is the original problem, and `k` is not given to you. Stage 2 caps the number of hops instead, and Stage 3 measures `k` itself.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Converging without knowing k

```cpp
std::vector<std::int64_t> converge_routes(int n, std::span<const Edge> edges, int source);
```

Guarantee: for every vertex `v` reachable from `s`, some min-weight path uses at most `k` edges. **You are not told `k`**, but it may be far smaller than V.

`converge_routes(n, edges, s)`: return `dist`, with `kUnreachable` for unreachable vertices. Must run in O(V + k · E): when the graph has millions of edges and `k` is only a few dozen, you can't run all V rounds.

## Stage 2 — A hop limit

```cpp
std::vector<std::int64_t> hop_limited_routes(int n, std::span<const Edge> edges, int source,
                                             int hops);
```

`hop_limited_routes(n, edges, s, hops)`: return the minimum weights using at most `hops` edges. **Stage 1's guarantee does not hold here**, there may be negative cycles, and a route may revisit vertices (it is a walk). When `hops == 0`, only `s` is 0, and every other vertex is `kUnreachable`.

Must run in O(hops · (V + E)).

## Stage 3 — Measuring the hops needed to converge

```cpp
int convergence_hops(int n, std::span<const Edge> edges, int source);
```

`convergence_hops(n, edges, s)`: under the same guarantee as Stage 1, return the smallest `k` such that every reachable vertex has a shortest path that uses at most `k` edges. Return 0 when `s` is the only reachable vertex.

Must run in O(V + k · E).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can Stage 1 stop as soon as a round makes no updates? If you already knew `k`, what work could you skip?
2. Why can't Stage 2 simply run Stage 1's loop for `hops` rounds? Give a small concrete example.
3. Why does real RIP have a hop limit? What does count-to-infinity have in common with the negative cycles here?

---

**Official solution:** [Problem Set 6 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9073af3b8151002c5f63d06dc7ac961e_MIT6_006S20_ps6_solutions.pdf). Read it after you finish.
