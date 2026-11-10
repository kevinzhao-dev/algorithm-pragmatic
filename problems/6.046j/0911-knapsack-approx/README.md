---
title: Knapsack (greedy, 2-approx, DP, and FPTAS)
unit: 09 · Approximation and FPT (PS9)
source: 6.046J PS9 Problem 9-1
pdf: materials/6.046j/psets/ps09/questions.pdf
answer: materials/6.046j/psets/ps09/solutions.pdf
read: materials/6.046j/lectures/lec17.pdf
kind: code
minutes: 140
---

# 0911 · Knapsack: From Greedy to FPTAS

**Original problem:** [Problem Set 9](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bb6a432d876819c618c5be099dc5e2cb_MIT6_046JS15_pset9.pdf) · **Read first:** [Complexity: Approximation Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/a4a7f356ba3e65a00ad2bdcfed6e0f35_MIT6_046JS15_lec17.pdf)

> **Why this comes up in practice:** budgeted selection of ad slots, choosing which model checkpoints to deploy, and cache replacement under a capacity limit are all knapsack. The usual engineering path is exactly these four steps: ship a greedy, watch it fail, switch to a 2-approximation, and then trade running time for accuracy with an FPTAS.

## Original problem

6.046J PS9 Problem 9-1 studies approximation algorithms for Knapsack. The input is a set A = {a1, …, an} of items, where item ai has a positive integer size si and a positive integer value vi, plus a positive integer capacity B, with si ≤ B for every i. The goal is a subset of A whose total size is at most B and whose total value is as large as possible. For a subset S, s_S and v_S denote its total size and total value, and Opt denotes an optimal solution.

- (a) Alg1 sorts the items by density vi/si, from highest to lowest, and makes one pass, taking each item that still fits. Prove that Alg1 has no constant approximation ratio: for every positive integer k, some input makes its total value at most v_Opt/k.
- (b) Alg2 takes every item if they all fit together. Otherwise it sorts by density as Alg1 does (renumbering so that the indices follow that order) and finds the smallest i for which a1, …, ai have total size above B, so the first i − 1 items still fit. If vi > v1 + … + v_{i−1}, it returns {ai}; otherwise it returns {a1, …, a_{i−1}}. Prove that Alg2 is a 2-approximation.
- (c) Take the items in any order and let V be the largest value, so nV bounds the value of any solution. For i ∈ {1, …, n} and v ∈ {1, …, nV}, let S_{i,v} be the smallest total size of a subset of {a1, …, ai} whose total value is exactly v, or ∞ if there is none. Give a dynamic program Alg3 that solves Knapsack exactly: a recurrence for every S_{i,v}, how to get the answer from the table, and its running time.
- (d) Alg4 is an FPTAS built on part (c). With the items in any order and V the largest value, for any ε with 0 < ε < 1 it replaces each value vi with the scaled, rounded-down value v′i = ⌊(vi/V)·(n/ε)⌋, runs Alg3 on these values and the original sizes, and outputs the set C that Alg3 returns. Prove that Alg4 is an FPTAS for Knapsack.

## Your task

`Item{size, value}`: both are positive integers. `capacity` is the knapsack capacity `B` (a positive integer); unless stated otherwise, `size ≤ B` is guaranteed. A `Selection` returns the chosen indices (`items`, in increasing order), the total `value`, and the total `size`.

Each stage implements one part, (a) through (d). For part (a) you also build the bad input in code, and every algorithm returns the chosen items, not just their value.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Why density greedy isn't enough (Part a)

```cpp
struct Item {
    long long size = 0;
    long long value = 0;
};

struct Selection {
    std::vector<int> items;
    long long value = 0;
    long long size = 0;
};

Selection greedy_by_density(std::span<const Item> items, long long capacity);
struct DensityTrap {
    std::vector<Item> items;
    long long capacity = 0;
};
DensityTrap density_trap(long long k);
```

`Selection greedy_by_density(std::span<const Item> items, long long capacity)`: go through the items by `value/size` from largest to smallest (ties go to the smaller index) in a single pass, taking each item that fits. Don't use floating point to compare densities (cross-multiply).

`DensityTrap density_trap(long long k)`, with `DensityTrap{items, capacity}`: given `k ≥ 1`, construct an input on which greedy's total value × k ≤ OPT (two items are enough). It must be a valid input for every `k` up to 10⁶: at least one and at most 20 items, every size and value ≥ 1, and every size ≤ `capacity`.

## Stage 2 — 2-approximation (Part b)

```cpp
Selection greedy_two_approx(std::span<const Item> items, long long capacity);
```

`Selection greedy_two_approx(std::span<const Item> items, long long capacity)`: the problem's `Alg2`. If everything fits, take everything. Otherwise sort by density (in the same order as Stage 1), find the first item `a_i` at which the prefix's total size exceeds `B`, and compare `v_i` with the value of the prefix: return `{a_i}` when `v_i > Σ_{j<i} v_j`, and `{a_1, …, a_{i-1}}` otherwise. This guarantees `value ≥ OPT/2`.

## Stage 3 — Exact DP by value (Part c)

```cpp
Selection exact_by_value(std::span<const Item> items, long long capacity);
```

`Selection exact_by_value(std::span<const Item> items, long long capacity)`: the problem's `Alg3`. `S[i][v]` is the smallest total size of a subset of the first `i` items whose value is exactly `v`. O(n²V) time, where `V` is the largest single value. Items with `size > capacity` can simply be ignored.

## Stage 4 — FPTAS (Part d)

```cpp
Selection knapsack_fptas(std::span<const Item> items, long long capacity, double eps);
```

`Selection knapsack_fptas(std::span<const Item> items, long long capacity, double eps)`: the problem's `Alg4`. Scale the values to `v'_i = ⌊v_i · n / (V · ε)⌋`, then run Stage 3 on `v'`. The returned set must respect the **original** size limit, and `value ≥ (1-ε)·OPT`. `0 < ε < 1`. O(n³/ε) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does adding Part b's "compare with the item that no longer fits" to the greedy order give a constant-factor approximation? How does this relate to the fractional knapsack upper bound?
2. Why is Alg3 pseudo-polynomial? When is it better than the O(nB) DP, and when is the O(nB) DP better?
3. Where does the FPTAS's error come from? If you set `ε` to `1/n`, what does the running time become? How does this relate to P vs NP (why isn't this a polynomial-time solution)?

---

**Official solution:** [Solutions to Problem Set 9](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ff14ef9418b742bf99cfa9f52c6accd7_MIT6_046JS15_pset9sols.pdf). Read it after you finish.
