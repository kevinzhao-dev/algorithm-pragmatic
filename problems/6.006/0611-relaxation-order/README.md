---
title: Pencil-and-Paper Dijkstra as Code (relaxation order and legality checks)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 PS6 Problem 6-1
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec11.pdf, materials/6.006/lectures/lec13.pdf
kind: code
minutes: 60
---

# 0611 · Pencil-and-Paper Dijkstra as Code

**Original problem:** [Problem Set 6 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0cb5c705752261af9da744c22925a136_MIT6_006S20_ps6_questions.pdf) · **Read first:** [Lecture 11: Weighted Shortest Paths notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/aa57a9785adf925bc85c1920f53755a0_MIT6_006S20_lec11.pdf), [Lecture 13: Dijkstra’s Algorithm notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/d819e7f4568aced8d5b59e03db6c7b67_MIT6_006S20_lec13.pdf)

> **Why this comes up in practice:** to judge whether an implementation is correct, the final distances aren't enough; you also have to check that the process follows the algorithm. Writing a trace checker is a good way to really think through an algorithm's invariants, and property-based testing often uses it the same way.

## Original problem

6.006 PS6 Problem 6-1 works by hand on a small DAG whose edge weights are all nonnegative. Its vertices are a–f, and its weighted edges are a→b (2), a→d (6), b→c (1), b→e (3), b→f (2), d→b (1), d→e (0), e→f (2) and f→c (1).

- (a) Starting from a, trace DAG Relaxation and Dijkstra. Both attempt to relax every edge, just in different sequences; write out each algorithm's sequence of attempted edges, called its relaxation order. Break any tie between vertices, or between outgoing edges, alphabetically.
- (b) Give δ(a, v) for every vertex v.

## Your task

The original problem has you work out the relaxation order of two algorithms by hand. Here you write that process as code, and then turn it around and write a checker. The graph types:

```cpp
struct Arc { int to; std::int64_t weight; };
using WeightedGraph = std::vector<std::vector<Arc>>;
using RelaxedEdge = std::pair<int, int>;
struct RelaxationTrace {
    std::vector<RelaxedEdge> order;
    std::vector<std::int64_t> dist;
};
```

`dist` holds the shortest distances (`kUnreachable` when unreachable). `order` lists **every edge that leaves a vertex reachable from `s`**, each exactly once, in the order the edges are tried for relaxation. Throw `std::out_of_range` when the source doesn't exist.

Stages 1 and 2 answer parts (a) and (b) for any input graph, with fixed tie-breaking rules, and Stage 3, the checker, goes beyond the original.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — DAG Relaxation

```cpp
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();

RelaxationTrace dag_relaxation_trace(const WeightedGraph& g, int source);
```

`dag_relaxation_trace(g, s)`: `g` is guaranteed to be a DAG, and weights may be negative. Consider only the vertices reachable from `s` and process them in topological order; when more than one vertex could be processed next, pick the one with the smallest ID. Try the outgoing edges of a vertex in increasing order of `to`.

## Stage 2 — Dijkstra

```cpp
RelaxationTrace dijkstra_trace(const WeightedGraph& g, int source);
```

`dijkstra_trace(g, s)`: weights are guaranteed to be nonnegative (cycles are allowed). The next vertex to process is the unprocessed reachable vertex with the smallest `(distance estimate, ID)`. Try the outgoing edges of a vertex in increasing order of `to`.

## Stage 3 — A legal-order checker

```cpp
bool is_legal_dijkstra_order(const WeightedGraph& g, int source,
                             std::span<const RelaxedEdge> order);
```

`is_legal_dijkstra_order(g, s, order)`: decide whether `order` could be the relaxation order of **some** run of Dijkstra, with any tie-breaking:

- Extracting a vertex is legal whenever its distance estimate is the smallest at that moment (ties may be broken in any order). The outgoing edges of a vertex may come in any order, but they must appear consecutively, each exactly once.
- Every vertex that is reachable from `s` and has outgoing edges must appear; a vertex with no outgoing edges never appears in `order` and doesn't affect the verdict.
- Return `false` if `order` contains a vertex or an edge that doesn't exist.

Must run in O((V + E) log V).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why doesn't DAG Relaxation need to go in order of distance, while Dijkstra must?
2. When two vertices end up at the same distance, does it matter which one is processed first? If there are edges of weight 0, what does your checker need to watch out for?
3. How could you use a trace checker like this in stress testing? Which bugs can it catch that comparing the final distances alone can't?

---

**Official solution:** [Problem Set 6 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9073af3b8151002c5f63d06dc7ac961e_MIT6_006S20_ps6_solutions.pdf). Read it after you finish.
