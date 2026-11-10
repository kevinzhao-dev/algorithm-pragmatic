---
title: Parallel Quicksort and Fixing a Race
unit: 05 · Fork-Join and Reducers (HW4)
source: 6.172 HW4 §2.2–2.3 (Checkoff 4–5)
pdf: materials/6.172/homework/hw04/handout.pdf
read: materials/6.172/lectures/lec07.pdf
kind: code
sanitizer: tsan
minutes: 100
---

# 0511 · Parallel Quicksort and Fixing a Race

**Original problem:** [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf) · **Read first:** [Races and Parallelism](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/543e419bff1ed05b1a649115c24c44b3_MIT6_172F18_lec7.pdf)

> **Why this comes up in practice:** almost every first version of a hand-rolled parallel algorithm has a determinacy race, and most of the time it "seems to work." Being able to use ThreadSanitizer, read its report, and point out which two accesses collide matters more than memorizing any lock API.

## Original problem

6.172 HW4 §2.2–2.3 continue the Cilk recitation with two tools:

- §2.2, the Cilksan race detector: a quicksort was parallelized naively by adding `cilk_spawn` and `cilk_sync` to a serial version, and it has a determinacy race that test runs rarely expose. Built with Cilksan, which reports the source lines of the two conflicting memory accesses (often a read and a write), it shows the race on an input of just 10 elements.
- §2.3, the Cilkscale scalability analyzer: a second parallel quicksort should have no races (check with Cilksan). Built with Cilkscale, it reports work and span from measured cycle counts, and the parallelism they imply.

The checkoff items:

- **Checkoff Item 4:** find the race with Cilksan, fix it, and confirm with Cilksan that no races remain. Report the line numbers of the racing read and write, and briefly explain what causes the race.
- **Checkoff Item 5:** report the parallelism that Cilkscale computes for the race-free quicksort on a few input sizes.

The original is C with Cilk.

## Your task

Look at the race first:

```bash
D=problems/6.172/0511-parallel-quicksort
clang++ -std=c++20 -g -O1 -fsanitize=thread -fno-omit-frame-pointer \
  -Iharness -I$D/support $D/lab/race_lab.cpp -o /tmp/race_lab
/tmp/race_lab 2>&1 | head -30
```

Read the TSan report (a racy program often crashes right afterwards; that's a consequence of the race, and the report already tells you enough): which two addresses, and which two strands touched them at the same time? Where is the bug in `support/racy_quicksort.hpp`? Think it through before you read on.

The algorithms in this exercise are all written as `template <typename Exec>`, and the tests use both `exec::Meter` (`problems/6.172/_shared/01_exec.hpp`: computes work/span deterministically) and `exec::ForkJoin` (really parallel, guarded by TSan).

Here ThreadSanitizer replaces Cilksan and `exec::Meter` stands in for Cilkscale: after finding the race, you write a race-free parallel quicksort and then coarsen it.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Partition

```cpp
template <typename T, typename Compare = std::less<>>
std::size_t partition_around(std::span<T> data, std::size_t pivot_index, Compare cmp = {});
```

`partition_around(data, pivot_index, cmp)`: use `data[pivot_index]` as the pivot, partition, and return the pivot's final position `p`, such that `data[p]` is the original pivot, everything to its left is `<= pivot`, and everything to its right is `>= pivot` (according to `cmp`). `data` is a `std::span<T>`, and `T` is comparable.

## Stage 2 — A race-free parallel quicksort

```cpp
template <typename Exec, typename T, typename Compare = std::less<>>
void quicksort(Exec& exec, std::span<T> data, Compare cmp = {});
```

`quicksort(exec, data, cmp = {})`: choose as the pivot "the median of `first`, `middle`, and `last`" (the tests rely on this to make the Meter results reproducible). The two sides of the recursive calls must be **disjoint** (neither one includes the pivot's position). `charge(1)` for each element visited and compared, so partitioning `n` elements costs `n`.

Sorting with `exec::ForkJoin` produces no TSan report; with `exec::Meter` you can see the parallelism (work/span). That's exactly the number HW4 Checkoff 5 asks for: plot it against `n`. Why isn't it linear?

What the Meter tests check: 50,000 already-sorted elements cost less than `20n` work (that's what median-of-three buys you), and on `2^16` random elements the work stays below `40·n·lg n`, the span is at least `n` (the top-level partition is serial), and the parallelism is above 3 and larger than for `2^10` elements.

## Stage 3 — Coarsening

```cpp
template <typename Exec, typename T, typename Compare>
void quicksort(Exec& exec, std::span<T> data, Compare cmp, std::size_t cutoff);
```

`quicksort(exec, data, cmp, cutoff)`: sort subarrays of length `<= cutoff` with insertion sort (still charging the number of comparisons) instead of forking further. The number of forks must drop noticeably as `cutoff` grows: on `2^14` elements, `cutoff = 64` must make fewer than `4n/64 + 32` forks, and under a quarter of the forks that `cutoff = 1` makes.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Is HW4's qsort-race really a race, or is it benign? Why does "it looks right" still not count under TSan?
2. Does median-of-three give an O(n) span on the Meter? On what inputs does it degrade? How would you make the span really go down? (Hint: parallel partition.)
3. Why can't the split around the pivot here be "left side `<=`, right side `>=`, with both sides including elements equal to the pivot"?
