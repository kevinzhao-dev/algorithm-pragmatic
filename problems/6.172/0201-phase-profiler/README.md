---
title: Phase Profiler (build your own perf report)
unit: 02 · Profiling (HW2)
source: 6.172 HW2 §2 Recitation (perf/Cachegrind concept)
pdf: materials/6.172/homework/hw02/handout.pdf
read: materials/6.172/homework/hw02/handout.pdf, materials/6.172/lectures/lec10.pdf
kind: code
minutes: 90
---

# 0201 · Phase Profiler (build your own perf report)

**Based on:** [HW2: Profiling](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/796439e646c02f44348d50b1836ff7f9_MIT6_172F18hw2.pdf) · **Read first:** [HW2: Profiling](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/796439e646c02f44348d50b1836ff7f9_MIT6_172F18hw2.pdf), [Measurement and Timing](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/4d7f4bb31bf1ed90c669a11867d36d36_MIT6_172F18_lec10.pdf)

> **Why this comes up in practice:** when an inference service slows down, the question to answer is "which part is the time going to?" The inclusive/self columns of `perf report` aren't magic: they're exactly what this exercise has you write. macOS has no perf, but it has `sample` and Instruments, and what they output is the kind of call stack you'll process below.

## Your task

Build a profiler that times a program by phase (a name for a section of code). Time comes from the `clock` passed in at construction (it returns monotonically increasing ticks, and the tests use a fake clock, so everything is deterministic).

HW2's recitation has you read the output of perf and Cachegrind on small C programs. Here you compute a profiler's numbers yourself: first from instrumented enter/exit events, then from sampled call stacks.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — enter / exit, self, and inclusive

```cpp
class Profiler {
public:
    using Clock = std::function<std::uint64_t()>;

    struct Entry {
        std::string name;
        std::uint64_t calls = 0;
        std::uint64_t inclusive = 0;
        std::uint64_t self = 0;
    };

    explicit Profiler(Clock clock);

    void enter(std::string_view name);
    void exit();
    std::size_t depth() const;
    std::vector<Entry> report() const;
};
```

| Interface | Behavior |
| --- | --- |
| `Profiler(clock)` | `clock` is a `std::function<std::uint64_t()>` |
| `enter(name)` | Start a phase. You must copy `name` yourself, since the string passed in may be destroyed right away |
| `exit()` | End the innermost phase; throw `std::logic_error` when no phase is open (leaving the state unchanged) |
| `depth()` | The current nesting depth |
| `report()` | Return an `Entry{name, calls, inclusive, self}` for each phase, sorted by `self` from largest to smallest, with ties broken by `name` in lexicographic order |

- `self`: the time the phase spends on its own, minus the phases nested directly inside it.
- `inclusive`: the phase's total time from enter to exit, including everything inside it.
- **Recursion:** when the same name appears twice on the call chain, `inclusive` counts only the outermost activation (otherwise the time would be counted twice). `self` and `calls` accumulate as usual.

Example: `a` is entered and does 1 tick of work, calls `b` (which does 2 ticks), and does 1 more tick after it returns. Then `a` has `self = 2` and `inclusive = 4`, and `b` has `self = inclusive = 2`.

`report()` counts only the calls that have already ended.

`enter` and `exit` must also be cheap: a `PERF` test makes two million nested `enter`/`exit` pairs within a fixed time budget.

## Stage 2 — RAII

```cpp
class ScopedPhase {
public:
    ScopedPhase(Profiler& profiler, std::string_view name);
    ~ScopedPhase();

    ScopedPhase(const ScopedPhase&) = delete;
    ScopedPhase& operator=(const ScopedPhase&) = delete;
};
```

`ScopedPhase(profiler, name)`: `enter` on construction, `exit` on destruction. The destructor must not throw. When an exception passes through a scope, the nesting must stay correct.

## Stage 3 — Folded stacks (the flamegraph input format)

```cpp
// in class Profiler
std::vector<std::string> folded() const;
```

`std::vector<std::string> folded() const`: one line per distinct call chain, in the format `"a;b;c 120"` (the number is the `self` tick count), with the names joined by semicolons from the outermost to the innermost. Paths whose `self` is 0 are not output. Sort the result lexicographically by whole line.

## Stage 4 — Real-world extension: the same report from stack samples

`sample`/`perf` take samples at fixed intervals, and each sample is a complete call stack (outermost frame first). Implement:

```cpp
std::vector<Profiler::Entry> report_from_samples(std::span<const std::vector<std::string>> stacks);
```

Count each sample as 1 tick: `self` goes to the innermost frame, and `inclusive` goes to every name that appears in that sample (a name that appears more than once in the same sample counts only once). Leave `calls` at 0. Ignore empty samples. Sort the same way as `report()`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Where does the error come from in a sampling profiler, and in an instrumenting profiler? When would a buggy program not reproduce its bug under a profiler?
2. When you look at a Release executable with `sample $PID 2`, why have some functions disappeared completely? (Inlining.)
3. What are the advantages and the disadvantages of using cachegrind's instruction count as a proxy for time? (HW2 Write-up 1)
