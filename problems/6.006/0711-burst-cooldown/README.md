---
title: Burst Scheduling and Cooldowns (linear DP with state)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 PS7 Problem 7-1
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec15.pdf
kind: code
minutes: 70
---

# 0711 · Burst Scheduling and Cooldowns

**Original problem:** [Problem Set 7 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/3dff7a1ecbc241e9b91b48e69a1588d4_MIT6_006S20_ps7_questions.pdf) · **Read first:** [Lecture 15: Dynamic Programming, Part 1: SRBOT, Fib, DAGs, Bowling notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9eb3e9a51a7b5b60b0f67c2277f8b0ee_MIT6_006S20_lec15.pdf)

> **Why this comes up in practice:** GPU and CPU boost clocks, burst credits in the cloud, and API cooldowns all follow the pattern "do a bit more now, then stay at the baseline for the next few time slots". A scheduler has to decide ahead of time when a burst pays off, rather than bursting whenever it can.

## Original problem

6.006 PS7 Problem 7-1 is set in a primary election that a candidate is projected to lose. There are n state contests, held one per day over n days. Contest i ∈ {1, …, n} has a delegate count d_i, a positive integer, and a projected delegate count z_i < d_i: the number of those delegates she would win if she did nothing more. In total there are D = Σ_i d_i delegates, winning requires at least ⌊D/2⌋ + 1, and Σ_i z_i < ⌊D/2⌋ + 1.

On any given day, her team may campaign in a single state or not at all. A campaign on day i captures all d_i delegates of state i, after which the team spends the next two days relocating and can't campaign anywhere. The problem asks for an O(n)-time algorithm that decides whether some choice of campaign days lets her win. As with every PS7 problem, the solution must be a dynamic program presented through the SRT BOT steps.

## Your task

There are `n` consecutive time slots. Slot `i` normally processes `base` units, or `burst` units when it bursts (`0 ≤ base < burst`). If slot `i` bursts, **none of the next `cooldown` slots may burst**; each of them processes only its own `base`.

```cpp
struct Slot { std::int64_t base; std::int64_t burst; };
```

This is the original problem in scheduling terms: a slot is a day, `base` and `burst` are z_i and d_i, a burst is a campaign day, `cooldown = 2` is the two days of relocation, and meeting the quota ⌊D/2⌋ + 1 is the original yes-or-no question. Beyond that question, you also compute the maximum throughput, return a schedule, support any cooldown, and cap the number of bursts.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Maximum total throughput (`cooldown = 2`)

```cpp
std::int64_t max_throughput(std::span<const Slot> slots, std::size_t cooldown = 2);
bool can_meet_quota(std::span<const Slot> slots, std::int64_t quota, std::size_t cooldown = 2);
```

`can_meet_quota`: whether the total throughput can reach `quota`. O(n) time. This stage only needs to handle the default `cooldown`.

## Stage 2 — The schedule

```cpp
std::vector<std::size_t> burst_plan(std::span<const Slot> slots,
                                    std::size_t cooldown = 2);
```

`std::vector<std::size_t> burst_plan(std::span<const Slot> slots, std::size_t cooldown = 2)`: return the slots that burst, in increasing order; the total throughput must equal the optimum. When several schedules tie, any of them is fine.

## Stage 3 — Any cooldown, O(n) time, O(min(n, cooldown)) memory

Now `cooldown` can be any value (`0` means no restriction, and it may also be larger than `n`). `max_throughput` must stay O(n) time and must not slow down as `cooldown` grows; its extra memory must stay within O(min(n, cooldown)) (with `cooldown = 8`, the tests allow at most 4 KiB of heap for 200,000 slots). `burst_plan` must also support any `cooldown`, in O(n) time, but its memory isn't limited this way.

## Stage 4 — Real-world extension: a burst budget

```cpp
std::int64_t max_throughput_limited(std::span<const Slot> slots, std::size_t cooldown,
                                    std::size_t max_bursts);
```

`std::int64_t max_throughput_limited(std::span<const Slot> slots, std::size_t cooldown, std::size_t max_bursts)`: additionally limit the total number of bursts to at most `max_bursts`. O(n · max_bursts) time, O(n) memory.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Would deciding feasibility directly in terms of `quota` be faster than computing the maximum first and then comparing? Why are "can we reach the target?" and "what is the most we can get?" the same question here?
2. Why can Stage 3 keep only the most recent `cooldown + 1` subproblems? If the cooldown varied from slot to slot, would this memory-saving trick still work?
3. Is this polynomial or pseudopolynomial? What about Stage 4? (`max_bursts` never needs to be larger than `n`.)

---

**Official solution:** [Problem Set 7 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ed996e1569867432dc1fbdeae53e309f_MIT6_006S20_ps7_solutions.pdf). Read it after you finish.
