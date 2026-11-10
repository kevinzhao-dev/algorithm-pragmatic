---
title: Sequence Compaction with Dynamic Adjacency (bracket-style interval DP)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 PS8 Problem 8-2, Recitation 17
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/lectures/lec15.pdf, materials/6.006/recitations/rec17.pdf
kind: code
minutes: 95
---

# 0812 · Sequence Compaction with Dynamic Adjacency

**Original problem:** [Problem Set 8 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/740643e3711574870a83ebfceee9a8ae_MIT6_006S20_ps8_questions.pdf) · **Read first:** [Lecture 15: Dynamic Programming, Part 1: SRBOT, Fib, DAGs, Bowling notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9eb3e9a51a7b5b60b0f67c2277f8b0ee_MIT6_006S20_lec15.pdf), [Recitation 17 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/045bcc2846d6aef7bd77d597d74f213a_MIT6_006S20_r17.pdf)

> **Why this comes up in practice:** LSM-tree compaction, cleaning up tombstones in a log, and eliminating temporary variables from code often take the form "two items can be processed together only after everything between them has been cleared". This is the same shape as RNA secondary structure and matching parentheses: once you decide what the first element pairs with, the sequence splits into an inside part and an outside part that are independent of each other.

## Original problem

6.006 PS8 Problem 8-2 builds on Bowling from Lecture 15, a one-player game on a row of n pins in which pin i has an integer value v_i, possibly negative. The player can knock down a single pin i for v_i points, or two adjacent pins i and i + 1 for v_i · v_{i+1} points. No pin can fall twice, and the player may leave some pins standing. The variant Split Bowling adds a third move, the split: pins i and j > i + 1 can be knocked down together for v_i · v_j points, provided that every pin in {i + 1, …, j − 1} has already fallen. The problem asks for an O(n³)-time algorithm that computes the highest possible Split Bowling score on n given pins.

Like every PS8 problem, it must be solved with a dynamic program presented through the SRT BOT steps, and you must also say whether the requested running time is polynomial, pseudopolynomial, or exponential in the size of the input. PS8 has no coding part.

## Your task

There is a row of records with values `v[i]` (possibly negative). You may repeat the following operations in any order, and you may leave some records unprocessed:

- **Process alone** record `i`: score `v[i]` points and remove it.
- **Process as a pair** records `i < j` that are currently adjacent (everything between them has already been removed): score `v[i]·v[j]` points and remove both.

Each record can be processed at most once. This extends Bowling from Lecture 15 with one more rule: once everything between them has been cleared, two records that weren't originally adjacent can pair up.

This is the original problem with records in place of pins, so Stage 1 answers it. The later stages also require clearing every record, produce a plan that can actually be executed, and reuse the technique on the Wafer Power exercise from Recitation 17.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — The highest score

```cpp
std::int64_t max_compaction_score(std::span<const int> records);
```

`std::int64_t max_compaction_score(std::span<const int> records)`: O(n³) time.

The subproblem needs to carry one extra boolean. First work out the difference between "the stretch in between must be cleared" and "anything goes".

## Stage 2 — The score for clearing everything

```cpp
std::int64_t max_full_compaction_score(std::span<const int> records);
```

`std::int64_t max_full_compaction_score(std::span<const int> records)`: the highest score when every record must be processed (it may be negative). Also O(n³); the states of Stage 1 already compute it along the way.

## Stage 3 — Ordering the operations

```cpp
struct Removal { std::size_t first; std::size_t second; };   // first == second: alone; first < second: pair
struct CompactionPlan { std::int64_t score; std::vector<Removal> removals; };
CompactionPlan compaction_plan(std::span<const int> records);
```

`removals` is a list of operations that **can be executed in order**: when a pair's turn comes, every record between the two must already have been removed. `score` is the highest score, and actually executing the list must earn that score. When several plans tie, any of them is fine.

## Stage 4 — Real-world extension: wafer wiring (the same trick on a different problem)

```cpp
std::vector<std::pair<std::size_t, std::size_t>> max_noncrossing_wires(
    std::span<const bool> is_source);
```

`n` ports are spaced evenly around a circle, and `is_source[i]` says whether port `i` is a power source (`true`) or a computing unit (`false`). Draw as many wires as possible without any two of them crossing; each wire connects one source to one unit, and no wire may connect two ports that are adjacent on the circle (`i` and `i+1`, as well as `0` and `n−1`). Each port gets at most one wire.

`std::vector<std::pair<std::size_t, std::size_t>> max_noncrossing_wires(std::span<const bool> is_source)`: return every wire as `(a, b)` (`a < b`), in O(n³) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is it correct for Stage 1 to split into two independent problems, "the inside enclosed by the pair" and "what remains on the right"? Under what rules would they not be independent?
2. Is this problem polynomial, pseudopolynomial, or exponential? How does the shape of its subproblems differ from Stage 4's?
3. If the condition for a pair changed to "as long as no record in between has a higher value", would this interval DP still work?

---

**Official solution:** [Problem Set 8 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/39b3ead18e480e4322a8a7308daf8fa1_MIT6_006S20_ps8_solutions.pdf). Read it after you finish.
