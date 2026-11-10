---
title: Parallel Reduce and Prefix Scan
unit: 06 · Work/Span Theory (HW5)
source: 6.172 HW5 Write-up 7 (CLRS Problem 27-4)
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
sanitizer: tsan
minutes: 150
---

# 0617 · Parallel Reduce and Prefix Scan

**Original problem:** [HW5: Theory of Performance Engineering](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cd57dbdecbce3bd4da39d620c1e61d14_MIT6_172F18hw5.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf)

> **Why this comes up in practice:** `reduce` and `scan` are the two cornerstones of data parallelism. MapReduce, SQL running totals, and parallel filter/partition are all built on them. Scan is especially interesting: it looks inherently serial (every item depends on the one before it), yet it can be done with linear work and logarithmic span.

## Original problem

6.172 HW5 Write-up 7 is CLRS Problem 27-4, "Multithreading reductions and prefix computations." HW5 is a written problem set, so it asks for pseudocode, proofs, and analysis, not a program. Let ⊗ be an associative operator. The ⊗-reduction of an array x[1..n] is x[1] ⊗ x[2] ⊗ ⋯ ⊗ x[n], and the ⊗-prefix computation (or ⊗-scan) produces the array y[1..n] with y[i] = x[1] ⊗ ⋯ ⊗ x[i]. A single serial loop computes either one, but simply turning the scan's loop into a parallel loop would create races, because each iteration depends on the previous one.

- (a) Use nested parallelism to write P-REDUCE, which computes the ⊗-reduction with Θ(n) work and Θ(lg n) span, and analyze it.
- (b) P-SCAN-1 computes each y[i] independently: a parallel loop calls P-REDUCE on every prefix. Analyze its work, span, and parallelism.
- (c) P-SCAN-2 splits the array in half, scans both halves recursively in parallel, and then, in a parallel loop, combines the last prefix of the left half into every element of the right half. Argue that it is correct, and analyze its work, span, and parallelism.
- (d) P-SCAN-3 makes two passes. An upward pass, P-SCAN-UP, stores the ⊗ of various contiguous subarrays in a temporary array t; a downward pass, P-SCAN-DOWN, hands each subarray a value v and uses t to fill in y. The book's pseudocode leaves out three expressions: what P-SCAN-UP returns, and the values of v that P-SCAN-DOWN passes to its two recursive calls. Fill them in, and argue that P-SCAN-3 is then correct by proving that the v passed for the subarray x[i..j] equals x[1] ⊗ ⋯ ⊗ x[i−1].
- (e) Analyze the work, span, and parallelism of P-SCAN-3.

## Your task

`op` is associative but **not necessarily commutative** (the tests use string concatenation and 2×2 matrix multiplication): in `op(a, b)`, `a` always comes to the left of `b`. The element type is default-constructible and copyable.

Write everything as `template <typename Exec>` (`_shared/01_exec.hpp`): `exec::Meter` measures work/span, and with `exec::ForkJoin`, TSan guards against races. **`exec.charge(1)` once for every call to `op`** (`fork2` charges an extra 1 by itself).

You implement every algorithm in the problem, plus a parallel filter built on scan; the tests check the `op` counts and span bounds that the analysis predicts.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — P-REDUCE

```cpp
template <typename Exec, typename T, typename Op>
T p_reduce(Exec& exec, std::span<const T> x, Op op);
```

`p_reduce(exec, x, op)`: return `x[0] ⊗ x[1] ⊗ … ⊗ x[n-1]`, using divide and conquer with `fork2`. Throw `std::invalid_argument` when `x` is empty; return a single element directly (without calling `op`). The number of `op` calls is exactly `n-1`, and the span is `O(lg n)` (the tests allow at most `3⌈lg n⌉ + 4`).

## Stage 2 — P-SCAN-1

```cpp
template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_1(Exec& exec, std::span<const T> x, Op op);
```

`p_scan_1(exec, x, op)`: return the prefix array `y`, with `y[i] = x[0] ⊗ … ⊗ x[i]`; an empty input returns an empty array. Follow CLRS: a parallel loop in which every item calls `p_reduce` on its own. The number of `op` calls is exactly `n(n-1)/2`, and the span must be at most `6⌈lg n⌉ + 8`.

(This is what you get by making the serial scan parallel in one line: is there a race? No. Is it worth it? The work goes from `Θ(n)` to `Θ(n²)`.)

## Stage 3 — P-SCAN-2

```cpp
template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_2(Exec& exec, std::span<const T> x, Op op);
```

`p_scan_2(exec, x, op)`: divide and conquer. Recursively compute the prefixes of the left half and of the right half, then use `parallel_for` to combine the left half's total into every item of the right half. When `n` is a power of 2, the number of `op` calls is exactly `(n/2)·lg n`; the span is `O(lg²n)` (the tests allow at most `3⌈lg n⌉² + 16`). As in every scan, an empty input returns an empty array.

## Stage 4 — P-SCAN-3 (two passes)

```cpp
template <typename Exec, typename T, typename Op>
std::vector<T> p_scan_3(Exec& exec, std::span<const T> x, Op op);
```

`p_scan_3(exec, x, op)`: CLRS's two-pass version. The up pass stores the accumulated value of each subrange in a temporary array `t`, and the down pass hands down "the accumulated value `v` of every element to the left of this range." **Work out for yourself what `P-SCAN-UP` returns and which `v` the two recursive calls of `P-SCAN-DOWN` must pass** (hint: prove that `v = x[1] ⊗ … ⊗ x[i-1]`). The number of `op` calls is at most `3n`, and the span is `O(lg n)` (at most `6⌈lg n⌉ + 12`). An empty input returns an empty array.

## Stage 5 — Real-world extension: Parallel filter

```cpp
template <typename Exec, typename T, typename Pred>
std::vector<T> parallel_filter(Exec& exec, std::span<const T> x, Pred pred);
```

`parallel_filter(exec, x, pred)`: return the elements for which `pred` is true, in their original order. Use three steps, "flags → scan to get the output positions → write in parallel," so the work is `O(n)` and the span `O(lg n)` (`charge(1)` per element for the flag test, and `charge(1)` per element written to the output). The tests allow at most `6n` charged units in total (flags, scan `op` calls, and writes) and a span of at most `10⌈lg n⌉ + 24`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Analyze the work, span, and parallelism of P-SCAN-1, P-SCAN-2, and P-SCAN-3 (parts b, c, and e of HW5 Write-up 7), and explain why P-SCAN-3 is correct (part d).
2. Why is the work of a scan still `Θ(n)`, yet it can't be less than the serial version's work? How many more `op` calls does P-SCAN-3 make than the serial version?
3. Why can't `parallel_filter` use "one vector per worker, joined at the end"? (The problem from 0513.)
