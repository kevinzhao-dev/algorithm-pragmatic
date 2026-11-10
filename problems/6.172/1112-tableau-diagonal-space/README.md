---
title: 2N−1 Space Compression and Cache Analysis for the Tableau
unit: 11 · Caching (HW8)
source: 6.172 HW8 §2.1 Write-up 1–2 (iterative tableau)
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec08.pdf, materials/6.172/lectures/lec14.pdf
kind: code
minutes: 60
---

# 1112 · 2N−1 Space Compression and Cache Analysis for the Tableau

**Original problem:** [HW8: Caching and Cachegrind](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5bc7ce0d33656fd3783abcdc3dc4eb92_MIT6_172F18hw8.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf), [Caching and Cache-Efficient Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/329bfc6e1808c375afa517feb3c4c273_MIT6_172F18_lec14.pdf)

> **Why this comes up in practice:** when a DP table (LCS, edit distance, sequence alignment) only needs its last cell, the space can be compressed drastically. The compression in this exercise happens to keep "every cell's neighbors" from being overwritten too early, and it also makes the cache behavior easy to derive; it's a miniature of many stencil computations.

## Original problem

6.172 HW8 §2 takes the tableau-construction problem from Lecture 8: fill an N × N tableau in which each entry is computed from three of its neighbors by an arbitrary function f, A[i][j] = f(A[i − 1][j − 1], A[i][j − 1], A[i − 1][j]). Only the final entry A(N−1, N−1) is wanted, so the iterative version in §2.1 keeps the whole tableau in an array of just 2N − 1 entries, mapping cell (i, j) to index N + i − j − 1. It sets the first row and the first column to an initial value and then fills the remaining cells row by row, left to right. Matrices are row-major, and the tall-cache assumption is B² < αM, where B is the line size, M is the cache size, and α ≤ 1 is a constant.

- Write-up 1: explain why 2N − 1 space is enough and how the iterative function uses it.
- Write-up 2: assuming an optimal replacement strategy and a tall cache, give a tight upper bound, in O notation, on the cache complexity Q(n) for n ≥ cM and for n < cM, where c ≤ 1 is a sufficiently small constant.

## Your task

An N×N tableau only needs `A(N−1, N−1)`, so use just one array `a` of length `2N − 1`: cell `(i, j)` goes at index `N + i − j − 1` (one slot per diagonal). When you fill the table row by row, the cells `(i−1, j−1)`, `(i, j−1)`, and `(i−1, j)` needed to compute `(i, j)` are all still in the right places (think about why).

Besides implementing the iterative fill on the compressed array, you turn Write-up 2's answer into a prediction that the tests check against a cache simulator.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Filling the table (Write-up 1)

```cpp
inline std::size_t diagonal_index(std::size_t i, std::size_t j, std::size_t n);

// Arr exposes size(), get(k), set(k, v) and has exactly 2n - 1 cells.
// f(up_left, left, up) computes one cell.
template <typename Arr, typename F>
double fill_tableau(Arr& a, std::size_t n, double init, F f);
```

- `diagonal_index(i, j, n)`: return the index given by the mapping above.
- `fill_tableau(a, n, init, f)`: `a` has exactly `2n − 1` cells (its only interface is `size()`, `get(k)`, and `set(k, v)`), and you must not allocate any extra array. First set the first row and the first column to `init`, then compute the remaining cells row by row from top to bottom, and left to right within each row, as `f(A(i−1,j−1), A(i,j−1), A(i−1,j))`, and return `A(n−1, n−1)`. When `n == 1`, return `init` directly.

## Stage 2 — Cache analysis (Write-up 2)

```cpp
struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
};

inline double predict_iterative_misses(double n, const CacheGeometry& cache);
```

`predict_iterative_misses(n, cache)`: return the Θ estimate for `fill_tableau` (`cache` is as in 1111, all in elements). Derive it yourself first, then compare it with the simulator. There are two cases:

- `n` smaller than the cache: the cache can hold the working range of a whole row; what happens?
- `n` larger than the cache: does every line a row sweeps over have to be reloaded?

The tests (M = 512 elements, B = 8 elements, fully associative) compare the prediction with the actual count, which must be within a factor of 2 of each other, and also verify how the miss count grows with n: roughly linearly below the cache size (an exponent between 0.8 and 1.2 from n = 64 to 200) and quadratically above it (between 1.8 and 2.2 from n = 700 to 1400).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does this mapping make the space just enough? What would happen if you put `A(i, j)` at index `i + j` (by anti-diagonal)? (Homework Write-up 1)
2. In your analysis, how large must the cache be to hold a row? Did you use the tall-cache assumption?
3. When you parallelize Lecture 8's tableau, can you still use this compressed array? (See 1113.)
