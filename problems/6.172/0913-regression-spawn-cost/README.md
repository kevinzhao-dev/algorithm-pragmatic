---
title: Measuring the Cost of a Spawn with Regression
unit: 09 · Dynamic Analysis (HW7)
source: 6.172 HW7 §3 Write-up 4–5 (cost of a spawn)
pdf: materials/6.172/homework/hw07/handout.pdf
read: materials/6.172/lectures/lec13.pdf
kind: code
minutes: 60
---

# 0913 · Measuring the Cost of a Spawn with Regression

**Original problem:** [HW7: Dynamic Analysis Tools](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/1ca096c3f8a039f3b782479c1deca8a4_MIT6_172F18hw7.pdf) · **Read first:** [The Cilk Runtime System](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/fe9b746636bea5357ea8269fe6463020_MIT6_172F18_lec13.pdf)

> **Why this comes up in practice:** to choose a parallel grain size, you need to know "how much more one extra task costs than an ordinary function call". Fork-join overhead is typically tens to hundreds of cycles, and only after measuring it once do you know how large the cut-off should be.

## Original problem

6.172 HW7 §3 then measures the cost of a `cilk_spawn` the same way it measured a function call. The serial running time T1 of `fib_cilk`, an exponential-time Cilk Fibonacci program, is modeled as T1 = a · I + b · S (2), where I is the number of instructions (serial function calls included), S is the number of `cilk_spawn`s, and a and b are constants; the one of interest is b. The program takes n and a base-case size: when n is larger than the base-case size, the first recursive call is spawned, and otherwise it is an ordinary call, so changing the base case changes the number of spawns.

- Write-up 4: extend the CSI tool from Write-up 1 (which counts instructions and function calls) to also count the spawns a run executes. The compiler inserts a detach hook, `__csi_detach`, at every spawn, and you may assume the program runs with `CILK_NWORKERS=1`.
- Write-up 5: following the approach of Write-ups 2 and 3 (inputs that decorrelate the counts, careful timing, and a way around instrumentation perturbing the running time), run the regression with one worker. Describe your strategy, report b and how correlated the instruction and spawn counts are, and use the result of Write-up 3 to express the cost of a spawn in function calls.

## Your task

Follow 0912's process, with the number of spawns as the regressor instead. The model is the homework's `T = a·I + b·S`: `I` is the ordinary work (including ordinary function calls), `S` is the number of `fork2` calls, and `b` is the cost of one spawn.

The homework's `cilk_spawn` is replaced here by `practice::fork2` from `harness/forkjoin.hpp`. `workload::fib_spawn(n, base)` in `support/workload.cpp` uses `fork2` only when `n >= base`, and calls `spawn_marker()` once before every fork. This is the equivalent of the homework's `__csi_detach`: the number of entries your hook counts for it is `S`.

Set the number of workers to 1 (the homework's `CILK_NWORKERS=1`): `practice::set_workers(1)`. That way you measure the overhead of the spawn itself, with no steals, and the hooks stay single-threaded.

Write the hooks and the interface in `profiler.cpp`. For the regression, use `_shared/08_ols.hpp`.

As in 0912, Stages 1 and 2 run on the workload's virtual clock, so the fit is exact; Stage 3 measures real time and compares a spawn with an ordinary function call.

Each stage lists the declarations it adds, copied from `solution.hpp` (the hooks from `profiler.cpp`).

## Stage 1 — Counting (Write-up 4)

```cpp
// profiler.cpp: called by the compiler at every entry and exit of an instrumented function
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site);
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site);

struct RunCounts {
    std::uint64_t instructions = 0;
    std::uint64_t spawns = 0;
};

using Input = std::pair<int, int>;  // (n, base)

RunCounts count_run(int n, int base);
```

`count_run(n, base)`: run `workload::fib_spawn(n, base)` once and return `{instructions, spawns}`: `instructions` is the increase in `work_units()`, and `spawns` is the number of entries into `spawn_marker` that the hook counted.

## Stage 2 — Fit

```cpp
struct Sample {
    double instructions = 0;
    double spawns = 0;
    double time = 0;
};

struct CostModel {
    double per_instruction = 0;
    double per_spawn = 0;
};

Sample sample_virtual(int n, int base);
CostModel fit_spawn_cost(std::span<const Sample> samples);
```

- `sample_virtual(n, base)`: one `Sample{instructions, spawns, time}`, where `time` is the increase in the virtual clock.
- `fit_spawn_cost(samples)`: solve `T = a·I + b·S` and return `CostModel{per_instruction, per_spawn}`. When the regressors are dependent, let the `std::domain_error` propagate.

## Stage 3 — Real measurements (Write-up 5)

```cpp
double time_fib_spawn_ns(int n, int base, int repeats);
std::vector<Input> design_inputs();
double measure_spawn_cost(std::span<const Input> inputs, int repeats);
double call_cost_ns();
```

Use `workload_plain`, which has **no** instrumentation:

- `time_fib_spawn_ns(n, base, repeats)`: time `workload_plain::fib_spawn(n, base)` and return the minimum over `repeats` runs (in nanoseconds).
- `design_inputs()`: return your `(n, base)` experiment design, with at least 6 pairs and `n ≤ 24`. Think about how to make `S` vary separately from `I`. The tests also require the pairs to be distinct, with `1 ≤ n` and `2 ≤ base`, and the correlation of `I` and `S` to satisfy `|ρ| ≤ 0.95`.
- `measure_spawn_cost(inputs, repeats)`: take the counts from the instrumented version and the times from the plain version, and regress out the nanoseconds per spawn.
- `call_cost_ns()`: measure the nanoseconds of one `workload_plain::empty_call()` (an opaque function, so the call is not inlined).

The `PERF` test prints the results (`run --perf-only --verbose`) and checks that one spawn costs at least 2 times as much as one function call.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What does Stage 3's ratio `b / call_cost` tell you? How do you use it to work out where the parallel cut-off should be? (Homework Write-up 5)
2. Which costs do you measure with workers = 1? With more workers, what extra costs do steals add, and why are they harder for the regression to separate?
3. Cilk follows the work-first principle: put the costs on the steal path, not on the spawn path. Where does `forkjoin.hpp` violate this principle? (Lecture 13)
