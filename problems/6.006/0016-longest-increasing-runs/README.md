---
title: Counting Longest Increasing Runs (streaming version)
unit: 00 · Warm-up (PS0)
source: 6.006 PS0 Problem 0-6
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 35
---

# 0016 · Counting Longest Increasing Runs

**Original problem:** [Problem Set 0 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/60f0a029ab154553cad51b54cc25605b_MIT6_006S20_ps0-questions.pdf)

> **Why this comes up in practice:** a monitoring system needs to know the longest stretch of "latency rising steadily" and how many times it has happened before it decides whether to alert. The data is a time series that keeps streaming in, so you can't keep the whole history around and rescan it.

## Original problem

6.006 PS0 Problem 0-6 is the one coding problem in PS0. It defines an "increasing subarray" of an integer array as a contiguous stretch of the array in which every value is larger than the one before it.

- Write `count_long_subarrays(A)`. It receives a tuple A = (a0, a1, …, an−1) of n > 0 positive integers and returns how many increasing subarrays of A are as long as the longest one.
- Example: for A = (1, 3, 4, 2, 7, 5, 6, 9, 8), the longest increasing subarrays have length 3, and there are two of them, (1, 3, 4) and (5, 6, 9), so the answer is 2.

Implement it in the Python template, which comes with a few test cases.

## Your task

The original problem: given an integer array, find the greatest length among its **strictly increasing contiguous subarrays**, and return how many subarrays have that greatest length (the details are in the Original problem section above). An empty array returns 0.

This exercise solves it for an array first, then for a stream that arrives one value at a time, and finishes with an alarm for rising streaks, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — The array version

```cpp
template <typename T>
std::size_t count_longest_runs(std::span<const T> values);
```

`count_longest_runs<T>(values)`: `values` is a `std::span<const T>`. Use a single pass, **O(n)**. Return 0 for an empty array.

When two adjacent elements are equal, the increasing run breaks (the increase must be strict).

## Stage 2 — The streaming version

```cpp
template <typename T, typename Less = std::less<T>>
class RunTracker {
public:
    void push(const T& value);

    std::size_t longest() const;
    std::size_t count() const;
    std::size_t current() const;
};
```

The data arrives one value at a time. `RunTracker<T, Less>` (the comparator defaults to `std::less<T>`):

- `push(value)`: **O(1)**, and it must not use any extra heap memory (you can't store the data).
- `longest()`: the length of the longest strictly increasing run so far, or 0 before any data has arrived.
- `count()`: how many runs have a length equal to `longest()`, or 0 before any data has arrived.
- `current()`: the length of the increasing run that ends at the latest value, or 0 before any data has arrived.

`Less` defines what "increasing" means: with `std::greater<>`, the tracker follows strictly decreasing runs instead.

## Stage 3 — Real-world extension: alerting on a rising streak

```cpp
template <typename T, typename Less = std::less<T>>
class StreakAlarm {
public:
    explicit StreakAlarm(std::size_t threshold);

    bool observe(const T& value);
};
```

`StreakAlarm<T, Less>`:

- `explicit StreakAlarm(std::size_t threshold)`: `threshold ≥ 1`; otherwise throw `std::invalid_argument`.
- `observe(value)`: returning `true` means an alert should go out **now**, that is, the current increasing run has **just** reached length `threshold`. The same run growing further must not alert again; a new run that starts after the old one breaks can alert again.

O(1), with no memory allocation.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is a single pass enough, with no need to enumerate every subarray? Write down your loop invariant: after processing element i, what does each of the three variables you maintain represent?
2. If "rising steadily" became a sliding-window alert, "each of the last k values is larger than the one before it," how would your state have to change?
3. Why is there such a gap in difficulty between this problem and "longest increasing **subsequence**" (where the elements need not be contiguous)?

---

**Official solution:** [Problem Set 0 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cfc7d2d5bc3b9f4df35b5bdc7aa49284_MIT6_006S20_ps0-solutions.pdf). Read it after you finish.
