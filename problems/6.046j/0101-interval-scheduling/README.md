---
title: Interval Scheduling (greedy and weighted DP)
unit: 01 · Scheduling and Divide & Conquer (PS1)
source: 6.046J Lecture 1, Recitation 1
pdf: materials/6.046j/lectures/lec01.pdf
read: materials/6.046j/lectures/lec01.pdf, materials/6.046j/recitations/rec01.pdf
kind: code
minutes: 75
---

# 0101 · Interval Scheduling

**Based on:** [Overview, Interval Scheduling](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ff2a7015ff913fc01f0381d8f5126a9a_MIT6_046JS15_lec01.pdf) · **Read first:** [Overview, Interval Scheduling](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ff2a7015ff913fc01f0381d8f5126a9a_MIT6_046JS15_lec01.pdf), [Divide & Conquer: Smarter Interval Scheduling, Master Theorem, Strassen’s Algorithm](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7c15f31b5a1aa1991313d1b4544cc137_MIT6_046JS15_Recitation1.pdf)

> **Why this comes up in practice:** booking systems for GPU clusters, meeting rooms, and ad slots all have to answer either "how many orders can one resource take at most?" or "which orders bring in the most revenue?" The two questions differ by a single weight, yet that weight moves the usable algorithm from greedy to DP.

## Your task

Each request occupies the half-open interval `[start, finish)`, and `value` is the revenue from accepting it. Two requests are compatible when `a.finish <= b.start` or `b.finish <= a.start`, so two requests that only touch at an endpoint can both be accepted.

Valid input has `start < finish` and `value >= 0`; otherwise throw `std::invalid_argument`. Every list of request indices you return is in increasing order of start time. When there are several optimal answers, any one of them is accepted.

You implement the greedy from Lecture 1 for the unweighted problem and a DP for the weighted one, then build a planner on top of the DP that answers what-if questions.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — The most requests

```cpp
struct Request {
    std::int64_t start;
    std::int64_t finish;
    std::int64_t value;
};

std::vector<int> max_compatible(std::span<const Request> requests);
```

`max_compatible(requests)`: return a compatible subset with as many requests as possible, ignoring `value`. O(n log n).

Lecture 1 lists four greedy rules, and only one of them is correct. Work out a counterexample for each of the other three before you start; the tests check all of them.

## Stage 2 — The highest revenue

```cpp
struct Schedule {
    std::int64_t total = 0;
    std::vector<int> chosen;
};

Schedule max_value_schedule(std::span<const Request> requests);
```

`max_value_schedule(requests)`: return the compatible subset with the highest total revenue, where `total` is the sum of the values and `chosen` holds the selected requests. It must run in O(n log n), so finding the "previous compatible position" for each request must use binary search, not a linear scan.

The total is stored as an `int64`, and the input is guaranteed not to overflow it.

## Stage 3 — Real-world extension: the opportunity cost of an order

```cpp
class WhatIfPlanner {
public:
    explicit WhatIfPlanner(std::span<const Request> requests);

    std::int64_t best_total() const;
    std::int64_t best_with(std::size_t index) const;
    std::int64_t regret(std::size_t index) const;
};
```

Sales asks: "If we have to accept this VIP order, what does the total revenue become?" Build a `WhatIfPlanner`:

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `WhatIfPlanner(requests)` | Preprocess; after construction it must not depend on the `span` passed in | O(n log n) |
| `best_total()` | The highest revenue when no order is forced | O(1) |
| `best_with(i)` | The highest revenue when request `i` is forced in; throw `std::out_of_range` if `i` is out of range | O(log n) |
| `regret(i)` | `best_total() - best_with(i)` | O(log n) |

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Use an exchange argument to explain why the earliest-finish-time greedy is optimal, and give a counterexample for each of the other three rules.
2. Why does greedy fail for the weighted version? Spell out the DP's subproblem definition and transition, and explain why the intervals you recover by backtracking come out already sorted by start time.
3. If the single resource becomes `m` machines of different types, and each request can run only on some of them, what complexity class does the problem fall into? How would you handle it in practice?
