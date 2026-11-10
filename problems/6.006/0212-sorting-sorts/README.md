---
title: Choosing a Sort by Cost Model
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 PS2 Problem 2-2
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 45
---

# 0212 · Choosing a Sort by Cost Model

**Original problem:** [Problem Set 2 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/70d9c6b212c6dfce6723259ffb429521_MIT6_006S20_ps2-questions.pdf) · **Read first:** [Lecture 3: Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6d1ae5278d02bbecb5c4428928b24194_MIT6_006S20_lec3.pdf)

> **Why this comes up in practice:** in real systems the cost model is rarely "comparisons and moves cost the same." Writes to flash are expensive, comparing large objects across machines is expensive, and data that is almost sorted already (time series, online updates) is common too. Choosing the right algorithm starts with thinking clearly about one question: **which operation is the expensive one?**

## Original problem

6.006 PS2 Problem 2-2, "Sorting Sorts," describes three scenarios. For each one, pick the best fit among selection sort, insertion sort, and merge sort, and argue for it; the argument counts for more than the pick. The same sort can be the answer to more than one scenario. When more than one sort could work, compare their trade-offs and go with the one that suits the scenario best, and spell out and defend any assumptions. Judge "best" by asymptotic running time.

- (a) A data structure D keeps n items in an "extrinsic order" and offers `D.get_at(i)` in worst-case Θ(1) time and `D.set_at(i, x)` in worst-case Θ(n log n) time. The items must be sorted in place, within D.
- (b) A static array A stores pointers to n comparable objects, and comparing any two of the objects costs Θ(log n) time. Reorder the pointers so that the objects they point to are in non-decreasing order.
- (c) A sorted array A of n integers, each fitting in a single machine word, has been disturbed by log log n swaps of adjacent items, so it is no longer sorted. Restore its sorted order.

The answers are written; there is no code.

## Your task

Each of the problem set's three scenarios makes a different elementary sort (selection, insertion, merge) the winner. This exercise turns the "why" into a budget you can verify: **implement each function with the algorithm you choose; the tests count the expensive operations and fail if you go over budget.** For the selection, insertion, and merge sorts in these three stages, you can adapt your versions from 0202 and 0203 directly.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Writes are expensive

```cpp
template <typename Seq>
void sort_expensive_writes(Seq& seq);
```

A sequence `seq` offers only `size()`, `get_at(i)` (cheap), and `set_at(i, v)` (a write, and very expensive).

`sort_expensive_writes(seq)`: sort `seq` into increasing order with **at most 2(n − 1) calls to `set_at`** (each swap counts as two writes). Elements can be compared and moved/copied. The number of reads is not limited (O(n²) is fine).

## Stage 2 — Comparisons are expensive

```cpp
template <typename T, typename Less>
void sort_pointers(std::span<const T*> pointers, Less less);
```

`sort_pointers(pointers, less)`: `pointers` is a `std::span<const T*>` that points to `n` objects. Rearrange the **pointers** so that the objects they point to are in increasing order according to `less(const T&, const T&)` (the objects themselves don't move, and some may be equal).

Each call to `less` costs far more than moving a pointer (the objects are large, or comparing them goes over the network), so **use at most 1.5·n·⌈log₂ n⌉ + n comparisons**.

## Stage 3 — The data is nearly sorted

```cpp
template <typename T, typename Less = std::less<T>>
void repair_nearly_sorted(std::span<T> items, Less less = {});
```

`repair_nearly_sorted<T>(items, less)` (`less` defaults to `std::less<T>`): `items` is what remains of a sorted array after **an unknown number k** of swaps of adjacent elements. Sort it back into increasing order.

Use at most `2n + 2k` comparisons, which means close to linear time on nearly sorted data (`k` is not passed to you).

## Interview follow-ups

Once you're done, answer without looking at your code:

For every stage you must be able to explain the trade-off; that is the real point of the original problem:

1. Why not use insertion or merge in Stage 1? How expensive does `set_at` have to be before selection sort is worth it?
2. Why merge rather than selection in Stage 2? If comparisons were cheap but you had to stably sort the objects themselves (moving large objects), how would the answer change?
3. In Stage 3, why can one adjacent swap add at most one inversion? What is insertion sort's running time when `k = log log n`?

---

**Official solution:** [Problem Set 2 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/b06b8a8baf9b15f98947e75bae55b8bf_MIT6_006S20_ps2-solutions.pdf). Read it after you finish.
