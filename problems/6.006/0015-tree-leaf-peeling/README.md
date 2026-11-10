---
title: Tree Recognition and Leaf Peeling (turning induction into an algorithm)
unit: 00 · Warm-up (PS0)
source: 6.006 PS0 Problem 0-5
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 60
---

# 0015 · Tree Recognition and Leaf Peeling

**Original problem:** [Problem Set 0 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/60f0a029ab154553cad51b54cc25605b_MIT6_006S20_ps0-questions.pdf)

> **Why this comes up in practice:** network topologies, org charts, and the parent-child relationships in config files are often required to be trees. Plug one extra cable in between two switches and broadcast packets circle the loop forever (a broadcast storm), so the network management system has to find that loop.

## Original problem

6.006 PS0 Problem 0-5 asks for a written proof by induction about undirected graphs: if a graph G = (V, E) is connected and has exactly one edge fewer than it has vertices, |E| = |V| − 1, then G contains no cycle. There is only this one part, and there is no code to write.

## Your task

The original problem asks you to prove by induction that "a connected undirected graph with `|E| = |V| − 1` has no cycle." The heart of the proof: such a graph always has a vertex of degree 1, and removing it shrinks the problem by 1. This exercise writes that inductive process directly as an algorithm; the proof itself is in the interview follow-ups.

A graph is given as `n` (the vertices are numbered `0…n−1`, `n ≥ 1`) and a list of undirected edges. It may contain self-loops (`u == v`) and repeated edges (parallel edges; both of them count). Every function must be **O(V + E)** and must not recurse (on a path graph one million vertices long, recursion would overflow the stack).

You decide whether a graph is a tree, record the leaf-peeling order that the proof describes, and finally find the one cycle in a graph that has a single edge too many, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Three checks

```cpp
struct Edge {
    int u;
    int v;
};

bool is_connected(int n, std::span<const Edge> edges);
bool is_acyclic(int n, std::span<const Edge> edges);
bool is_tree(int n, std::span<const Edge> edges);
```

- `is_connected(n, edges)`: whether the graph is connected.
- `is_acyclic(n, edges)`: whether the graph has no cycle. Self-loops and parallel edges both count as cycles.
- `is_tree(n, edges)`: connected and acyclic.

The tests enumerate every small graph, which also checks the original problem's theorem along the way.

## Stage 2 — Writing the proof as a trace

```cpp
struct Peel {
    int leaf;
    int neighbor;
};

std::optional<std::vector<Peel>> peel_order(int n, std::span<const Edge> edges);
```

`peel_order(n, edges)`: if the graph is a tree, return `n − 1` entries `Peel{leaf, neighbor}` that describe the steps of the inductive proof:

> At each step, `leaf` has degree exactly 1 in the graph **that remains at that point**, and its only neighbor is `neighbor`; remove `leaf` and that edge. Once every step is done, only one vertex is left.

If the graph is not a tree, return `std::nullopt`. When `n = 1` and there are no edges, return an empty list (a single vertex is already a tree). The tests verify the trace entry by entry and don't require any particular order.

## Stage 3 — Real-world extension: find that loop

```cpp
std::vector<int> find_cycle(int n, std::span<const Edge> edges);
```

You are given a graph that is **connected and has exactly n edges** (that is, a tree plus one edge, so it has exactly one cycle). `find_cycle(n, edges)` returns the vertices on the cycle in cycle order; any starting vertex and either direction is fine.

- For a self-loop, return the single vertex; for a pair of parallel edges, return their two endpoints.
- Hint: run Stage 2's peeling on this graph until it can go no further, and **only the cycle is left**.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Finish the original problem's inductive proof out loud: why must a vertex of degree 1 exist? (Hint: the average degree.) After you remove it, why is the graph still connected with `|E| = |V| − 1`?
2. For the same problem, when would you switch to union-find? What can it do that peeling can't?
3. Why is Stage 1's "edges + connected components == vertices" test still correct when there are parallel edges?

---

**Official solution:** [Problem Set 0 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cfc7d2d5bc3b9f4df35b5bdc7aa49284_MIT6_006S20_ps0-solutions.pdf). Read it after you finish.
