---
title: Max Subarray and LIS (constrained subproblems)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 Lecture 16, Recitation 16
pdf: materials/6.006/lectures/lec16.pdf
read: materials/6.006/lectures/lec16.pdf, materials/6.006/recitations/rec16.pdf
kind: code
minutes: 90
---

# 0704 · Max Subarray and LIS

**Based on:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf) · **Read first:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf), [Recitation 16 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/487dc976eabd0f9bcd6a3c385203ea27_MIT6_006S20_r16.pdf)

> **Why this comes up in practice:** when Vue 3 reorders a list, it uses LIS to find the nodes that can stay put and moves only the others, so the DOM operations are minimal. Max subarray is the kind of after-the-fact analysis that asks which time window had the worst anomalies. Both problems share the same key idea: a prefix alone isn't enough, so the subproblem must carry one more condition, "must end at the k-th element".

## Your task

Recitation 16 solves max subarray sum and Lecture 16 solves the longest increasing subsequence, both in their basic forms. Here you also recover an LIS, bring it down to O(n log n) comparisons, and use it to reorder a list with as few moves as possible.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Max Subarray

```cpp
struct Subarray { std::int64_t sum; std::size_t first; std::size_t last; };  // inclusive
Subarray max_subarray(std::span<const std::int64_t> values);
```

Return the non-empty contiguous range with the largest sum, in O(n) time. Throw `std::invalid_argument` for empty input. When every value is negative, the answer is the single largest element. When several ranges tie, return any of them.

## Stage 2 — LIS length

```cpp
template <std::ranges::random_access_range R>
std::size_t lis_length(const R& values);
```

`lis_length(values)`: `values` is a `std::ranges::random_access_range` whose elements need only `operator<`. Return the length of the longest **strictly increasing** subsequence. Equal elements don't count as increasing. O(n²) is enough for this stage.

## Stage 3 — Recovering one LIS

```cpp
template <std::ranges::random_access_range R>
std::vector<std::size_t> lis_indices(const R& values);
```

`std::vector<std::size_t> lis_indices(values)`: return the indices of one longest strictly increasing subsequence, from smallest to largest. If several have the same length, any of them is fine.

## Stage 4 — O(n log n)

Both `lis_length` and `lis_indices` must be changed to make at most about `n log₂ n` calls to `operator<` (the tests count them with `Counted`, not with a clock, and allow at most n·(⌊log₂ n⌋ + 3) comparisons).

Hint: maintain "the smallest possible last element of an increasing subsequence of length k". This table is increasing, so you can binary search it. One last element per length isn't enough to recover a solution, so each element must also record which element comes before it.

## Stage 5 — Real-world extension: reordering with the fewest moves

```cpp
template <std::ranges::random_access_range R>
std::vector<std::size_t> keep_in_place(const R& before, const R& after);
```

`before` and `after` are two orderings of the same set of distinct keys, and one operation in turning `before` into `after` is "pick up one element and insert it at any position". Return the indices in `after` of the elements that **don't need to move** (from smallest to largest), keeping as many as possible so that the number of moves, `n - keep`, is as small as possible. Keys must be hashable.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't the subproblem "the LIS length of a prefix" give a recurrence? What does binding it to "ends at k" solve?
2. The Stage 4 table is **not** an LIS itself, so why is its length still correct?
3. Why can't Stage 5 get by with fewer than `n - LIS` moves? If the cost of a move grew with distance, could you still use LIS?
