---
title: Performance Measurement Auditor (the work/span laws as an LP)
unit: 06 · Work/Span Theory (HW5)
source: 6.172 HW5 Write-up 2, 3, 4 (CLRS 27.1-5)
pdf: materials/6.172/homework/hw05/handout.pdf
read: materials/6.172/lectures/lec08.pdf
kind: code
minutes: 100
---

# 0612 · Performance Measurement Auditor

**Original problem:** [HW5: Theory of Performance Engineering](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cd57dbdecbce3bd4da39d620c1e61d14_MIT6_172F18hw5.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf)

> **Why this comes up in practice:** a colleague posts benchmark results for 3 core counts and says "the parallelism is about 20." From those numbers alone, can you show that "these three measurements can't all be true," or work out the possible range of the parallelism? That's real-world reasoning you can use in capacity planning.

## Original problem

6.172 HW5 is a written, take-home problem set on Chapter 27 of CLRS; it asks for arguments, not code. These three write-ups apply the work/span bounds to measured running times. Each measurement comes from a deterministic program run by a greedy scheduler on an ideal parallel computer, so with work T₁ and span T∞, the time T_P on P processors must satisfy the Work Law (T_P ≥ T₁/P), the Span Law (T_P ≥ T∞), and the greedy bound from CLRS Exercise 27.1-3 (T_P ≤ (T₁ − T∞)/P + T∞).

- **Write-up 2:** Ben measures T₄ = 100 seconds and T₆₄ = 10 seconds. What are the lowest and the highest possible values of the program's parallelism?
- **Write-up 3:** Ben measures T₄ = 80, T₁₀ = 42, and T₆₄ = 9 (in seconds). Argue that at least one of these measurements must be wrong.
- **Write-up 4** (CLRS Exercise 27.1-5): a professor claims that her program took T₄ = 80 seconds, T₁₀ = 42 seconds, and T₆₄ = 10 seconds. Argue that these claims can't all be true.

## Your task

Turn HW5's hand calculations into a program. Assume that the program is deterministic and is run by a greedy scheduler on an ideal parallel computer. The unknowns are `T1` (work) and `T∞` (span). Each measurement `Measurement{processors p, seconds T}` gives three constraints:

- Work Law: `T ≥ T1 / p`
- Span Law: `T ≥ T∞`
- Greedy upper bound (CLRS 27.1-3): `T ≤ (T1 − T∞) / p + T∞`

In addition, `T1 ≥ T∞ ≥ 0`. Together these constraints enclose a (possibly empty) convex polygon in the `(T1, T∞)` plane.

Hint: the intersections of the constraint lines are the vertices, so for a two-dimensional LP it's enough to "enumerate every intersection of two lines and keep the feasible ones."

Instead of reasoning about three fixed sets of numbers on paper, you write an auditor that works for any set of measurements: it checks them for consistency, bounds the work, span, and parallelism, names the smallest conflicting subset, and predicts the running time on a new number of cores.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Consistency and ranges

```cpp
struct Measurement {
    int processors = 1;
    double seconds = 0;
};

struct Audit {
    bool consistent = false;
    double min_work = 0;
    double max_work = 0;
    double min_span = 0;
    double max_span = 0;
    double min_parallelism = 0;
    double max_parallelism = 0;
};

Audit audit_measurements(std::span<const Measurement> measurements);
```

`audit_measurements(measurements)` returns `Audit{consistent, min_work, max_work, min_span, max_span, min_parallelism, max_parallelism}`.

- `measurements` must not be empty, and each measurement needs `processors >= 1` and `seconds > 0`; otherwise throw `std::invalid_argument`.
- When `consistent == false`, the other fields don't matter.
- When `T∞` can be arbitrarily small, `min_span` is 0 and `max_parallelism` is `+∞` (`std::numeric_limits<double>::infinity()`). Values are compared with a relative error of at most `1e-6`.

Mapping to HW5: Write-up 2 asks for the lower and upper bounds on the parallelism, and Write-ups 3/4 find inconsistent measurements. Check your code by working out HW5's three sets of numbers by hand (see Original problem): what is the range of the parallelism for `T4 = 100, T64 = 10`?

## Stage 2 — Pointing out which measurements conflict

```cpp
std::vector<std::size_t> minimal_conflict(std::span<const Measurement> measurements);
```

`minimal_conflict(measurements)`: return the **smallest** mutually contradictory subset (indices in increasing order); if several have the same size, return the lexicographically smallest; when everything is consistent, return an empty vector. (Helly's theorem guarantees that a contradictory subset needs at most 3 measurements; a single measurement is always consistent.)

## Stage 3 — Extrapolating to a new core count

```cpp
struct RuntimeBounds {
    double low = 0;
    double high = 0;
};

RuntimeBounds predict_runtime(std::span<const Measurement> measurements, int processors);
```

`predict_runtime(measurements, p)` returns `RuntimeBounds{low, high}`: over all possible `(T1, T∞)`, the smallest possible value of `T_p` (the minimum of `max(T1/p, T∞)`) and the largest possible value (the maximum of `(T1−T∞)/p + T∞`). Throw `std::domain_error` when the measurements themselves are inconsistent.

Note that the minimum of `max(T1/p, T∞)` isn't necessarily at a vertex; it can also lie where the line `T1 = p·T∞` crosses an edge of the polygon.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why are the Work Law and the Span Law "necessary" conditions, while the greedy upper bound holds only under the "greedy scheduler" assumption? If the scheduler isn't greedy, does this audit still hold?
2. Why can `max_parallelism` be infinite? Under what condition would one more measurement tighten it?
3. On a real machine, how do caches and NUMA make this model inaccurate? How would you flag that in a report?
