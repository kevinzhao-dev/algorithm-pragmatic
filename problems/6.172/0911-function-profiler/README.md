---
title: Writing a Function Profiler with Compiler Instrumentation
unit: 09 · Dynamic Analysis (HW7)
source: 6.172 HW7 §2 Checkoff 1–5 (function coverage and profiling)
pdf: materials/6.172/homework/hw07/handout.pdf
kind: code
minutes: 90
---

# 0911 · Writing a Function Profiler with Compiler Instrumentation

**Original problem:** [HW7: Dynamic Analysis Tools](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/1ca096c3f8a039f3b782479c1deca8a4_MIT6_172F18hw7.pdf)

> **Why this comes up in practice:** perf gives you samples, but some questions need an exact answer, such as "how many times is this function really called", or need something done on every call (coverage, allocation attribution, tracing). That's when you insert hooks at compile time; LLVM XRay, gcov, and all the sanitizers take this route.

## Original problem

6.172 HW7 §2 is a recitation exercise in CSI, the compiler-instrumentation framework of the Tapir/LLVM compiler. A CSI tool is a library that defines hooks: the compiler inserts calls to them throughout the program under test (on function entry and exit, basic-block entry, memory accesses, and so on), and any hook the tool leaves undefined is optimized away. Every instrumented object gets a CSI ID, a dense integer within its category, so a tool can keep its data in plain arrays, and forensic tables map IDs to static information such as function names and source locations. You complete a function-analysis tool in `func-analysis.c`:

- Checkoff 1: add global variables that record how many times each function runs, and initialize them in the `__csi_init` and `__csi_unit_init` hooks.
- Checkoff 2: add a `report` function that uses the forensic tables to print every function's name and execution count, and register it in `__csi_init` to run when the program exits.
- Checkoff 3: decide which IR-object hooks let you observe each function execution, and implement them to count executions.
- Checkoff 4: build the tool and use it to instrument the provided `fib_serial` program, then run it.
- Checkoff 5: instrument `bzip2` with the tool, compress a test file, and show a TA that the counts are right.
- Final checkoff (time permitting): turn the coverage tool into a profiler that records how much of the running time is spent in each function, try it on fib and on bzip2, and make sure it doesn't over-count the time spent in recursive functions.

## Your task

The CSI toolchain the homework uses doesn't exist for Apple clang. Instead, this exercise uses clang's built-in `-finstrument-functions`: the compiler calls the two hooks below on entry to and exit from every instrumented function, and you provide their implementations:

```cpp
extern "C" void __cyg_profile_func_enter(void* this_fn, void* call_site);
extern "C" void __cyg_profile_func_exit(void* this_fn, void* call_site);
```

`support/workload.cpp` is the program under test, and it is the only file compiled with `-finstrument-functions` (see `practice.cmake`). Write the hooks and the `profiler::` interface in `profiler.cpp` in this folder. `this_fn` is the function's address, the same as `&workload::fib` in the tests, so you can use it directly as a key.

The file that holds the hooks is **not** instrumented, so using `std::unordered_map` inside a hook won't call the hooks recursively; go ahead and use it.

The tests are single-threaded, so you don't need to handle races.

Instead of a CSI tool in C that prints a report at exit, you build a C++ profiler with a small API that the tests query directly: per-function call counts, a demangled report, inclusive and exclusive time, and a growth estimate from call counts.

Each stage lists the declarations it adds, copied from `solution.hpp`.

## Stage 1 — Function coverage (Checkoff 1, 3, 4)

```cpp
namespace profiler {

void start();
void stop();
std::uint64_t calls(const void* function);

}  // namespace profiler
```

`start()` clears the data and starts recording, and `stop()` stops recording (the data stays available for queries). `calls(fn)` returns the number of times the function was entered during this recording.

## Stage 2 — Report (Checkoff 2)

```cpp
namespace profiler {

struct ReportLine {
    std::string name;
    std::uint64_t calls = 0;
    friend bool operator==(const ReportLine&, const ReportLine&) = default;
};

std::vector<ReportLine> report();

}  // namespace profiler
```

`report()` returns the functions that were **actually called** during this recording, sorted by name, with demangled names (for example `workload::fib(int)`). Hint: `dladdr` plus `abi::__cxa_demangle`.

## Stage 3 — Profiling (Checkoff, time permitting)

```cpp
namespace profiler {

struct FunctionStats {
    std::uint64_t calls = 0;
    std::uint64_t inclusive_ticks = 0;
    std::uint64_t exclusive_ticks = 0;
};

using Clock = std::uint64_t (*)();

void set_clock(Clock clock);
FunctionStats stats(const void* function);

}  // namespace profiler
```

`stats(fn)` returns `FunctionStats`:

- `calls`: the number of entries.
- `exclusive_ticks`: the time spent in the function itself (excluding its callees).
- `inclusive_ticks`: the total time from entry to exit of the function's outermost activations. Recursive functions must not be double-counted: when `f` is called inside `f`, the inner stretch of time must not be counted twice (the pitfall the homework specifically warns about).

Inject the time source with `set_clock(clock)` (`Clock` is a pointer to a function returning `std::uint64_t`; passing `nullptr` restores the default, `std::chrono::steady_clock` in nanoseconds). The tests use a fake virtual clock, so the results are exact integers.

## Stage 4 — Estimating complexity from call counts

```cpp
namespace profiler {

practice::ols::Growth estimate_growth(const void* function, void (*run)(int),
                                      std::span<const int> sizes);

}  // namespace profiler
```

`estimate_growth(function, run, sizes)`: for each `n ∈ sizes`, record one run of `run(n)` and collect the call count of `function`, then return the complexity class that `practice::ols::classify_growth` from `_shared/08_ols.hpp` gives.

The tests apply it to linear, quadratic, merge sort, and fib workloads.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. CSI can see basic blocks and memory accesses, while `-finstrument-functions` sees only function entry and exit. What kinds of tools is each suited for?
2. Instrumentation changes the behavior of the program under test (timing, inlining). How do you measure the cost of the hooks? (Homework Write-up 2)
3. When a function exits because of an exception, `__cyg_profile_func_exit` may not be called. How should your shadow stack handle that?
