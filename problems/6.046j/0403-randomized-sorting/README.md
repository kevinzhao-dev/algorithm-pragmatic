---
title: Randomized Quicksort and Select
unit: 04 · Amortization and Randomization (PS4)
source: 6.046J Lecture 6, Recitation 4
pdf: materials/6.046j/lectures/lec06.pdf
read: materials/6.046j/lectures/lec06.pdf, materials/6.046j/recitations/rec04.pdf
kind: code
minutes: 90
---

# 0403 · Randomized Quicksort and Select

**Based on:** [Randomization: Matrix Multiply, Quicksort](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/cb55cb123a557eed0738a1187a452c24_MIT6_046JS15_lec06.pdf) · **Read first:** [Randomization: Matrix Multiply, Quicksort](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/cb55cb123a557eed0738a1187a452c24_MIT6_046JS15_lec06.pdf), [Randomization: Randomized Median](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/08020963ae819d4c6dcf177983bb7c6b_MIT6_046JS15_Recitation4.pdf)

> **Why this comes up in practice:** in an online service, if anyone can control the input order, a quicksort with a fixed pivot rule can be driven to O(n²). Randomization turns the worst case from "a property of the input" into "the luck of the dice", and it is also the basis on which `sort` and `nth_element` are implemented.

## Your task

Every function takes a `std::uniform_random_bit_generator` as its source of randomness, so that test results are reproducible. `T` is only guaranteed to support copying, moving, and `operator<`. The input may be all equal (three-way partitioning is required).

You build randomized quicksort, a "paranoid" variant with a guaranteed depth bound, and randomized select, and then use select to find the top k.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Randomized quicksort

```cpp
template <typename T, std::uniform_random_bit_generator Rng>
void randomized_quicksort(std::span<T> values, Rng& rng);
```

`randomized_quicksort(values, rng)`: pick every pivot at random and partition three ways. The expected number of comparisons is O(n log n), for every input.

Recurse on the smaller side and loop on the larger side, so that the recursion depth stays O(log n).

## Stage 2 — Paranoid quicksort

```cpp
template <typename T, std::uniform_random_bit_generator Rng>
int paranoid_quicksort(std::span<T> values, Rng& rng);
```

`paranoid_quicksort(values, rng)`: keep choosing a new pivot until both the less-than part and the greater-than part have at most `3n/4` elements, and only then recurse. Return the **recursion depth** (0 when `n ≤ 1`; otherwise 1 plus the larger of the two subproblems' depths).

Whatever the random numbers are, this depth is at most `⌈log_{4/3} n⌉ + 1`, and the expected number of comparisons is still O(n log n) (each round expects fewer than two retries).

## Stage 3 — Randomized select

```cpp
template <typename T, std::uniform_random_bit_generator Rng>
T randomized_select(std::span<T> values, std::size_t k, Rng& rng);
```

`randomized_select(values, k, rng)`: return the `k`-th smallest element (0-based), and throw `std::out_of_range` if `k` is out of range. Expected O(n).

When it returns, `values` must be **partitioned around rank `k`**: none of the first `k` elements is greater than the answer, and none of the elements after position `k` is smaller. The next stage relies on this.

## Stage 4 — Real-world extension: top-k

```cpp
template <typename T, std::uniform_random_bit_generator Rng>
std::vector<T> top_k(std::span<T> values, std::size_t k, Rng& rng);
```

`top_k(values, k, rng)`: return the `k` largest elements, **sorted from largest to smallest**. Throw `std::out_of_range` if `k` exceeds the number of elements, and return an empty result when `k == 0`. You may rearrange `values`.

The expected number of comparisons must be `O(n + k log k)`, which means that when `k` is much smaller than `n`, you can't sort the whole array. In practice, this is how a leaderboard of the hottest APIs gets recomputed every minute.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why doesn't the analysis of a random pivot need to assume a random input? How does that differ from the "simple uniform hashing" assumption for hash tables?
2. Why is paranoid quicksort easier to analyze than the original? What does choosing the pivot again cost, and is it slower in practice?
3. What is the worst case of `randomized_select`? How do implementations of `std::nth_element` avoid it?
