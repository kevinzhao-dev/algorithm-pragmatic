---
title: Selection Sort, Insertion Sort, and Top-K Tracker
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 Lecture 3, Recitation 3
pdf: materials/6.006/lectures/lec03.pdf
read: materials/6.006/recitations/rec03.pdf
kind: code
minutes: 55
---

# 0202 · Elementary Sorts

**Based on:** [Lecture 3: Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6d1ae5278d02bbecb5c4428928b24194_MIT6_006S20_lec3.pdf) · **Read first:** [Recitation 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1869dbf640ded6b31f1bd369d2001ef5_MIT6_006S20_r03.pdf)

> **Why this comes up in practice:** almost every sort in a standard library switches to insertion sort on small ranges (it is the fastest when there is little data or the data is nearly sorted). Selection sort's "fewest element moves" still matters on storage devices where writes are expensive.

## Your task

Every function below compares with `less(a, b)`, which defaults to `std::less<T>`, and elements are only guaranteed to be movable. `items` is a `std::span<T>`: **sort in place**, without allocating any memory (the tests count allocations). Call them as `selection_sort<int>(values)`.

You write selection sort and insertion sort, then reuse insertion sort's insert step to keep a top-k list.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Selection Sort

```cpp
template <typename T, typename Less = std::less<T>>
void selection_sort(std::span<T> items, Less less = {});
```

`selection_sort<T>(items, less)`: in each round, pick the largest element of the part that isn't sorted yet and swap it into its correct position.

- The **total number of element moves (copy + move) is ≤ 3(n − 1)**, that is, at most `n − 1` swaps (the tests count them with `Probe`).
- Θ(n²) comparisons are fine.

## Stage 2 — Insertion Sort

```cpp
template <typename T, typename Less = std::less<T>>
void insertion_sort(std::span<T> items, Less less = {});
```

`insertion_sort<T>(items, less)`: insert each element backward into the already sorted prefix.

- **Stable**: elements that compare equal keep their input order.
- **It must be fast on nearly sorted data**: let `I` be the number of inversions in the input; then comparisons ≤ `2n + I` and moves ≤ `3(n + I)`. Input that is already sorted takes only O(n), and it must not move any element at all (0 copies and 0 moves): leave an element that is already in place where it is, rather than moving it into a temporary and back.

## Stage 3 — Real-world extension: Top-K Tracker

```cpp
template <typename T, typename Less = std::less<T>>
class TopK {
public:
    explicit TopK(std::size_t k, Less less = {});

    void push(T value);

    std::span<const T> items() const;
    std::size_t size() const;
};
```

`TopK<T, Less>`: keeps receiving data and can answer "the k largest so far" at any time. For a dashboard leaderboard or the top ten search results, when k is small, maintaining a small sorted array directly is the most practical choice (a heap only pays off when k is large; Lecture 8 covers it).

- `explicit TopK(k)`: `k ≥ 1`; otherwise throw `std::invalid_argument`.
- `push(value)`: O(k). Put the new element in place with insertion sort's insert step; once there are more than `k`, evict the smallest. If the new element is not larger than the current k-th place (as judged by `less`), just discard it.
- `items()`: a `std::span<const T>`, **from largest to smallest**. On ties, the one that arrived first comes first (stable).
- `size()`: how many are currently kept (`≤ k`).

Each `push` must move elements at most `k + 8` times, so you can't re-sort everything on every push.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When would you actually choose selection sort? How many times more expensive than a comparison does a write have to be before it pays off?
2. Why is insertion sort stable while this selection sort is not? Give a minimal counterexample.
3. Why does `std::sort`'s introsort switch to insertion sort on small ranges? How is the number of inversions related to insertion sort's running time?
