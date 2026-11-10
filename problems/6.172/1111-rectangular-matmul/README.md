---
title: Cache Analysis of Rectangular Matrix Multiplication
unit: 11 · Caching (HW8)
source: 6.172 HW8 §1 Checkoff 1
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec14.pdf
kind: code
minutes: 70
---

# 1111 · Cache Analysis of Rectangular Matrix Multiplication

**Original problem:** [HW8: Caching and Cachegrind](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5bc7ce0d33656fd3783abcdc3dc4eb92_MIT6_172F18hw8.pdf) · **Read first:** [Caching and Cache-Efficient Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/329bfc6e1808c375afa517feb3c4c273_MIT6_172F18_lec14.pdf)

> **Why this comes up in practice:** real data is rarely square; tall-and-skinny matrices (such as the data matrix in PCA) are extremely common. Different aspect ratios land in different cache regimes, and the analysis has to treat each one separately; this exercise turns those asymptotic analyses into something you can run and check.

## Original problem

6.172 HW8 §1 recalls Lecture 14's cache analysis of n × n matrix multiplication with row-major matrices, on a tall cache of size M with lines of size B. The naive triple loop incurs Θ(n³) misses when n > M/B and Θ(n³/B) when M^{1/2} < n < M/B; blocking with a block size s < M^{1/2} incurs Θ(n³/(B·M^{1/2})); and the cache-oblivious recursion matches blocking without needing the "voodoo" parameter s.

- Checkoff 1: for the product of an m × n matrix and an n × r matrix, under the same tall-cache assumption, analyze the cache complexity in one case of your choice: the naive approach with n > M/B, the naive approach with M/r < n < M/B, the blocked approach, or the cache-oblivious approach.

## Your task

A is m×n, B is n×r, and C is m×r, all row-major. Keep 1102's matrix interface (`rows/cols/get/set`), and accumulate the result into `c`.

The homework asks you to analyze these cases (M = cache capacity, B = line size, both measured in "elements"):

- The naive triple loop (`i`, `j`, `k` running over m, r, and n respectively) when `n > M/B`.
- The same naive loop when `M/r < n < M/B`.
- The cache-oblivious recursion.

Rather than analyzing one case on paper, you implement the naive and recursive products and encode your analysis as predictions that the tests compare with simulated miss counts.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Naive

```cpp
// A is m×n, B is n×r, C is m×r (row-major); the result accumulates into C.
template <typename A, typename B, typename C>
void multiply_rect_naive(const A& a, const B& b, C& c);
```

`multiply_rect_naive(a, b, c)`: `for i in m, for j in r, for k in n`: `c(i, j) += a(i, k) * b(k, j)`.

## Stage 2 — Recursion

```cpp
template <typename A, typename B, typename C>
void multiply_rect_recursive(const A& a, const B& b, C& c);
```

`multiply_rect_recursive(a, b, c)`: halve the largest of the three dimensions m, n, and r (on a tie, prefer m, then n), recurse on the two subproblems, and stop when all three dimensions are 1. How the subproblems are cut:

- Cutting m: split A and C by rows into a top and a bottom subproblem.
- Cutting n: split A by columns and B by rows.
- Cutting r: split B and C by columns.

## Stage 3 — Analysis (Checkoff)

```cpp
struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
    double lines() const;      // provided: M/B
};

inline double predict_naive_misses(double m, double n, double r, const CacheGeometry& cache);
inline double predict_recursive_misses(double m, double n, double r, const CacheGeometry& cache);
```

Write your analysis as two functions; the tests compare them with the actual miss counts from the simulator (M = 512 elements, B = 8 elements):

- `predict_naive_misses(m, n, r, cache)`: return the Θ estimate for the naive loop in the two cases above (ignore constants, but it must be roughly close).
- `predict_recursive_misses(m, n, r, cache)`: return the Θ estimate for the recursive version. Hint: `Θ(mnr/(B√M) + (mn+nr+mr)/B + m+n+r)`.

`CacheGeometry` gives you `capacity_elements` (M), `line_elements` (B), and `lines()` (M/B).

The naive predictions must be within a factor of 2 of the actual counts, and the recursive ones within a factor of 4. To rule out conflict misses (Lecture 14 covered how row-major layout is especially hurt by power-of-2 dimensions), this exercise's simulator is fully associative. The tests also check that the recursion beats the naive loop by more than 10 times on a 32 × 512 × 32 product, where a column of B doesn't fit in the cache, and by more than 2 times on a 128 × 32 × 64 product.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When `n > M/B`, why does the naive loop miss on every access to B? Does it depend on the size of r?
2. A tiled version can only be tuned for one fixed ratio of m, n, and r. Why doesn't the recursive version need to know these ratios?
3. Where does the tall-cache assumption (`B² ≤ cM`) come into this exercise's analysis? What breaks without it?
