---
title: Trace-Driven Fixed-Block Allocation and Autotuning
unit: 08 · Storage Allocation (HW6)
source: 6.172 HW6 Checkoff 1–2; Recitation 6 Problem 1
pdf: materials/6.172/homework/hw06/handout.pdf
answer: materials/6.172/recitations/r6/solutions.pdf
read: materials/6.172/recitations/r6/problems.pdf
kind: code
minutes: 45
---

# 0811 · Trace-Driven Fixed-Block Allocation and Autotuning

**Original problem:** [HW6: Simple Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e8e29570ad0673ec3460af46536a1e04_MIT6_172F18hw6.pdf) · **Read first:** [R6: Simple, Straightforward Memory Allocation Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/7f2c864d6f81e8f98da02ee657202518_MIT6_172F18_rec6.pdf)

> **Why this comes up in practice:** the most common tuning mistake is to tune a number on a single workload and watch it blow up in production. Whether it's a buffer pool size, an allocator's size classes, or a cache TTL, the process is the same: replay a trace first, then let the machine search, and finally validate on traces it hasn't seen.

## Original problem

6.172 HW6 (Custom Memory Allocators) has you write allocators in C and run them on traces, text files that list a sequence of `malloc()` and `free()` calls. A run is scored on two measures:

- Space utilization U = max{M, 40KB} / max{H, 40KB}, taken at its peak over the run, where M is the memory currently allocated and not yet freed and H is the size of the heap the allocator uses.
- Throughput T, the average number of operations completed per second.

They combine into a performance index P = wU + (1 − w) min{1, T/Tlibc}, where Tlibc is the throughput of libc's `malloc()` on the default traces. The allocator you start from grows the heap on every `malloc()` and does nothing on `free()`, so it is fast but never reuses memory.

§3 switches it to Lecture 11's fixed-size strategy: every block has the same size `BLOCK_SIZE` (1024 bytes by default), and freed blocks go on a singly linked free list whose next pointers are stored inside the free blocks. §4 then makes `malloc()` fail on any request larger than `BLOCK_SIZE` and round every other request up to it, and treats `BLOCK_SIZE` as a parameter for the OpenTuner autotuner to search over the powers of two from 2^5 to 2^15.

- Checkoff 1: implement the fixed-size allocator and show that its utilization on `rec_traces/trace_c0_v0` rises above 99%.
- Checkoff 2: use OpenTuner to find the best `BLOCK_SIZE` for `rec_traces/trace_c0_v0` and for `rec_traces/trace_c1_v0` (which requests only 4096-byte blocks), explain how the tuner arrives at 1024 and 4096 respectively, and add a Makefile target that runs it on a trace file given on the command line.

6.172 Recitation 6 Problem 1 is a true-or-false question: Ben autotunes the parameters of a performance-critical matrix function, using the largest matrix he can find as the only input. Is the tuned function always faster than the untuned original?

## Your task

Here, the homework's OpenTuner is replaced by a search you write yourself. A `Trace` is a sequence of `Alloc(id, size)` / `Free(id)` operations (the equivalent of the homework's trace files).

This exercise **does not actually allocate memory**; it only simulates the bookkeeping (you already wrote a real free list in 0801).

Instead of changing a C allocator and running it on recorded traces, you replay a trace through a simulated fixed-block allocator, compute the homework's metrics, and choose the block size by exhaustive search, first for one trace and then robustly across several.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Replay: fixed-size block allocation

```cpp
struct TraceOp {
    enum class Kind { Alloc, Free };
    Kind kind;
    int id;
    std::size_t size;  // meaningful for Alloc only
};

using Trace = std::vector<TraceOp>;

struct TraceReport {
    std::size_t peak_live_bytes = 0;  // M
    std::size_t heap_bytes = 0;       // H
    std::size_t failed_allocs = 0;
    double utilization = 0.0;  // U
};

inline TraceReport run_fixed_block(const Trace& trace, std::size_t block_size);
inline double performance_index(double utilization, double throughput, double libc_throughput,
                                double weight);
```

`TraceReport run_fixed_block(trace, block_size)`: simulate an allocator whose blocks are all `block_size` bytes.

- `Alloc(id, size)`: if `size > block_size`, it fails (`failed_allocs` goes up by one), and a later `Free` of that `id` must be ignored. Otherwise reuse an idle block first; only when none is idle does the heap grow by `block_size`.
- `Free(id)`: return the block to the idle list (this exercise never needs to shrink the heap).
- `peak_live_bytes`: the maximum, over the whole run, of the total "bytes requested by the user" (M).
- `heap_bytes`: the final heap size (H).
- `utilization` (U): `max(M, 40 KiB) / max(H, 40 KiB)`, where M is the peak above and H is the final heap size. The 40 KiB floor keeps very small traces from being dominated by noise.

In addition, `performance_index(U, T, Tlibc, w)` implements the formula from the homework handout: `w·U + (1 - w)·min(1, T/Tlibc)`.

## Stage 2 — Autotuner

```cpp
inline double objective(const TraceReport& report);
inline std::size_t autotune(const Trace& trace, std::span<const std::size_t> candidates);
```

The homework asks: how does OpenTuner know to pick 1024 and 4096? The answer is the shape of the objective function.

- `objective(report)`: 0 if there are any `failed_allocs` at all, and `utilization` otherwise.
- `autotune(trace, candidates)`: try every candidate size in turn and return the one with the highest objective; on a tie, pick the smallest block. `candidates` may be in any order, and the objective is not necessarily monotonic (don't use binary search).

## Stage 3 — Avoiding overfitting

```cpp
inline std::size_t autotune_robust(std::span<const Trace> traces,
                                   std::span<const std::size_t> candidates);
```

A value tuned on a single trace isn't necessarily right for other traces (Recitation 6 Problem 1).

`autotune_robust(traces, candidates)`: for each candidate, take its **minimum** objective over all the traces, and return the candidate whose minimum is largest (on a tie, pick the smallest block).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. With the homework's objective, a single huge allocation in a trace drags the whole block size up. How would you handle such tail requests in practice? (Hint: 0802's large-allocation path)
2. Once the search space has two or more parameters, brute-force enumeration stops being feasible. What strategies do tools like OpenTuner use? How do you judge whether the result can be trusted?
3. Why does U need a 40 KiB floor? What would happen without it?

---

**Official solution:** [R6 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/3f0e281a9df611606311d6508ca76f7d_MIT6_172F18_rec6sol.pdf). Read it after you finish.
