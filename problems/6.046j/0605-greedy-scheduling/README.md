---
title: Greedy Scheduling (SPT, SRPT, interval partitioning)
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J Recitation 6
pdf: materials/6.046j/recitations/rec06.pdf
read: materials/6.046j/recitations/rec06.pdf
kind: code
minutes: 75
---

# 0605 · Greedy Scheduling

**Based on:** [Greedy Algorithms: More Examples](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/151160c7c18d3f87159b7482823ff61c_MIT6_046JS15_Recitation6.pdf) · **Read first:** [Greedy Algorithms: More Examples](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/151160c7c18d3f87159b7482823ff61c_MIT6_046JS15_Recitation6.pdf)

> **Why this comes up in practice:** batch job scheduling, SJF/SRPT in CPU scheduling, and the minimum number of meeting rooms or GPU machines to provision. The common interview question "meeting rooms II" is the last stage.

## Your task

All times are integers.

You implement three greedy schedules from Recitation 6: SPT on one machine, its online preemptive counterpart SRPT, and interval partitioning onto the fewest machines.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Shortest Processing Time First

```cpp
std::vector<int> spt_order(std::span<const long long> durations);
long long total_completion_time(std::span<const long long> durations,
                                std::span<const int> order);
```

`std::vector<int> spt_order(std::span<const long long> durations)`: return an execution order (a list of job indices) that minimizes the total completion time on a single machine. O(n log n).

`long long total_completion_time(std::span<const long long> durations, std::span<const int> order)`: compute `Σ C_i` for that order, starting at time 0 with no idle time.

## Stage 2 — The online version: SRPT

```cpp
struct Job {
    long long release = 0;
    long long duration = 0;
};
std::vector<long long> srpt_completion(std::span<const Job> jobs);
```

`Job{release, duration}`: a job becomes known only at time `release`. Preemption is allowed: at any moment the single machine runs one job, and it may switch jobs at any integer time at no cost.

`std::vector<long long> srpt_completion(std::span<const Job> jobs)`: return each job's completion time, minimizing `Σ (completion time)`. O(n log n).

Edge cases: simultaneous arrivals, a job arriving while another one is halfway through, and equal durations.

## Stage 3 — Interval partitioning (real-world extension: the fewest machines)

```cpp
struct Interval {
    long long start = 0;
    long long finish = 0;
};
```

`Interval{start, finish}` is half-open, `[start, finish)`, and `start < finish` is guaranteed. Assign every interval to a machine so that the intervals on the same machine don't overlap.

```cpp
struct Allocation {
    int machines = 0;              // minimum possible
    std::vector<int> assignment;   // machine id per interval, 0..machines-1
};
Allocation allocate(std::span<const Interval> intervals);
```

O(n log n).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. How does the exchange argument for SPT go? If jobs have weights (weighted completion time), what should the sort key become?
2. Why does "sort by start time + a min-heap" reach the maximum-overlap lower bound? What happens if you sort by finish time instead?
3. Why is greedy correct for fractional make-change, where coins can be melted down, but not for make-change with whole coins? Give a counterexample set of denominations.
