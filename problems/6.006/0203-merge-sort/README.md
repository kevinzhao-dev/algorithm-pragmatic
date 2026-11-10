---
title: Merge Sort and Inversion Counting
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 Lecture 3, Recitation 3
pdf: materials/6.006/lectures/lec03.pdf
read: materials/6.006/recitations/rec03.pdf
kind: code
minutes: 70
---

# 0203 · Merge Sort

**Based on:** [Lecture 3: Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6d1ae5278d02bbecb5c4428928b24194_MIT6_006S20_lec3.pdf) · **Read first:** [Recitation 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1869dbf640ded6b31f1bd369d2001ef5_MIT6_006S20_r03.pdf)

> **Why this comes up in practice:** `std::stable_sort`, external sorting (when the data doesn't fit in memory), and LSM-tree compaction are all merges. Quantifying "how different two rankings are" with an inversion count (Kendall tau) is something recommender-system evaluation does every day.

## Your task

Every comparison uses `less(a, b)`, which defaults to `std::less<T>`.

You write a stable merge, then a merge sort that allocates its scratch space only once, and finish by counting inversions to measure the distance between two rankings.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Merge

```cpp
template <typename T, typename Less = std::less<T>>
std::vector<T> merge_sorted(std::span<const T> a, std::span<const T> b, Less less = {});
```

`merge_sorted<T>(a, b, less)`: `a` and `b` are both sorted (`std::span<const T>`); return the merged, sorted `std::vector<T>`.

- At most `|a| + |b|` comparisons.
- **Stable**: when elements compare equal, the element from `a` goes before the element from `b`.

## Stage 2 — Merge Sort

```cpp
template <typename T, typename Less = std::less<T>>
void merge_sort(std::span<T> items, Less less = {});
```

`merge_sort<T>(items, less)`: sort `items` (a `std::span<T>`) in place (you may use extra space, but when the call returns, `items` is sorted). Elements are only guaranteed to be movable; they need not be default-constructible.

- **Stable**.
- At most `1.5 · n · ⌈log₂ n⌉ + n` comparisons (the worst case of pairwise merging; the tests check it by counting).
- **The whole sort allocates scratch space only once** (at most 2 heap allocations, counted by the tests): don't create a new `vector` in every recursive call.

Hint: a single buffer of length `n` is enough. Write a helper that sorts `items[lo..hi)`; to merge, move the left half into the buffer, then merge from the buffer and the right half back into place (or merge straight into the buffer and move the result back).

## Stage 3 — Real-world extension: ranking distance

```cpp
std::uint64_t count_inversions(std::span<const int> values);

std::uint64_t kendall_tau_distance(std::span<const int> rank_a, std::span<const int> rank_b);
```

- `count_inversions(values)`: the number of pairs in `values` with `i < j` and `values[i] > values[j]`, in **O(n log n)**; a brute-force O(n²) solution is not allowed. The answer is a `uint64_t`.
- `kendall_tau_distance(rank_a, rank_b)`: `rank_a` and `rank_b` are both permutations of `0…n−1` that represent two rankings (for example, how the old and new versions of a search algorithm order the same set of results). Return "the number of pairs that the two rankings put in opposite orders." If the lengths differ, or either one is not a permutation, throw `std::invalid_argument`.

Hint: during a merge, every time an element of the right half comes out ahead of the elements still left in the left half, add to the inversion count.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Merge sort's recursion depth is O(log n); why isn't stack usage a problem? Where is the real extra space? Can you merge in place, and what does that cost?
2. Write down merge sort's recurrence, and explain which case of the Master Theorem applies.
3. How is the number of inversions related to the number of swaps insertion sort makes (compare 0202 and 0212)?
