---
title: Measuring the Cost of a Function Call with Regression
unit: 09 · Dynamic Analysis (HW7)
source: 6.172 HW7 §3 Write-up 1–3 (linear-regression analysis)
pdf: materials/6.172/homework/hw07/handout.pdf
kind: code
minutes: 90
---

# 0912 · Measuring the Cost of a Function Call with Regression

**Original problem:** [HW7: Dynamic Analysis Tools](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/1ca096c3f8a039f3b782479c1deca8a4_MIT6_172F18hw7.pdf)

> **Why this comes up in practice:** to answer "how much slower is a virtual call" or "how much does one more RPC layer cost", measuring the total time is useless, because that cost is buried under all the other work. Regression can separate the individual costs, but only if your experiment is well designed; when F and I are highly correlated, the numbers it produces are meaningless.

## Original problem

6.172 HW7 §3 measures the cost of a function call with linear regression. The running time of `fib_serial`, an exponential-time recursive Fibonacci program, is modeled as T = a · I + b · F (1), where I is the number of instructions other than function calls, F is the number of function calls, and a and b are constants. The program takes n and a base-case size of at least 2: it recurses until n drops below the base-case size and then computes the rest with a simple loop, so different inputs give different, ideally uncorrelated, values of I and F. The counts come from a CSI tool, a library of hooks that the compiler calls throughout the instrumented program.

- Write-up 1: implement a CSI tool in `linreg.c` that counts the LLVM IR instructions (using each basic block's non-empty size) and the function calls of a run.
- Write-up 2: describe your measurement strategy: which inputs keep the instruction and call counts from being highly correlated; how you measure time (which timers, how many runs per input, and whether you aggregate with the min, mean, or median); and how you deal with the instrumentation perturbing the running time.
- Write-up 3: following that strategy, collect the times and counts, solve for a and b in (1), and report how correlated the instruction and call counts are in your measurements.

## Your task

The model is the homework's `T = a·I + b·F`: `I` is the amount of work (loop iterations), `F` is the number of calls, and `a` and `b` are the costs of one unit of work and of one call.

`support/workload.cpp` is compiled with `-finstrument-functions` and contains `fib_base(n, base)`, which computes with a loop when `n < base` (`work_units()` accumulates the loop iterations) and recurses otherwise. Write the hooks and functions such as `count_run` in `profiler.cpp` in this folder, and for the regression use `practice::ols::solve` from `_shared/08_ols.hpp` directly.

The workload also has a built-in **virtual clock**: `ticks()` accumulates according to `kUnitCost` and `kCallCost`, independent of the machine. Stages 1–3 use it, so their answers are exact; only Stage 4 measures real time.

Where the homework uses a CSI tool and an external fitting program, you count with `-finstrument-functions` hooks, fit with the shared least-squares code, design the experiment in code, and check the estimates first against the virtual clock and then against real time.

Each stage lists the declarations it adds, copied from `solution.hpp` (the hooks from `profiler.cpp`).

## Stage 1 — Counting (Write-up 1)

```cpp
// profiler.cpp: called by the compiler at every entry and exit of an instrumented function
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site);
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site);

struct RunCounts {
    std::uint64_t instructions = 0;
    std::uint64_t calls = 0;
};

using Input = std::pair<int, int>;  // (n, base)

RunCounts count_run(int n, int base);
```

`count_run(n, base)`: run `workload::fib_base(n, base)` once and return `{instructions, calls}`, where `instructions` is the increase in `work_units()` and `calls` is the number of entries into `fib_base` that the hook counted.

## Stage 2 — Fit (Write-up 3)

```cpp
struct Sample {
    double instructions = 0;
    double calls = 0;
    double time = 0;
};

struct CostModel {
    double per_instruction = 0;
    double per_call = 0;
};

Sample sample_virtual(int n, int base);
CostModel fit_cost_model(std::span<const Sample> samples);
double counts_correlation(std::span<const Sample> samples);
```

- `sample_virtual(n, base)`: return one `Sample{instructions, calls, time}`, where `time` is the increase in the virtual clock.
- `fit_cost_model(samples)`: solve `T = a·I + b·F` with `solve` and return `CostModel{per_instruction, per_call}`. When the regression fails (the regressors are dependent), let the `std::domain_error` propagate.
- `counts_correlation(samples)`: the Pearson correlation coefficient of `I` and `F`.

## Stage 3 — Experiment design (Write-up 2)

```cpp
std::vector<Input> design_inputs();
```

`design_inputs()`: return the list of `(n, base)` pairs you will use for the experiment. The tests check:

- At least 6 pairs, all distinct, with `2 ≤ base` and `1 ≤ n ≤ 30` (the homework's base-case size is at least 2).
- The correlation of `I` and `F` satisfies `|ρ| ≤ 0.9`: the two must vary separately (think about what `n` and `base` each affect).
- It must also hold up under noise: the tests add deterministic noise and compare your design with a poor "fixed base" design, and your estimation error must be much smaller (less than half of the fixed-base design's).

## Stage 4 — Real measurements (Write-up 2–3)

```cpp
double time_fib_ns(int n, int base, int repeats);
CostModel measure_real_cost(std::span<const Input> inputs, int repeats);
```

- `time_fib_ns(n, base, repeats)`: time `workload_plain::fib_base` (the same code, but **without** instrumentation) `repeats` times and return the minimum, in nanoseconds.
- `measure_real_cost(inputs, repeats)`: for each input, count with the instrumented version and time the plain version (the homework's way of avoiding perturbation), then run the regression.

The `PERF` test only does a basic sanity check (both coefficients must be positive), but it prints the numbers. To see the results, run `run --perf-only --verbose` directly: on your machine, how many loop iterations does one function call cost?

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Should `T` be the min, the mean, or the median? What kind of noise is each one suited to? (Homework Write-up 2)
2. Why measure the counts and the time with two separate binaries? If you timed the same instrumented binary, how would the coefficients be biased?
3. When the experiment uses only one `base`, why are `I` and `F` almost perfectly correlated? What does that do to the standard errors of `a` and `b`?
