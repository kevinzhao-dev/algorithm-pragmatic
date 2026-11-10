---
title: Tuning Merge Sort Step by Step
unit: 02 · Profiling (HW2)
source: 6.172 HW2 §3 Homework — Sorting (Write-up 1–7)
pdf: materials/6.172/homework/hw02/handout.pdf
read: materials/6.172/homework/hw02/handout.pdf, materials/6.172/lectures/lec02.pdf
kind: perf
minutes: 240
---

# 0211 · Tuning Merge Sort Step by Step

**Original problem:** [HW2: Profiling](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/796439e646c02f44348d50b1836ff7f9_MIT6_172F18hw2.pdf) · **Read first:** [HW2: Profiling](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/796439e646c02f44348d50b1836ff7f9_MIT6_172F18hw2.pdf), [Bentley Rules for Optimizing Work](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/1a57adbec9520270d4485b42a2e1a316_MIT6_172F18_lec2.pdf)

> **Why this comes up in practice:** this is what performance engineering is 90% of the time: change one variable at a time, measure after every step, and replace assumptions with numbers. This exercise takes an ordinary stable merge sort and applies inlining, pointers, coarsening, and memory reuse one after another, to see what each trick is really worth.

## Original problem

6.172 HW2 §3 hands you a simple serial merge sort in C, `sort_a.c` (run as `./sort <num_elements> <num_trials>`), and improves it one step at a time while profiling each version with perf and Cachegrind. Each step copies the latest version into a new file with a new suffix, so that every version stays in the test suite for comparison. All versions keep `sort_a`'s signature, keep the internal functions `static`, and keep the overall shape of the merge sort.

- §3.1 Inlining, `sort_i`: mark one or more functions `inline`, confirm in the annotated assembly whether the compiler really inlined them, and also try inlining the recursive sort function.
- §3.2 Pointers vs. arrays, `sort_p`: on top of the inlining, access the arrays through pointers instead of indexing.
- §3.3 Coarsening, `sort_c`: starting from the fastest correct version so far, stop the recursion at small subarrays and sort those with a simple algorithm, such as the insertion sort provided in `isort.c`.
- §3.4 Reducing memory usage, `sort_m`: `merge` fills two scratch arrays, `left` and `right`; keep only one and let the input array serve as the other, which halves the scratch memory.
- §3.5 Reusing temporary memory, `sort_f`: allocate the scratch space once before the sort starts and free it at the end, rather than in every call to `merge`.

The write-ups:

- **Write-up 1:** compare Cachegrind's output for the `DEBUG=1` build and the optimized build, and explain the pros and cons of comparing versions by instruction count instead of time. All later measurements use the optimized build.
- **Write-up 2:** which functions you inlined, and how the inlined and non-inlined versions compare.
- **Write-up 3:** the possible performance costs of inlining a recursive function, and how Cachegrind's data could expose them.
- **Write-up 4:** one reason pointers can be faster, and the difference you measured.
- **Write-up 5:** the algorithm you chose for the base case, how you picked its size, and the difference you measured.
- **Write-up 6:** the performance change in `sort_m`, and whether a compiler could make this optimization for you automatically (explain).
- **Write-up 7:** the performance change in `sort_f`, explained with profiling data.

## Your task

Each stage adds a separate function, and the previous version **stays in the file** (don't rewrite it). That's exactly HW2's workflow, and it's where the baselines for comparison come from. Every function is a **stable** merge sort with this signature (`Compare` defaults to `std::less<>`):

```cpp
template <typename T, typename Compare = std::less<>>
void sort_x(std::span<T> data, Compare cmp = {});
```

You must not use `std::sort`, `std::stable_sort`, or `std::inplace_merge`.

The steps follow the original, but the code is generic C++ and the perf tests compare each stage against fixed baselines inside one executable, instead of you profiling and reporting times.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Baseline

```cpp
template <typename T, typename Compare = std::less<>>
void sort_a(std::span<T> data, Compare cmp = {});
```

`sort_a`: translate HW2's `sort_a.c` into C++. Recurse down to single elements, and merge using two scratch arrays. Empty arrays and single elements must work. The number of comparisons must not exceed `n⌈lg n⌉`.

## Stage 2 — Inlining (§3.1)

```cpp
template <typename T, typename Compare = std::less<>>
void sort_i(std::span<T> data, Compare cmp = {});
```

`sort_i`: pass the comparator as a template parameter so that the compiler can inline it. **The `inline` keyword makes almost no difference at `-O2`**: the hot loop gets inlined anyway, and what really stands in the way is "the compiler can't see the function being called."

The test's baseline (`erased_sort` in `support/sort_baselines.hpp`) has a `qsort`-style interface: every comparison is an indirect call, and every element move is a `memcpy(size)`. Your `sort_i` must beat it by at least **1.1×** in the same executable (compared on 16-byte records; both sides allocate scratch space on every merge, a cost that later stages deal with, so the actual gap is usually around 1.3×. Measure your own numbers).

## Stage 3 — Pointer iteration and aliasing (§3.2)

```cpp
template <typename T, typename Compare = std::less<>>
void sort_p(std::span<T> data, Compare cmp = {});

struct ByteView {
    std::uint8_t* data;
    std::size_t size;
};

[[gnu::noinline]] inline void add_to_all(ByteView& view, std::uint8_t delta);
```

- `sort_p`: rewrite `merge` to walk the arrays with pointers. It behaves the same as `sort_i`, and it only has to be correct. Measure it: the pointer version and the index version come out about the same. The compiler isn't dumb, and the two styles usually produce the same machine code. **Where pointers really pay off is the next item.**
- `add_to_all(ByteView& view, uint8_t delta)`: add `delta` to every byte of `view` (wrapping around). `ByteView` is `{uint8_t* data; size_t size}`, and an empty view (`{nullptr, 0}`) must work too.
  The baseline is written as `for (i < view.size) view.data[i] += delta`: a write through `uint8_t*` may alias `view`'s own fields, so the compiler has to reload them on every iteration and can't vectorize the loop. Copying `data` and `size` into local variables first (hoisting) removes the problem. It must be at least **4×** faster than the baseline.

(Think about why `for (i < v.size()) v[i] = ...` over a `std::vector<uint8_t>` falls into the same trap, while a `std::vector<uint32_t>` doesn't.)

## Stage 4 — Coarsening (§3.3)

```cpp
template <typename T, typename Compare = std::less<>>
void sort_c(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16);
```

`sort_c(data, cmp, cutoff = 16)`: switch to insertion sort (which must be stable) once a subarray's length is `<= cutoff`. `cutoff <= 1` is the same as recursing all the way down. It must beat the fixed `fine_grained_sort` baseline by at least **1.25×**.

## Stage 5 — A single scratch buffer (§3.4)

```cpp
template <typename T, typename Compare = std::less<>>
void sort_m(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16);
```

`sort_m(data, cmp, cutoff = 16)`: `merge` copies only the left half into scratch space and uses the original array as the source of the other half. The test measures the peak scratch usage by replacing the global `operator new`, and it must not exceed `(n/2 + 1)` elements (a small overshoot is allowed, on the order of 16 bytes).

Once it works, think about it: why is this write-back order safe? Could the compiler make this optimization for you?

## Stage 6 — Reusing the scratch buffer (§3.5)

```cpp
template <typename T, typename Compare = std::less<>>
void sort_f(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16);
```

`sort_f(data, cmp, cutoff = 16)`: allocate scratch space only **once** for the whole sort, and share it across all the recursive calls (the `operator new` count must be 1; an input so small that it never merges may allocate nothing). The peak scratch usage limit is the same as in Stage 5. It must beat the fixed baseline by at least **1.4×**.

Look at a Release build with `sample` yourself once, and put the time of each stage in a table.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. If you also mark the recursive function in `sort_i` as `inline`, what does the compiler do? What does inlining a recursive function cost? Can the signal from cachegrind and instruction counts contradict the actual running time?
2. Use the instruction count as a proxy for time: comparing `sort_a` at `-O0` and at `-O2`, by what factor does the instruction count change, and by what factor does the time? What does it mean where the two disagree?
3. When Stage 5 writes back into the original array, why doesn't it overwrite the part of the right half that hasn't been read yet?
