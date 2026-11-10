---
title: Placing Tasks on Servers by Priority (sequence-alignment DP)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 PS7 Problem 7-2
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 70
---

# 0712 · Placing Tasks on Servers by Priority

**Original problem:** [Problem Set 7 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/3dff7a1ecbc241e9b91b48e69a1588d4_MIT6_006S20_ps7_questions.pdf) · **Read first:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf)

> **Why this comes up in practice:** cluster schedulers often require that "more important tasks must go on closer, faster servers", while overcommitting resources as little as possible. The rule that pairs high priority with low latency squeezes the problem into a one-dimensional order, so instead of solving a general assignment problem, an LCS-shaped table is enough.

## Original problem

6.006 PS7 Problem 7-2 is about a pet owner with n tigers and n² cages.

- Tiger i has a positive integer age a_i and a positive integer size s_i. All ages are distinct, and so are all sizes.
- Cage j has a positive integer capacity c_j and a distance d_j from the owner's bedroom. All capacities are distinct, and so are all distances.

Each tiger needs a cage to itself. Older tigers get the closer cages: for any tigers x and y with a_x < a_y, their cages X and Y must satisfy d_Y < d_X. A tiger i housed in cage j has discomfort s_i − c_j if s_i > c_j, and no discomfort if s_i ≤ c_j. The problem asks for an O(n³)-time algorithm that finds an assignment obeying the age rule with the least total discomfort. As with every PS7 problem, the solution must be a dynamic program presented through the SRT BOT steps.

## Your task

There are `n` tasks and `m` servers (usually `m ≥ n`, and `m` may be far larger than `n`; each stage says what to do when `n > m`). Each task goes on exactly one server, and each server hosts at most one task.

```cpp
struct Task { int priority; int demand; };      // all priorities distinct; larger = more important
struct Server { int latency; int capacity; };   // all latencies distinct
```

- **Order rule:** if task `a` has a larger `priority` than task `b`, the server hosting `a` must have a **strictly smaller** `latency` than the one hosting `b`.
- **Overload cost:** placing a task on a server with capacity `c` costs `max(0, demand − c)`.

The goal is the smallest total cost that obeys the order rule.

This is the original problem with tasks in place of tigers (`priority` for age, `demand` for size) and servers in place of cages (`latency` for distance); with m = n² servers, O(n·m) is the original O(n³) bound. Beyond the minimum cost, you also return the assignment, cut the memory, and allow tasks to be dropped.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Minimum cost

```cpp
std::optional<std::int64_t> min_overload(std::span<const Task> tasks,
                                         std::span<const Server> servers);
```

`std::optional<std::int64_t> min_overload(std::span<const Task> tasks, std::span<const Server> servers)`: O(n·m) time (not counting sorting). Return `std::nullopt` when `n > m`, and 0 when there are no tasks.

Think it through first: once both sides are sorted, the answer looks like picking a subsequence of one sequence and aligning it with the other sequence.

## Stage 2 — The actual assignment

```cpp
std::optional<std::vector<std::size_t>> assign_servers(std::span<const Task> tasks,
                                                       std::span<const Server> servers);
```

`std::optional<std::vector<std::size_t>> assign_servers(tasks, servers)`: the `i`-th value in the returned vector is the index of the server that `tasks[i]` is placed on (in input order, not in sorted order). The total cost must be the minimum; when several assignments tie, any of them is fine. Return `std::nullopt` when `n > m`.

## Stage 3 — Cut `min_overload` to O(m) memory

The relation looks only at the next row, so the memory shouldn't grow with `n`. The heap high-water mark must stay within O(m) (the tests allow at most 96 bytes per server).

## Stage 4 — Real-world extension: allowing tasks to be dropped

When there aren't enough servers or the cost is too high, a scheduler simply rejects tasks:

```cpp
struct DropPlan { std::int64_t cost; std::vector<std::optional<std::size_t>> assignment; };
DropPlan plan_with_drops(std::span<const Task> tasks, std::span<const Server> servers,
                         std::span<const std::int64_t> drop_penalty);
```

`drop_penalty[i] ≥ 0` is the cost of dropping `tasks[i]`, and `assignment[i]` is the server it is placed on, or `std::nullopt`. The order rule must still hold **among the tasks that are placed**. Return the plan with the lowest total cost; it must also handle `n > m`. O(n·m) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does the order rule reduce the problem from general assignment (Hungarian, O(n³) or more) to O(n·m)? Does this DP still work once the rule is removed?
2. Stage 2 has to recover the solution. When the table is too big, how would you shrink the storage for the parents (how many bits does one cell need)?
3. Given that `m` is part of the input size, is this algorithm polynomial or pseudopolynomial?

---

**Official solution:** [Problem Set 7 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ed996e1569867432dc1fbdeae53e309f_MIT6_006S20_ps7_solutions.pdf). Read it after you finish.
