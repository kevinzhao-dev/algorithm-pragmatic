---
title: Subset Sum, Bitset, and Partition (pseudopolynomial)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 Lecture 18, Recitation 18
pdf: materials/6.006/lectures/lec18.pdf
read: materials/6.006/lectures/lec18.pdf, materials/6.006/recitations/rec18.pdf
kind: code
minutes: 90
---

# 0802 · Subset Sum, Bitset, and Partition

**Based on:** [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf) · **Read first:** [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf), [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf)

> **Why this comes up in practice:** splitting work evenly between two machines and hitting an exact combination within a budget are both subset sum. O(n·T) looks unremarkable, but T is the size of a number, not the length of the input, which is why an NP-hard problem can still have a "fast" algorithm. In real code, treating the bits as parallel processing units (word-level shifts) often makes it another 64 times faster.

## Your task

Lecture 18 decides subset sum with an O(n·T) table, and Recitation 18 poses partition and negative numbers as exercises. Here you also find a witness in O(target) memory, pack the reachable sums into a word-level bitset, find the closest partition, and handle negative values.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Decision

```cpp
bool subset_sum(std::span<const int> values, int target);
```

`bool subset_sum(std::span<const int> values, int target)`: all `values` are positive and `target ≥ 0`. Decide whether some subset (possibly the empty set) sums to exactly `target`. O(n·target) time.

Each element can be used at most once.

## Stage 2 — Finding one subset

```cpp
std::optional<std::vector<std::size_t>> subset_sum_witness(std::span<const int> values,
                                                           int target);
```

`std::optional<std::vector<std::size_t>> subset_sum_witness(std::span<const int> values, int target)`: return a set of indices in increasing order whose values sum to exactly `target`, or `std::nullopt` if there is none. Also O(n·target) time.

The goal here is O(target) memory rather than O(n·target); the tests allow at most 32 bytes of heap per unit of `target`. Think about what "the element being processed when `sum` first becomes reachable" can tell you.

## Stage 3 — Word-level bitset

```cpp
class SumSet {
public:
    explicit SumSet(std::size_t limit);       // tracks sums in [0, limit]; starts as {0}
    void add(std::size_t value);              // include one more item; O(limit / 64)
    bool contains(std::size_t sum) const;     // O(1); sums above limit are false
    std::size_t count() const;                // how many sums in [0, limit] are reachable
};
```

`add` must not work bit by bit: use an array of `std::uint64_t` and "shift the whole thing left, then OR". Handle `value` being an exact multiple of 64, `value > limit`, and `limit` landing exactly on a word boundary. Memory must stay within a constant multiple of `limit / 8` bytes (the tests allow twice that, plus 4 KiB).

## Stage 4 — The closest split in two

```cpp
struct Partition { std::int64_t difference; std::vector<std::size_t> left; };
Partition closest_partition(std::span<const int> values);
```

Split `values` (all positive) into two groups so that the difference between their sums is as small as possible. `left` holds the indices of one group (in increasing order), and `difference` is the absolute difference between the two sums. O(n·Σvalues) time.

## Stage 5 — Allowing negative values

```cpp
bool subset_sum_signed(std::span<const int> values, std::int64_t target);
```

`bool subset_sum_signed(std::span<const int> values, std::int64_t target)`: `values` may include negative numbers and 0, and the empty subset sums to 0. O(n·(Σ|values|)) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why isn't O(n·T) polynomial? When the number of bits in `T` grows by 1, how does the running time change? When is it actually very useful?
2. Why is the "first reachable" information from Stage 2 enough to recover a solution? If the elements were processed in an arbitrary order, would the solution still be correct?
3. If there are only a few `values` but the numbers are huge (`n = 40`, `T = 10^12`), what approach would you switch to? (Meet in the middle.)
