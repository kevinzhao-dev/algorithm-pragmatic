---
title: Approximation Algorithms Toolbox (vertex cover, set cover, partition, TSP)
unit: 09 · Approximation and FPT (PS9)
source: 6.046J Lecture 17, Recitation 9
pdf: materials/6.046j/lectures/lec17.pdf
read: materials/6.046j/lectures/lec17.pdf, materials/6.046j/recitations/rec09.pdf
kind: code
minutes: 130
---

# 0901 · Approximation Algorithms Toolbox

**Based on:** [Complexity: Approximation Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/a4a7f356ba3e65a00ad2bdcfed6e0f35_MIT6_046JS15_lec17.pdf) · **Read first:** [Complexity: Approximation Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/a4a7f356ba3e65a00ad2bdcfed6e0f35_MIT6_046JS15_lec17.pdf), [Complexity: Approximations](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/9233649044c6d4308f0c6ffe4752b9f5_MIT6_046JS15_Recitation9.pdf)

> **Why this comes up in practice:** real systems rarely have room for exact solutions to NP-hard problems. For machine allocation in a cluster, ad slots, and route planning, what people want is "good enough, with a guarantee". This exercise turns a few classic approximation-ratio proofs into code you can check.

## Your task

The tests use brute-force solutions on small instances to check the approximation ratios.

You implement four approximation algorithms from Lecture 17 and Recitation 9, each with its proven guarantee.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — A 2-approximation for vertex cover

```cpp
std::vector<int> approx_vertex_cover(int n, std::span<const std::pair<int, int>> edges);
```

`std::vector<int> approx_vertex_cover(int n, std::span<const std::pair<int,int>> edges)`: return a cover (vertices in increasing order, no duplicates). The algorithm: repeatedly pick an edge neither of whose endpoints has been chosen yet, and add both endpoints to the cover.

This guarantees `|C| ≤ 2·OPT`, in O(V+E).

## Stage 2 — Greedy set cover

```cpp
std::vector<int> greedy_set_cover(int universe, const std::vector<std::vector<int>>& sets);
```

`std::vector<int> greedy_set_cover(int universe, const std::vector<std::vector<int>>& sets)`: repeatedly choose the set that covers the most elements not yet covered, and return the chosen set indices (in increasing order). The input guarantees that the union of all the sets is `0..universe-1`.

This guarantees `|C| ≤ (ln n + 1)·OPT`. Complexity O(Σ|Sᵢ|·log m) (use lazy updates on a heap; don't recompute every set in every round).

## Stage 3 — A PTAS for partition

```cpp
struct TwoWaySplit {
    std::vector<int> side_a;      // indices of items in A, ascending
    long long cost = 0;           // max(w(A), w(B))
};
TwoWaySplit approx_partition(std::span<const long long> weights, double eps);
```

Split the weights into two piles so that the heavier pile is ≤ `(1+ε)·OPT`. Two phases: first find the best split of the largest `m = ⌈1/ε⌉ - 1` items by brute force, then place the remaining items in decreasing order of weight, each one greedily onto the lighter pile. `m` is determined by `eps`, with `eps ∈ (0, 1]`. If there are fewer than `m` items, brute-force all of them (with no items, the cost is 0). The returned `side_a` must be a real set of indices, and `cost` must be consistent with it.

## Stage 4 — The MST 2-approximation for metric TSP

```cpp
std::vector<int> approx_tsp(const std::vector<std::vector<long long>>& dist);
```

`std::vector<int> approx_tsp(const std::vector<std::vector<long long>>& dist)`: `dist` is a symmetric distance matrix that satisfies the triangle inequality. Return a tour that starts at vertex 0 (a sequence of vertices of length n; the edge back to 0 is implied). Follow Recitation 9: take a preorder walk of the MST and short-cut the backtracking.

This guarantees a tour cost ≤ `2·OPT`, in O(V²).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't "max-degree greedy" take this algorithm's place and keep the `2·OPT` bound for vertex cover (see the lecture's counterexample)? Why does the lower-bound argument use a matching?
2. Is the `ln n` bound for set cover tight? Unless P=NP, can anyone do better?
3. Which step of the TSP 2-approximation needs the triangle inequality? Why can't TSP be approximated at all without it (unless P=NP)?
