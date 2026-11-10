---
title: NP-Completeness Proofs as Code (Triple-SAT, DONUT, SCHED)
unit: 08 · LP and NP-Completeness (PS8)
source: 6.046J PS8 Problem 8-2
pdf: materials/6.046j/psets/ps08/questions.pdf
answer: materials/6.046j/psets/ps08/solutions.pdf
read: materials/6.046j/lectures/lec16.pdf, materials/6.046j/recitations/rec08.pdf
kind: code
minutes: 160
---

# 0812 · NP-Completeness Proofs as Code

**Original problem:** [Problem Set 8](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/29a28961d8c73e013ba78238e66dc2ab_MIT6_046JS15_pset8.pdf) · **Read first:** [Complexity: P, NP, NP-completeness, Reductions](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/f2829e57caade7e7f21d765b8e7b9058_MIT6_046JS15_lec16.pdf), [Complexity: More Reductions](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/1dee182d301235b901119a0ff57f05e2_MIT6_046JS15_Recitation8.pdf)

> **Why this comes up in practice:** when a PM asks "can this schedule be guaranteed to make the most money?", the answer is often that the decision version is NP-complete, but a pseudo-polynomial DP is fast enough for the numbers you actually see. Writing the proofs as code makes the line between the two clear.

## Original problem

6.046J PS8 Problem 8-2 asks for NP-completeness proofs of three decision problems. Any problem proved NP-complete in lecture or in CLRS may serve as the source of a reduction.

- (a) TRIPLE-SAT: given a Boolean formula φ, decide whether φ has at least three different satisfying assignments. Prove that TRIPLE-SAT is NP-complete.
- (b) DONUT: Problem Set 1 placed donut shops at vertices of a road map given as any undirected graph G = (V, E). Each vertex u has a nonnegative integer profit p(u), no two shops may be on adjacent vertices, and the goal was a set U ⊆ V with the largest total profit Σ_{u∈U} p(u). The decision version DONUT takes G, the profits p, and a nonnegative integer k, and asks whether some U ⊆ V with no two vertices adjacent has Σ_{u∈U} p(u) ≥ k. Prove that DONUT is NP-hard (the hint suggests a reduction from 3SAT), and explain why a polynomial-time algorithm for the original maximization problem would imply P = NP.
- (c) SCHED: one machine and n tasks a1, …, an, where task aj needs tj units of machine time, earns profit pj, and has deadline dj, all nonnegative integers. The machine runs one task at a time. Tasks may be skipped, but a task that starts must run uninterrupted and end by its deadline. A schedule picks a subset of the tasks and a start time for each, and its profit is the sum of their pj; the goal is a schedule with the largest profit. Formulate the decision version and prove it NP-complete.

## Your task

A SAT formula is a `Cnf{num_vars, clauses}`: the variables are `1..num_vars`, and `clauses[i]` is a list of literals (positive or negative integers; `-3` means `¬x3`). An `Assignment` is a `std::vector<bool>` of length `num_vars + 1`; index 0 is unused.

The tests use brute-force solutions on small instances to check both directions of each reduction.

Each stage turns one part of the original into code: a reduction, a verifier where one is needed, and a lift that maps a witness back. Stage 4 adds the pseudo-polynomial DP for SCHED, which the original doesn't ask for.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — TRIPLE-SAT (Part a)

```cpp
struct Cnf {
    int num_vars = 0;
    std::vector<std::vector<int>> clauses;
};
using Assignment = std::vector<bool>;  // size num_vars + 1, index 0 unused

bool satisfies(const Cnf& f, const Assignment& a);
Cnf reduce_sat_to_triple_sat(const Cnf& f);
Assignment lift_triple_sat_to_sat(const Cnf& f, const Assignment& a);
```

`bool satisfies(const Cnf& f, const Assignment& a)`: the verifier for SAT (NP membership). A formula with no clauses is satisfied, and an empty clause is never satisfied.

`Cnf reduce_sat_to_triple_sat(const Cnf& f)`: return `f'` such that `f` is satisfiable ⟺ `f'` has at least three **distinct** satisfying assignments. `Assignment lift_triple_sat_to_sat(const Cnf& f, const Assignment& a)`: turn a satisfying assignment of `f'` back into a satisfying assignment of `f`. When `f` is unsatisfiable, `f'` must have no satisfying assignment at all (the tests count them).

## Stage 2 — DONUT (Part b)

A 3SAT formula is guaranteed to have exactly 3 literals in every clause (a variable may appear more than once).

```cpp
struct DonutInstance {
    int n = 0;                                   // vertices 0..n-1
    std::vector<std::pair<int,int>> edges;
    std::vector<long long> profit;               // nonnegative
    long long k = 0;
};
DonutInstance reduce_3sat_to_donut(const Cnf& f);
Assignment lift_donut_to_3sat(const Cnf& f, const DonutInstance& inst, std::span<const int> chosen);
```

`chosen` is a set of pairwise nonadjacent vertices with total profit ≥ k; return a satisfying assignment of `f`.

The tests pin down the standard gadget: one vertex of profit 1 per literal occurrence (3 per clause), a triangle on each clause's three vertices, an edge between every two occurrences of complementary literals in different clauses, and `k` equal to the number of clauses.

## Stage 3 — SCHED (Part c)

```cpp
struct Task {
    long long time = 0;
    long long profit = 0;
    long long deadline = 0;
};
struct ScheduledTask {
    int task = 0;
    long long start = 0;
};
using Schedule = std::vector<ScheduledTask>;

bool valid_schedule(std::span<const Task> tasks, const Schedule& schedule);
long long schedule_profit(std::span<const Task> tasks, const Schedule& schedule);

struct SubsetSumInstance {
    std::vector<long long> a;
    long long t = 0;
};
struct SchedInstance {
    std::vector<Task> tasks;
    long long k = 0;
};
SchedInstance reduce_subset_sum_to_sched(const SubsetSumInstance& src);
std::vector<int> lift_sched_to_subset_sum(const SubsetSumInstance& src,
                                          const SchedInstance& dst,
                                          const Schedule& schedule);
```

Tasks are `Task{time, profit, deadline}` and run on a single machine without interruption. A `Schedule` is a sequence of `(task, start)` pairs (in any order; each start is a nonnegative integer).

- `bool valid_schedule(std::span<const Task> tasks, const Schedule& schedule)`: no task appears twice, no two tasks overlap, and every task finishes by its deadline (`start + time ≤ deadline`). A task occupies `[start, start + time)`, so another task may start exactly when it ends.
- `long long schedule_profit(tasks, schedule)`: the total profit.
- The decision problem SCHED: is there a valid schedule with profit ≥ k? The reduction from `SubsetSumInstance{a, t}` is `SchedInstance reduce_subset_sum_to_sched(const SubsetSumInstance&)`, with `SchedInstance{tasks, k}`. It creates exactly one task per element of `a`.
- `std::vector<int> lift_sched_to_subset_sum(src, dst, schedule)`: turn the schedule back into a subset of indices whose sum is `t`.

## Stage 4 — Real-world extension: a pseudo-polynomial optimal schedule

```cpp
Schedule best_schedule(std::span<const Task> tasks);
```

SCHED is weakly NP-hard, which means there is a fast exact algorithm when the numbers are small. `Schedule best_schedule(std::span<const Task> tasks)`: return a valid schedule with the largest profit (the largest deadline D is small; the tests use up to 30,000 with thousands of tasks, so the deadlines' sum is not small), in O(n log n + n·D), where D = the largest deadline. Hint: sort in EDF order, then run a knapsack.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. The Stage 1 reduction works by adding just two variables. Why? What if the problem asked for "exactly three"?
2. Why does the NP-hardness of DONUT's optimization version mean that a polynomial-time algorithm for "find the best solution" would imply P = NP? (The relationship between decision and optimization.)
3. Why doesn't the Stage 4 DP prove P = NP? How does this relate to weak versus strong NP-hardness?

---

**Official solution:** [Solutions to Problem Set 8](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/e75cb61dde1769d9d9ea22069b5ce2c4_MIT6_046JS15_pset8sols.pdf). Read it after you finish.
