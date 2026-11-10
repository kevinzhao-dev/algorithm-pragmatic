---
title: Recursive and Parallel Tableau (work and span of the k²-way construction)
unit: 11 · Caching (HW8)
source: 6.172 HW8 §2.2 Write-up 3 (k²-way work/span)
pdf: materials/6.172/homework/hw08/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
minutes: 70
sanitizer: tsan
---

# 1113 · Recursive and Parallel Tableau

**Original problem:** [HW8: Caching and Cachegrind](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5bc7ce0d33656fd3783abcdc3dc4eb92_MIT6_172F18hw8.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf)

> **Why this comes up in practice:** when you turn a DP into a parallel program, the real constraint is the dependences: blocks on the same anti-diagonal can be computed at the same time. Splitting into four quadrants, or into finer k² blocks, changes the span, and with it how many cores you can keep busy.

## Original problem

6.172 HW8 §2.2 makes the tableau construction from Lecture 8 recursive. The tableau is an N × N table whose entry (i, j) is f of its upper-left, left, and upper neighbors, stored in an array of only 2N − 1 entries. The recursive version computes a single cell directly; otherwise it splits the range of rows and columns still to be computed into quadrants (halving a dimension only while it is longer than 1) and processes them in the order top-left, top-right, bottom-left, bottom-right. Once the top-left quadrant is done, the top-right and bottom-left quadrants can run in parallel, which gives work Θ(n²), span Θ(n^{lg 3}), and parallelism Θ(n^{2 − lg 3}); Lecture 8 also showed a more parallel construction that divides the tableau 9 ways.

- Write-up 3: derive general formulas for the work and the span of a k²-way construction, in which the tableau is divided into k² pieces of size n/k × n/k.

## Your task

Keep 1112's compressed 2N−1 array and `f(A(i-1,j-1), A(i,j-1), A(i-1,j))`; `init` still goes into the first row and the first column. `a` has the same interface as in 1112.

Because of `sanitizer: tsan`, this exercise's functional tests run under ThreadSanitizer, so your parallel version must truly have no data races.

Beyond the write-up's formulas, you implement the four-quadrant recursion, run it in parallel with fork-join, and compute the work and span of the k²-way construction in code.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Recursion (four quadrants)

```cpp
// Arr has exactly 2n - 1 cells, exposes size(), get(k), set(k, v).
template <typename Arr, typename F>
double recursive_tableau(Arr& a, std::size_t n, double init, F f);
```

`recursive_tableau(a, n, init, f)`: recursively split the range of cells not yet computed (rows 1..n−1, columns 1..n−1) into four quadrants, top-left, top-right, bottom-left, and bottom-right, and process them in that order. Stop splitting a dimension once it is 1 row or 1 column long (`mid` is `begin + len/2`). Return `A(n−1, n−1)`.

## Stage 2 — Parallel

```cpp
template <typename Arr, typename F>
double parallel_tableau(Arr& a, std::size_t n, double init, F f);
```

`parallel_tableau(a, n, init, f)`: once the top-left quadrant is done, **the top-right and bottom-left quadrants can run at the same time** (with `practice::fork2` from `harness/forkjoin.hpp`), and the bottom-right quadrant runs only after both are done. You may switch back to the serial version once a block is small enough (for example, side length ≤ 32).

The result must be identical to the serial version's. Think about why the two parallel quadrants never write the same slot, nor read a slot that is being written (1112's mapping helps a lot here).

## Stage 3 — Work and span of the k²-way construction (Write-up 3)

```cpp
struct WorkSpan {
    std::uint64_t work = 0;
    std::uint64_t span = 0;
};

inline WorkSpan tableau_work_span(std::size_t n, std::size_t k);
```

`tableau_work_span(n, k)`: with `n = k^h`, split the `n×n` table into k² blocks of side n/k and handle each block recursively; when `n == 1`, work and span are both 1. Blocks on the same anti-diagonal can run in parallel, so there are `2k − 1` phases in all. Return `{work, span}`.

- `work`: the total number of cells.
- `span`: the recurrence `S(n) = (2k−1)·S(n/k)`, `S(1) = 1`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Written as a power of n, the span is `Θ(n^{log_k(2k−1)})`. The larger k is, the higher the parallelism; what is the price? (Homework Write-up 3)
2. If you spawned all the blocks on one anti-diagonal of the k² blocks and then synced them all, how would that differ from using `fork2` on pairs?
3. This exercise is TSan-clean, but what about determinacy races? If `f` has internal shared state, is TSan guaranteed to catch it?
