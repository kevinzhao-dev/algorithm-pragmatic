---
title: FPT for Vertex Cover (bounded search and kernels)
unit: 09 · Approximation and FPT (PS9)
source: 6.046J Lecture 18
pdf: materials/6.046j/lectures/lec18.pdf
read: materials/6.046j/lectures/lec18.pdf
kind: code
minutes: 120
---

# 0902 · FPT: Vertex Cover

**Based on:** [Complexity: Fixed-parameter Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2cf0d5a0689efafd2bf2d13954d8ad9b_MIT6_046JS15_lec18.pdf) · **Read first:** [Complexity: Fixed-parameter Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2cf0d5a0689efafd2bf2d13954d8ad9b_MIT6_046JS15_lec18.pdf)

> **Why this comes up in practice:** data deduplication and conflict-graph repair usually have a small k but an n in the tens of millions. Confining the exponential part to the parameter k lets you solve exactly even when n is huge: `2^k · n` and `n^k` are completely different worlds.

## Your task

The graph is undirected, given as `edges` on vertices `0..n-1`, and it **may have duplicate edges** (the input is guaranteed to have no self-loops).

You build the FPT algorithms for vertex cover from Lecture 18: a bounded search tree, a kernel, the two combined, and finally a minimum cover.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Bounded search tree

```cpp
using EdgeList = std::span<const std::pair<int, int>>;

std::optional<std::vector<int>> vc_search(int n, EdgeList edges, int k);
```

`std::optional<std::vector<int>> vc_search(int n, std::span<const std::pair<int,int>> edges, int k)`: if a vertex cover of size ≤ `k` exists, return one (no repeated vertices, in any order); otherwise return `std::nullopt`. O(2^k·(V+E)) time, and the search depth must not exceed k.

The lecture notes' approach: pick an edge `(u,v)` that isn't covered yet, and try `u` in the cover and `v` in the cover as two separate branches.

## Stage 2 — Kernelization

```cpp
struct Kernel {
    bool no_instance = false;
    int n = 0;
    std::vector<std::pair<int,int>> edges;   // simple, n vertices (no isolated ones)
    int k = 0;
    std::vector<int> forced;                 // original ids, must be in any size-≤k cover
    std::vector<int> to_original;            // to_original[i] = original id of kernel vertex i
};
Kernel kernelize(int n, std::span<const std::pair<int,int>> edges, int k);
```

Rules (apply repeatedly): delete duplicate edges. A vertex of degree `> k` must be in the cover, so add it to `forced`, delete it, and set `k` to `k-1` (`k` going negative ⇒ `no_instance`). Afterwards, if the number of edges is `> k²` ⇒ `no_instance`; otherwise delete isolated vertices and renumber the remaining ones in increasing order of their original ids (so `to_original` is increasing). `(u, v)` and `(v, u)` count as the same edge. The result must satisfy `n ≤ 2k²` and have at most `k²` edges.

## Stage 3 — Kernel plus search

```cpp
std::optional<std::vector<int>> vc_fpt(int n, EdgeList edges, int k);
```

`std::optional<std::vector<int>> vc_fpt(int n, edges, int k)`: kernelize first, then run the bounded search, in O(V + E + 2^k·k²) total. Return a cover in the original vertex ids, of size ≤ k.

## Stage 4 — Real-world extension: a minimum cover

```cpp
std::vector<int> min_vertex_cover(int n, EdgeList edges);
```

`std::vector<int> min_vertex_cover(int n, edges)`: return a minimum vertex cover. Try k upward from a lower bound (iterative deepening), so the whole thing stays fast when OPT is small.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does `f(k)·n^{O(1)}` count as good while `n^{f(k)}` doesn't? Why is having a kernelization equivalent to being FPT?
2. Why is the rule "a vertex of degree > k must be chosen" correct? Why can you reject right away when there are more than k² edges?
3. Can this exercise's `2^k` be improved to `1.47^k`? (Branch on vertices of degree ≥ 3.)
