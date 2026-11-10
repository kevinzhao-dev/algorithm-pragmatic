---
title: Table Doubling and Binary Counter (amortized analysis)
unit: 04 · Amortization and Randomization (PS4)
source: 6.046J Lecture 5
pdf: materials/6.046j/lectures/lec05.pdf
read: materials/6.046j/lectures/lec05.pdf
kind: code
minutes: 120
---

# 0401 · Table Doubling and Binary Counter

**Based on:** [Amortization: Amortized Analysis](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/13e2c7d165259712327af0af312a068e_MIT6_046JS15_lec05.pdf) · **Read first:** [Amortization: Amortized Analysis](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/13e2c7d165259712327af0af312a068e_MIT6_046JS15_lec05.pdf)

> **Why this comes up in practice:** amortized analysis is behind both `std::vector` and LSM-tree compaction. A single operation can be expensive, but with a good design the total cost of any sequence of operations is linear; with a bad one, you keep triggering reallocations at a boundary.

## Your task

You build two containers from Lecture 5 whose costs only make sense amortized: a dynamic array that grows and shrinks by table doubling, and a log-structured index whose runs behave like a binary counter.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Dynamic array

```cpp
template <typename T>
class DynamicArray {
public:
    DynamicArray();
    DynamicArray(const DynamicArray& other);
    DynamicArray(DynamicArray&& other);  // should be noexcept once implemented
    DynamicArray& operator=(DynamicArray other);
    ~DynamicArray();

    std::size_t size() const;
    std::size_t capacity() const;
    bool empty() const;

    T& operator[](std::size_t index);
    const T& operator[](std::size_t index) const;

    void push_back(T value);
};
```

`DynamicArray<T>`: manage uninitialized raw memory yourself; you must not use `std::vector`. `T` may have no default constructor and no assignment operators, so you can't construct a batch of elements first and assign to them later.

- `push_back(value)`: when the array is full, double the capacity (`0 → 1 → 2 → 4 ...`).
- `operator[]` (without bounds checking), `size()`, `capacity()`, `empty()`.
- Copy construction makes a deep copy; move construction hands the memory over, and the source becomes an empty array with capacity 0. Implement `operator=` with a by-value parameter (copy-and-swap).

**Exception safety:** when growing, if `T`'s move constructor may throw, copy instead (`std::move_if_noexcept`). If a copy fails, destroy the new elements already built, free the new buffer, and leave the array exactly as it was (the strong guarantee). The copy constructor must also handle a copy that fails halfway through.

The tests use a counting element type to confirm that the amortized number of moves per `push_back` is constant, and `Tracked` to catch leaks and double destruction.

## Stage 2 — Shrinking without thrashing

```cpp
// in class DynamicArray<T>
void pop_back();
```

`pop_back()`: throw `std::out_of_range` if the array is empty.

If `size ≤ capacity / 4` after a removal, halve the capacity; this rule leaves the table half full after every reallocation. The special case: when `size` becomes 0, free all the memory and set the capacity back to 0.

`pop_back` **never throws**: if a copy fails while shrinking, keep the old table, and the elements stay as they were.

The tests pop and push repeatedly right at a resize threshold: if you shrink as soon as `size ≤ capacity / 2`, every operation becomes Θ(n).

## Stage 3 — Real-world extension: a log-structured index

```cpp
template <typename T>
class SortedRuns {
public:
    void insert(T value);
    bool contains(const T& value) const;
    std::size_t size() const;
    std::size_t run_count() const;
    std::vector<T> sorted() const;
};
```

`SortedRuns<T>`: keep the data in a set of sorted runs whose sizes are all powers of 2, with at most one run of each size.

- `insert(v)`: add a new run of size 1; whenever two runs of the same size appear, merge them, carrying all the way up, just like incrementing a binary counter.
- `run_count()`: the number of non-empty runs, which always equals the number of 1 bits in `size()`.
- `contains(v)`: binary-search each run, for O(log² n) comparisons.
- `sorted()`: return all the elements in sorted order. Duplicates are allowed.

**Cost requirement:** `n` consecutive calls to `insert` must make `O(n log n)` comparisons in total (the tests measure this with `Counted`).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Analyze table doubling with the potential method: how should you define `Φ`? Why does the shrinking rule use `1/4` rather than `1/2`?
2. The `insert` of `SortedRuns` is exactly a binary counter. Why does the amortized analysis of a binary counter break down if it also supports decrement? Which real LSM-tree problem does that correspond to?
3. What happens when `std::vector` grows if the move constructor is not `noexcept`? Why?
