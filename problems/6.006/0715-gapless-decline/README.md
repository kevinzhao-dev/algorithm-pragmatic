---
title: Longest Gap-Free Decline (constrained LIS)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 PS7 Problem 7-5
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec16.pdf, materials/6.006/recitations/rec16.pdf
kind: code
minutes: 110
---

# 0715 · Longest Gap-Free Decline

**Original problem:** [Problem Set 7 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/3dff7a1ecbc241e9b91b48e69a1588d4_MIT6_006S20_ps7_questions.pdf) · **Read first:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf), [Recitation 16 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/487dc976eabd0f9bcd6a3c385203ea27_MIT6_006S20_r16.pdf)

> **Why this comes up in practice:** a monitoring system looks for "a stretch in which a metric keeps getting worse, with no recovery in between" to pinpoint when an incident began. Each time bucket holds several samples, and what you want is not a global LIS but a trend across consecutive buckets. Real data has a large `k`, so O(k²) transitions can't keep up, and this exercise therefore layers a data structure on top of the DP.

## Original problem

6.006 PS7 Problem 7-5 follows a short seller who has collected stock prices for s companies C = (c_0, …, c_{s−1}) over n consecutive days. The prices of company c_i form a chronological sequence P_i = (p_0, …, p_{nk−1}) of nk positive integers, in which p_{kj}, …, p_{kj+k−1} are the k prices from day j, for j ∈ {0, …, n − 1}. A company's shorting value is the length of its longest strictly decreasing subsequence of prices that doesn't skip days: if the subsequence uses prices from days i < j, then every day in {i, …, j} must contribute at least one price to it.

- (a) Describe an O(snk²)-time algorithm that finds a company c_i with the highest shorting value and returns a longest such subsequence S of P_i.
- (b) Implement part (a) in the problem set's Python template as `short_company(C, P, n, k)`, which returns the company's name and S.

As with every PS7 problem, the solution must be a dynamic program presented through the SRT BOT steps.

## Your task

A series has `n` consecutive time buckets with exactly `k` samples each, flattened into an array of length `n·k`: the first `k` belong to bucket 0, and so on.

A valid **trend** is a strictly decreasing subsequence that doesn't skip buckets: if it contains samples from buckets `a` and `b` (`a < b`), then every bucket in `a..b` must contribute at least one sample.

Buckets are the original's days, `per_bucket` is k, and each series is one company's prices, so Stage 3's `worst_series` is part (a) of the original at its O(snk²) bound. Stage 4 then goes past the original with an O(n·k·log k) algorithm per series.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Length

```cpp
std::size_t gapless_decline_length(std::span<const int> samples, std::size_t per_bucket);
```

`std::size_t gapless_decline_length(std::span<const int> samples, std::size_t per_bucket)`

- Return the length of the longest trend; O(n·k²) time is enough (`samples.size() == n·k`).
- Throw `std::invalid_argument` when `per_bucket == 0` or `samples.size()` is not a multiple of `per_bucket`.
- Return 0 for an empty `samples`.

The next sample can be later in the same bucket, or in the next bucket.

## Stage 2 — Recovering one trend

```cpp
std::vector<std::size_t> gapless_decline(std::span<const int> samples,
                                         std::size_t per_bucket);
```

`std::vector<std::size_t> gapless_decline(std::span<const int> samples, std::size_t per_bucket)`: return the indices of one longest trend, in increasing order. If several have the same length, pick any of them.

## Stage 3 — The worst of several series

```cpp
struct WorstSeries { std::size_t series; std::vector<std::size_t> indices; };
WorstSeries worst_series(std::span<const std::vector<int>> series, std::size_t per_bucket);
```

Return the index of the series with the longest trend, along with one longest trend. Break ties however you like. Throw `std::invalid_argument` when `series` is empty or the length of any series is not a multiple of `per_bucket`.

## Stage 4 — O(n·k·log k)

`gapless_decline_length`, `gapless_decline`, and `worst_series` must all be improved to O(n·k·log k) (times the number of series for `worst_series`).

The Stage 1 transition searches a range for the candidate that has a smaller value and the longest trend. That range has two parts: the later samples in the same bucket, and all the samples in the next bucket. The next bucket is a fixed set, while the same-bucket part grows as the scan proceeds, so the two must be maintained in different ways.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does "no skipped buckets" keep the patience algorithm for LIS (0704 Stage 4) from applying directly?
2. What operations does the data structure in Stage 4 need to support? If the range were a general sliding window of "the `h` buckets before and after", how would you handle deletions?
3. `n·k` is already the input size, so why is this problem polynomial rather than pseudopolynomial?

---

**Official solution:** [Problem Set 7 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ed996e1569867432dc1fbdeae53e309f_MIT6_006S20_ps7_solutions.pdf). Read it after you finish.
