---
title: Work/Span Meter (build your own Cilkscale)
unit: 05 · Fork-Join and Reducers (HW4)
source: 6.172 HW4 §2 Recitation (Checkoff 1–3, 5), Recitation R4, Lecture 8
pdf: materials/6.172/homework/hw04/handout.pdf
answer: materials/6.172/recitations/r4/solutions.pdf
read: materials/6.172/homework/hw04/handout.pdf, materials/6.172/lectures/lec06.pdf, materials/6.172/lectures/lec08.pdf, materials/6.172/recitations/r4/problems.pdf
kind: code
sanitizer: tsan
minutes: 120
---

# 0501 · Work/Span Meter

**Based on:** [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf) · **Read first:** [HW4: Cilk, Reducer Hyperobjects](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b2ff5659ae776c6fa6834967f73a2ab4_MIT6_172F18hw4.pdf), [Multicore Programming](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cbef71a2b92bc700fbad43e69f951513_MIT6_172F18_lec6.pdf), [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf), [R4: Analysis of Multithreaded Programs Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/4d86bef65ea41cddc2655ef8550de0e2_MIT6_172F18_rec4.pdf)

> **Why this comes up in practice:** you start 16 threads and get only a 3× speedup. The problem usually isn't the scheduler; the program's parallelism is just 3 to begin with. What Cilkscale does is measure work (T₁) and span (T∞). This exercise builds a deterministic version of it yourself, and every parallel exercise after this one uses it to check complexity.

## Original problem

The recitation half of 6.172 HW4 (§2) introduces Cilk on 8-core machines. The checkoff items this exercise draws on:

- **Checkoff Item 1:** parallelize the recursive `fib` program with `cilk_spawn` and `cilk_sync`, and report its running time for `fib(45)` with 1, 4, and 8 Cilk workers. The parallel version may well be no faster than the serial one.
- **Checkoff Item 2:** coarsen the recursion until the parallel version really is faster, describe how you coarsened it, and report the times with 1, 4, and 8 workers.
- **Checkoff Item 3:** parallelize an in-place matrix transpose program with `cilk_for`, and report its time for input size 10000 with 1, 4, and 8 workers.
- **Checkoff Item 5:** report the parallelism that the Cilkscale analyzer computes for a race-free parallel quicksort on a few input sizes. Cilkscale derives work and span from measured cycle counts.

Recitation 4 has two problems:

- (1) Given a computation dag in which every strand takes unit time (the dag is a figure that isn't reproduced here), find its work, span, and parallelism.
- (2) Five students write a recursive Fibonacci whose base case returns 1 for n = 0 and n = 1; they differ only in the recursive case: (a) both calls are ordinary calls; (b) both calls are spawned, followed by a sync; (c) `fib(n-1)` is called normally and `fib(n-2)` is then spawned, followed by a sync; (d) `fib(n-2)` is spawned first and `fib(n-1)` is called normally, followed by a sync; (e) `fib(n-1)` is spawned and `fib(n-2)` is called normally, followed by a sync. If a spawn costs about 10 times an ordinary call, rank the five by the performance you'd expect for large n.

Lecture 8 analyzes the work and span of multithreaded algorithms: divide-and-conquer recurrences, and parallel loops that split their range recursively, where a grain size trades a longer span for less spawn overhead.

The original programs are C with Cilk, measured with Cilkscale on Linux.

## Your task

> ⚠️ **Spoiler warning:** `problems/6.172/_shared/01_exec.hpp` has a complete `Meter`, which later exercises use as a test tool. Don't open it before you finish this exercise.

There's no Cilk here; the basic parallel operation is `practice::fork2(f, g)` (see `harness/forkjoin.hpp`). This exercise introduces the idea of an **executor**: algorithms are written as templates, so the same code can measure its cost (`Meter`) or really run in parallel (`ForkJoin`). The tests run under ThreadSanitizer, and a data race fails them outright.

Instead of timing Cilk programs and reading Cilkscale's output, you build a deterministic work/span meter, use it to account for `fib` with and without coarsening and for a parallel sum and transpose, and finally turn work and span into speedup bounds.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Cost algebra

```cpp
struct Cost {
    long long work = 0;
    long long span = 0;

    double parallelism() const;
};

Cost series(Cost a, Cost b);
Cost parallel(Cost a, Cost b);
```

`struct Cost { work, span; }`, where `parallelism()` is `work / span` (return 1 when `span == 0`).

`series(a, b)`: do a, then b; both work and span add up. `parallel(a, b)`: do both at the same time; work adds up, and span takes the maximum.

## Stage 2 — Meter

```cpp
class Meter {
public:
    void charge(long long units = 1);

    template <typename F, typename G>
    void fork2(F&& f, G&& g);

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1);

    Cost cost() const;
    long long spawns() const;
};
```

Write an executor that runs everything in order (serially) but computes the cost exactly:

| Interface | Semantics |
| --- | --- |
| `charge(n = 1)` | The current strand did `n` units of work, which count toward both work and span |
| `fork2(f, g)` | Call `f()` and then `g()` (serial execution), but account for them as parallel: `fork2` itself charges 1 unit (on the strand before the fork), and the costs of the two sides are combined with `parallel` and added to the current strand |
| `parallel_for(begin, end, body, grain = 1)` | Split the range recursively in halves (with `fork2`) until its length is `<= max(grain,1)`, then call `body(i)` in order at each leaf; charge nothing extra, since `body` does its own accounting |
| `cost()` / `spawns()` | The accumulated cost; the number of times `fork2` was called |

## Stage 3 — Fib and coarsening

```cpp
template <typename Exec>
long long fib(Exec& exec, int n, int cutoff);
```

`fib(exec, n, cutoff)`: return `n` when `n <= 1` (fib(0)=0). Follow the rules **exactly** (the tests check the accounting):

- Every function call charges 1 (base cases included).
- When `n > cutoff`, compute `fib(n-1)` and `fib(n-2)` with `exec.fork2`; when `n <= cutoff` (and `n >= 2`), just recurse normally (no fork). That's coarsening.

Measure `fib(24)` with `Meter`: raising `cutoff` from 1 to 12 cuts the number of spawns by several orders of magnitude, while the span gets longer (the serial chains of the base cases all count toward the span). As long as the parallelism stays far above the number of cores, the trade pays off. **Think about why the real cost of a spawn makes you willing to pay that span.**

## Stage 4 — Real parallelism

```cpp
class ForkJoinExec {
public:
    void charge(long long units = 1) const;

    template <typename F, typename G>
    void fork2(F&& f, G&& g) const;

    template <typename Body>
    void parallel_for(std::size_t begin, std::size_t end, Body&& body, std::size_t grain = 1) const;
};

template <typename Exec>
long long parallel_sum(Exec& exec, std::span<const int> data, std::size_t grain);

template <typename Exec>
void transpose_in_place(Exec& exec, std::span<int> grid, std::size_t n, std::size_t grain);
```

`ForkJoinExec`: the same interface as Meter, but `charge` does nothing, and `fork2`/`parallel_for` forward to `practice::fork2`/`practice::parallel_for`.

- `parallel_sum(exec, data, grain)`: add everything up with `parallel_for`, with no data races (think about how to avoid a shared accumulator). Return the total. `charge(1)` per element and nothing else: on 4096 elements with `grain = 1`, the tests expect a work of exactly `2n - 1` and a span of at most 14.
- `transpose_in_place(exec, grid, n, grain)`: transpose an `n×n` grid in place (row-major, `grid.size() == n*n`), using `parallel_for` for both the outer and the inner loop. `charge(1)` for each pair of elements swapped. (This corresponds to HW4 Checkoff 3, with parallel loops instead of recursion.) With `n = 128` and `grain = 1`, the tests require a span of at most 30.

## Stage 5 — Predicting speedup

```cpp
struct SpeedupBounds {
    double lower = 0;
    double upper = 0;
};

SpeedupBounds speedup_bounds(Cost cost, int processors);
```

`speedup_bounds(cost, p)`: return `{lower, upper}`, which are the lower bound that theory guarantees for a greedy scheduler on `p` processors, and the upper bound that the work/span laws give (`min(p, parallelism)`). For the lower bound, use `T_p ≤ T1/p + T∞`: `T1 / (T1/p + T∞)`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. How do R4's five fib versions (`spawn` in different places, both calls spawned, neither spawned) rank in performance? Why is a `spawn` followed immediately by a `sync` useless? (The answer is in the R4 solutions.)
2. Why does Cilkscale measure work/span through instrumentation instead of looking at how fast the program actually runs?
3. If you raise the spawn cost of `fork2` from 1 to 100, how does the trade-off between the two `cutoff` values above change?

---

**Official solution:** [R4 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b5784797a007ded8f3276eed5e8c1d0d_MIT6_172F18_rec4sol.pdf). Read it after you finish.
