---
title: Cache Complexity of the Recursive Tableau (luck in the k²-way construction)
unit: 11 · Caching (HW8)
source: 6.172 HW8 §2.2 Write-up 4–5
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec14.pdf, materials/6.172/lectures/lec15.pdf
kind: code
minutes: 70
---

# 1114 · Cache Complexity of the Recursive Tableau

**Original problem:** [HW8: Caching and Cachegrind](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5bc7ce0d33656fd3783abcdc3dc4eb92_MIT6_172F18hw8.pdf) · **Read first:** [Caching and Cache-Efficient Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/329bfc6e1808c375afa517feb3c4c273_MIT6_172F18_lec14.pdf), [Cache-Oblivious Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cef17369f91d3140409f2be4ad9246a4_MIT6_172F18_lec15.pdf)

> **Why this comes up in practice:** cache-oblivious guarantees are asymptotic, and the way you split affects "how close" you get. With k too large, if the pieces at some level are just slightly bigger than the cache, the constant factor gets much worse; k = 2 doesn't have this problem. You'll run into this effect again and again when tuning a B-tree's fanout or a chunk size.

## Original problem

6.172 HW8 §2.2 analyzes the cache behavior of the recursive tableau construction. It fills an N × N tableau, in which each entry is f of its upper-left, left, and upper neighbors, inside an array of 2N − 1 entries, by splitting the remaining rows and columns into four quadrants and processing them in the order top-left, top-right, bottom-left, bottom-right. Assume an optimal replacement strategy and a tall cache of size M with line size B.

- Write-up 4: for this 4-way construction, (a) give the recurrence for the cache complexity Q(n); (b) draw the recursion tree, label its internal nodes and leaves with their cache complexity, and give its height; (c) count its leaves; and (d) derive a simplified expression for Q(n) from the tree and the recurrence.
- Write-up 5: for a k²-way construction, show that if you are "unlucky", meaning that a subpiece is just slightly larger than the cache, then Q(n) = Θ(n²k/(MB)), and that if you are lucky and this doesn't happen, then Q(n) = Θ(n²/(MB)).

## Your task

Keep the compressed array and `f` from 1112 and 1113; this time you measure misses with the simulator (`_shared/08_cache_sim.hpp`, fully associative throughout).

Besides deriving the write-ups' bounds, you implement both the 4-way and the k²-way recursion and turn Write-up 4's answer into a prediction that the tests check against the simulator.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — The recursive tableau

```cpp
// Arr has exactly 2n - 1 cells, exposes size(), get(k), set(k, v).
template <typename Arr, typename F>
double recursive_tableau(Arr& a, std::size_t n, double init, F f);
```

`recursive_tableau(a, n, init, f)`: the same four-quadrant recursion as 1113 Stage 1 (top-left, top-right, bottom-left, bottom-right; `mid = begin + len/2`; the range is rows and columns 1..n−1), with `f(A(i-1,j-1), A(i,j-1), A(i-1,j))`; return `A(n-1, n-1)`.

## Stage 2 — Q(n) (Write-up 4)

```cpp
struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
};

inline double predict_recursive_misses(double n, const CacheGeometry& cache);
```

`predict_recursive_misses(n, cache)`: return the Θ estimate for the recursive version (`CacheGeometry`: M and B, both in elements). Hint: once the recursion reaches a piece that fits in the cache, that piece costs only `Θ(M/B)` misses, and there are `Θ((n/M)²)` such leaf pieces. The actual miss count will fall between 0.5 and 12 times the estimate.

These tests use M = 512 elements and B = 8 elements. They also check that at n = 1024 the recursive fill misses more than 30 times less often than a row-by-row fill of the same array, and that from n = 1000 to n = 2000 its misses grow with an exponent between 1.6 and 2.2.

## Stage 3 — k²-way splits and luck (Write-up 5)

```cpp
template <typename Arr, typename F>
double recursive_tableau_k(Arr& a, std::size_t n, std::size_t k, double init, F f);
```

`recursive_tableau_k(a, n, k, init, f)`: split each dimension of the range into k parts (cut points `lo + len * p / k` for p = 0..k, skipping empty pieces), and visit the k² pieces in increasing row order, and within a row in increasing column order. The base case is a 1×1 cell.

Write-up 5 says that if the pieces at some level are just slightly bigger than the cache, you are "unlucky": `Θ(n²k/(MB))`, a factor of k more than the lucky `Θ(n²/(MB))`. The tests compare `Q(n)/n²` at k = 16 for an n that is exactly unlucky and one that is lucky, and confirm that k = 2 doesn't show this effect. They use a small cache of 128 elements with 4-element lines: at k = 16, n = 116 must cost more than twice as many misses per cell as n = 93, while at k = 2 it must cost less than 1.5 times as many.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Write-up 4's full derivation: Q(n)'s recurrence, the height of the recursion tree, the number of leaves, and which term dominates the sum.
2. Write-up 5's unlucky case: for pieces just slightly bigger than the cache, why does a larger k, with smaller sub-pieces inside, actually cause more misses?
3. Lecture 15's merge sort has a similar trade-off: how do you choose k for a k-way merge? What technique does the cache-oblivious funnelsort use to avoid this parameter?
