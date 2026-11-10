---
title: Matrix Multiplication — Loop Order, Tiling, and Cache-Oblivious Recursion
unit: 11 · Caching (HW8)
source: 6.172 Lecture 14; Recitation 8 Problem 3
pdf: materials/6.172/lectures/lec14.pdf
answer: materials/6.172/recitations/r8/solutions.pdf
read: materials/6.172/recitations/r8/problems.pdf
kind: code
minutes: 90
---

# 1102 · Matrix Multiplication: Loop Order, Tiling, and Cache-Oblivious Recursion

**Based on:** [Caching and Cache-Efficient Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/329bfc6e1808c375afa517feb3c4c273_MIT6_172F18_lec14.pdf) · **Read first:** [R8: Caching and Cachegrind Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ea9f0adaa90fd8c13b3cfcd90c419e5d_MIT6_172F18_rec8.pdf)

> **Why this comes up in practice:** for the same O(n³) matrix multiplication, just changing the loop order can make a difference of more than 10 times. Eigen and BLIS are built on these tricks underneath. Tiling needs a "voodoo parameter" tuned to the cache, while the cache-oblivious recursive approach gets a near-optimal miss count with no tuning at all; this exercise measures that with a simulator so you can see it for yourself.

## Original problem

6.172 Recitation 8 Problem 3 analyzes a recursive matrix multiplication, `Rec_Mult`, which adds A·B into C for n × n submatrices of row-major arrays with a given row size, where n is an exact power of 2. When n is 1, it multiplies the two entries and adds the product into C. Otherwise it splits each matrix into four quadrants and makes eight recursive calls of size n/2, in this order: C11 += A11·B11, C11 += A12·B21, C12 += A11·B12, C12 += A12·B22, C21 += A21·B11, C21 += A22·B21, C22 += A21·B12, C22 += A22·B22. Assume a tall cache of size M with lines of size B, where B² ≤ cM for some sufficiently small constant c ≤ 1.

- (a) What recurrence does the cache complexity Q(n), the number of cache misses, satisfy?
- (b) What is the height of the recursion tree for Q(n)?
- (c) How many cache misses occur in the leaves of the tree in total?
- (d) What is the total number of cache misses of the algorithm?

## Your task

Write everything as templates; the matrix types provide only `rows()`, `cols()`, `get(i, j)`, and `set(i, j, v)` (row-major). Results **accumulate into `c`** (`c(i, j) += ...`): `c` is normally zeroed before the call, but one test passes in a nonzero `c` to check that you add to it rather than overwrite it. `a`, `b`, and `c` are all n×n.

The tests count misses with `TracedMatrix` from `_shared/08_cache_sim.hpp` (each `get`/`set` counts as one element access), so the miss counts are exact and reproducible and don't depend on real hardware. In what follows, M is the cache size and B the line size.

Where the recitation only analyzes the recursive algorithm, here you implement it alongside two loop orders and a tiled version, and measure all of them on a simulated cache.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Loop order

```cpp
// A, B, C expose rows(), cols(), get(i, j), set(i, j, v); results accumulate into C.
template <typename A, typename B, typename C>
void multiply_ijk(const A& a, const B& b, C& c);

template <typename A, typename B, typename C>
void multiply_ikj(const A& a, const B& b, C& c);
```

`multiply_ijk(a, b, c)`: the inner `k` loop walks down a column of `b`, which is very unfriendly to row-major storage.

`multiply_ikj(a, b, c)`: swap only the inner two loops, so that the inner loop walks along a row of `b` and a row of `c` while its entry of `a` stays fixed.

On an 8 KiB cache with n = 64, the tests expect `ijk` to miss more than 0.5·n³ times and `ikj` fewer than 2·n³/8 times (8 doubles per line). A `PERF` test also checks that with n = 512 on real hardware, `ikj` is more than 3 times as fast as `ijk`.

## Stage 2 — Tiling

```cpp
template <typename A, typename B, typename C>
void multiply_tiled(const A& a, const B& b, C& c, std::size_t s);
```

`multiply_tiled(a, b, c, s)`: work in `s × s` blocks. The outer three loops `i1, k1, j1`, in that order, step by `s` (up to n), and the inner three loops `i, k, j` run within their tiles. `s` need not divide n, and `s > n` is also valid.

With n = 128 on a 4 KiB cache, the tests expect `s = 8` to cause fewer than 60% of `ikj`'s misses, and `s = 64` to cause more than twice as many misses as `s = 8`.

## Stage 3 — Cache-oblivious recursion

```cpp
// n must be a power of two.
template <typename A, typename B, typename C>
void multiply_recursive(const A& a, const B& b, C& c);
```

`multiply_recursive(a, b, c)` (`n` is guaranteed to be a power of 2): split each matrix in half into four quadrants and recurse 8 times, in the order of Recitation 8 Problem 3's Rec_Mult (`C11 += A11·B11`, `C11 += A12·B21`, `C12 += A11·B12`, `C12 += A12·B22`, `C21 += A21·B11`, `C21 += A22·B21`, `C22 += A21·B12`, `C22 += A22·B22`), with base case `n == 1`.

The tests verify that this version needs no tuning at all: on caches of different sizes, it does almost as well as "a tiling tuned specifically for that cache", while a mistuned tiling loses badly; its miss count also varies with M as `Θ(n³/(B√M))`. Concretely, with n = 128 on a 4 KiB cache, the recursion must stay within 1.4 times the misses of the `s = 8` tiling, while `s = 24` (a tile suited to a 32 KiB cache) must miss more than 2.5 times as often as the recursion. Moving to a 16 KiB cache must cut the recursion's misses by a factor between 1.4 and 2.8, and from n = 64 to n = 128 its misses must grow with an exponent between 2.6 and 3.4.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Derive the recursive version's Q(n) with the method of Recitation 8 Problem 3: the recurrence, the height of the recursion tree, and the total misses in the leaves.
2. Why does ijk miss on almost every access when `n > M/B`, while ikj needs only `Θ(n³/B)`?
3. To actually run fast, the base case won't be 1×1. How do the asymptotic guarantees of cache obliviousness combine with practical optimizations (switching to a BLAS kernel)?

---

**Official solution:** [R8 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/03993e6ee1513a35460c2ffaf7b07fd6_MIT6_172F18_rec8sol.pdf). Read it after you finish.
