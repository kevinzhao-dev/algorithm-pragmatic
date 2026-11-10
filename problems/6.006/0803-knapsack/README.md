---
title: 0-1 Knapsack (rolling arrays and choosing between two dimensions)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 Recitation 18
pdf: materials/6.006/recitations/rec18.pdf
read: materials/6.006/recitations/rec18.pdf, materials/6.006/lectures/lec18.pdf
kind: code
minutes: 75
---

# 0803 · 0-1 Knapsack

**Based on:** [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf) · **Read first:** [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf), [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf)

> **Why this comes up in practice:** choosing which experiments to run within a fixed budget of GPU hours, choosing which packages to ship within a weight limit, and admission control for batch training jobs are all knapsack. The capacity is often too large to fill a table with, so knowing whether to use **capacity** or **value** as the DP dimension matters more than memorizing a formula.

## Your task

There are `n` items: item `i` has size `size_i ≥ 1` and value `value_i ≥ 0`. Each item can be taken at most once, the total size must not exceed `capacity`, and the goal is to maximize the total value.

```cpp
struct Item { std::int64_t size; std::int64_t value; };
```

Recitation 18 solves 0-1 knapsack with a table indexed by capacity. Here you also recover the chosen items, roll the table down to O(capacity) memory, and switch to indexing by value when the capacity is huge.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Maximum value

```cpp
std::int64_t knapsack_value(std::span<const Item> items, std::int64_t capacity);
```

`std::int64_t knapsack_value(std::span<const Item> items, std::int64_t capacity)`: O(n·capacity) time. `capacity ≥ 0`. Greedy by value density is wrong.

## Stage 2 — Which items were chosen

```cpp
std::vector<std::size_t> knapsack_items(std::span<const Item> items, std::int64_t capacity);
```

`std::vector<std::size_t> knapsack_items(std::span<const Item> items, std::int64_t capacity)`: return the indices of a set of items that reaches the maximum value, in increasing order; their total size must not exceed `capacity`.

## Stage 3 — Cut `knapsack_value` to O(capacity) memory

The heap high-water mark must stay linear in `capacity` (the tests allow at most 24 bytes per unit of `capacity`). Think through why the direction of the inner loop decides whether an item can be used more than once.

## Stage 4 — Switching dimensions

```cpp
std::int64_t knapsack_by_value(std::span<const Item> items, std::int64_t capacity);
```

The capacity may be as large as 10¹⁵, but the total value is small:

`std::int64_t knapsack_by_value(std::span<const Item> items, std::int64_t capacity)`: O(n·V) time, where `V` is the sum of all the values.

Use "the smallest total size needed to reach value `v`" as the subproblem.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Stage 1 and Stage 4 are both pseudopolynomial; the difference is which number has to be small. How do you decide which one to use?
2. If you reverse the direction of the inner loop in Stage 3, which problem does the result answer?
3. Knapsack has an FPTAS. How does it trade time for accuracy? How does it relate to Stage 4?
