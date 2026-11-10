---
title: Recursive Parallel Transpose
unit: 06 · Work/Span Theory (HW5)
source: 6.172 HW5 Write-up 5 (CLRS 27.2-5)
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
sanitizer: tsan
minutes: 90
---

# 0615 · Recursive Parallel Transpose

**Original problem:** [HW5: Theory of Performance Engineering](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cd57dbdecbce3bd4da39d620c1e61d14_MIT6_172F18hw5.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf)

> **Why this comes up in practice:** tensor layout conversion (channel-first ↔ channel-last), image rotation, and the bit-reversal step of an FFT all need a transpose. Besides parallelizing well, divide and conquer is naturally cache-oblivious, so the same code performs well in both L1 and L2.

## Original problem

6.172 HW5 Write-up 5 is CLRS Exercise 27.2-5: give an efficient multithreaded algorithm that transposes an n × n matrix in place by divide and conquer, recursively splitting the matrix into four n/2 × n/2 submatrices, and analyze it (work, span, and parallelism). HW5 is a written problem set, so the original asks for pseudocode and an analysis, not a program.

## Your task

You already wrote a parallel-loop transpose in 0501. This exercise uses **divide and conquer**: split the `n×n` matrix into four blocks `A11 A12 / A21 A22`; transpose A11 and A22 in place recursively, and exchange A12 with A21 (transposing them as you swap). The three tasks are independent and can run at the same time, which is why the span is small.

The algorithms are written as `template <typename Exec>`: `exec::Meter` (which computes work/span deterministically, see `_shared/01_exec.hpp`) verifies the complexity, and with `exec::ForkJoin`, TSan guards against races. `exec.charge(1)` for each pair of elements swapped and for each element copied (`fork2` itself charges another 1, so don't count it twice).

The tests turn the exercise's analysis into checks: they compare your exact work and your span against the bounds below, and a second stage adds rectangular, out-of-place transposes.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — In-place transpose

```cpp
template <typename Exec>
void transpose_square(Exec& exec, std::span<int> grid, std::size_t n, std::size_t cutoff = 1);
```

`transpose_square(exec, grid, n, cutoff = 1)`: `grid` is `n×n` and row-major (`grid.size() == n*n`). Subproblems with a side `<= cutoff` switch to a serial loop. With `n` = 0 or 1 the grid stays as it is.

Requirements (with `cutoff = 1`):

- The work is exactly "the number of swaps, `n(n-1)/2`, plus the number of forks."
- The span is `O(lg n)`: the tests require at most `8·⌈lg n⌉ + 16`, and a parallelism (`work / span`) greater than `n/2`.

Hint: the recursion that exchanges the two rectangles must also split in halves, or the span will be `Θ(n)`.

## Stage 2 — Rectangular, out of place

```cpp
template <typename Exec>
void transpose_copy(Exec& exec, std::span<const int> src, std::size_t rows, std::size_t cols,
                    std::span<int> dst, std::size_t cutoff = 1);
```

`transpose_copy(exec, src, rows, cols, dst, cutoff = 1)`: transpose the `rows × cols` matrix `src` (row-major) into `dst` (`cols × rows`, row-major). Recursively split the longer side, and use a serial loop once the area is `<= cutoff`. The work is `rows*cols` plus the number of forks; the span is at most `4·(⌈lg rows⌉ + ⌈lg cols⌉) + 16`. Either dimension may be 0.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Write the work and span recurrences for Stage 1 and solve them (that's exactly what HW5 Write-up 5 asks for). What is the span of the branch that exchanges the rectangles?
2. The loop version (0501) and the recursive version do the same work. What's the difference? (Hint: cache misses, which HW8 measures.)
3. What's a reasonable `cutoff`? How does it relate to the fork cost and to the cache size?
