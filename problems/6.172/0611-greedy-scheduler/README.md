---
title: Greedy Scheduler and the Work/Span Laws
unit: 06 · Work/Span Theory (HW5)
source: 6.172 HW5 Write-up 1 (CLRS 27.1-3), Write-up 4; Recitation R4
pdf: materials/6.172/homework/hw05/handout.pdf
answer: materials/6.172/recitations/r4/solutions.pdf
read: materials/6.172/lectures/lec08.pdf, materials/6.172/recitations/r4/problems.pdf
kind: code
minutes: 120
---

# 0611 · Greedy Scheduler and the Work/Span Laws

**Original problem:** [HW5: Theory of Performance Engineering](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/cd57dbdecbce3bd4da39d620c1e61d14_MIT6_172F18hw5.pdf) · **Read first:** [Analysis of Multithreaded Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/5655e50541f85421cc47b7c85d8e5de3_MIT6_172F18_lec8.pdf), [R4: Analysis of Multithreaded Programs Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/4d86bef65ea41cddc2655ef8550de0e2_MIT6_172F18_rec4.pdf)

> **Why this comes up in practice:** `make -j`, CI pipelines, and parallel data pipelines are all "a dependency graph + P workers." A greedy scheduler is at most twice as slow as the optimal schedule, and that guarantee is why none of these systems has to solve the NP-hard problem of optimal scheduling.

## Original problem

6.172 HW5 is a written, take-home problem set on the theory in Chapter 27 of CLRS (*Introduction to Algorithms*, 3rd edition); it asks for proofs and arguments, not code. The model: a computation is a dag of strands run on an ideal parallel computer. Its work T₁ is the total time of all strands, its span T∞ is the length of a longest path, and T_P is its running time on P processors. The Work Law says T_P ≥ T₁/P, and the Span Law says T_P ≥ T∞. A greedy scheduler runs as many ready strands as it can at every step, up to P of them.

- **Write-up 1** (CLRS Exercise 27.1-3): prove that a greedy scheduler always achieves T_P ≤ (T₁ − T∞)/P + T∞, slightly stronger than the chapter's bound T_P ≤ T₁/P + T∞.
- **Write-up 4** (CLRS Exercise 27.1-5): a professor claims that her deterministic program, run on 4, 10, and 64 processors of an ideal parallel computer with a greedy scheduler, took T₄ = 80 seconds, T₁₀ = 42 seconds, and T₆₄ = 10 seconds. Argue that these claims can't all be true.

Recitation 4 asks for the work, span, and parallelism of a computation dag whose strands all take unit time (the dag is a figure that isn't reproduced here), and asks you to rank five recursive Fibonacci programs that place `cilk_spawn` differently, assuming that a spawn costs about 10 times an ordinary call.

## Your task

This exercise turns the theorems of CLRS Chapter 27 into code and checks them on the spot with random DAGs. A "strand" is a unit-time, indivisible piece of serial code; the DAG's vertices are strands, and an edge `u → v` means that `u` must finish first.

Instead of proving the bounds on paper, you compute work and span, simulate greedy schedules, build the instance where two greedy schedules differ by nearly 2×, and apply the same model to a build system.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Work and span

```cpp
class ComputationDag {
public:
    explicit ComputationDag(std::size_t strands);

    void add_edge(std::size_t before, std::size_t after);

    std::size_t size() const;
    const std::vector<std::size_t>& successors(std::size_t strand) const;

    std::size_t work() const;
    std::size_t span() const;
    double parallelism() const;
};
```

`ComputationDag(n)`: `n` strands (`0..n-1`). `add_edge(u, v)`: `u` must come before `v`. Throw `std::out_of_range` for an out-of-range index, and `std::invalid_argument` when `u == v`.

- `size()` / `work()` (unit time, so work = the number of strands)
- `successors(v)`: the strands that `v` has an edge to (the tests walk the DAG through it).
- `span()`: the **number of vertices** on a longest path; 0 for an empty DAG; throw `std::logic_error` if there's a cycle. Use an iterative O(V+E) version (the tests include a chain a million long, which would blow the stack with recursion).
- `parallelism()`: `work / span`; return 1 for an empty DAG. Like `span()`, it throws `std::logic_error` if there's a cycle.

## Stage 2 — Greedy scheduling

```cpp
struct Schedule {
    std::vector<std::size_t> step_of;
    std::size_t length = 0;
};

Schedule greedy_schedule(const ComputationDag& dag, std::size_t processors,
                         const std::vector<std::size_t>& priority);
```

`greedy_schedule(dag, p, priority)` returns `Schedule{step_of, length}`: `step_of[v]` is the time step in which `v` runs (counting from 0), and `length` is the total number of steps (0 for an empty DAG).

At each time step, a strand whose predecessors have all finished (in earlier time steps) is ready. If `>= p` strands are ready, pick the `p` of them with the smallest `priority[v]` (ties go to the lower strand number); otherwise run them all. `p >= 1` (throw `std::invalid_argument` when `p == 0`). Throw `std::logic_error` if there's a cycle. Complexity: O((V+E) log V).

On random DAGs, the tests check that all three hold: `T_p ≥ T1/p` (the Work Law), `T_p ≥ T∞` (the Span Law), and CLRS 27.1-3's `T_p ≤ (T1 − T∞)/p + T∞`.

## Stage 3 — Two greedy schedules a factor of two apart

```cpp
struct NearTwiceInstance {
    ComputationDag dag;
    std::vector<std::size_t> good_priority;
    std::vector<std::size_t> bad_priority;
};

NearTwiceInstance near_twice_instance(std::size_t processors, std::size_t chain_length);
```

CLRS 27.1-4 (the exercise just before HW5 Write-up 4, which is 27.1-5; HW5 itself doesn't assign 27.1-4): on the same DAG, one greedy schedule can be nearly twice as slow as another.

`near_twice_instance(p, k)` returns a DAG and two priorities (`good_priority` and `bad_priority`) such that `T_bad / T_good` approaches `2 - 1/p` as `k` grows large. Work out what this DAG looks like yourself (hint: a chain of length `k`, plus enough work that can run in parallel).

The tests also require the DAG to have exactly `k + (p-1)·k` strands, the good priority to finish in exactly `k` steps, and each priority to have one entry per strand (the two must differ).

## Stage 4 — Real-world extension: Build system

```cpp
struct BuildTask {
    long long duration = 1;
    std::vector<std::size_t> deps;
};

struct BuildPlan {
    long long total_work = 0;
    long long critical_path = 0;
    long long makespan = 0;
};

BuildPlan plan_build(std::span<const BuildTask> tasks, std::size_t workers);
```

`plan_build(tasks, workers)`: each `BuildTask{duration, deps}` has an integer `duration >= 1`, and `deps` lists the numbers of earlier tasks. Return `BuildPlan{total_work, critical_path, makespan}`.

Scheduling rule: whenever a worker is free, it starts the ready task with the lowest number (non-preemptive list scheduling). `critical_path` is the total duration along a longest path. Complexity: O((V+E) log V). The time at which every task has finished is the `makespan`. (This is exactly `make -jN`, and Graham's bound `makespan ≤ (T1−T∞)/p + T∞` still holds here.)

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Prove 27.1-3 without looking at the textbook: classify the steps as complete or incomplete, derive `T_p ≤ T1/p + T∞`, and explain how to tighten it further to `(T1−T∞)/p + T∞`. (HW5 Write-up 1)
2. Why is the guarantee of work stealing (`T_p ≤ T1/p + O(T∞)`) also close to this bound? Where's the difference?
3. Build the DAG from R4's problem (work/span/parallelism) with your `ComputationDag`, and use it to check your answer.

---

**Official solution:** [R4 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b5784797a007ded8f3276eed5e8c1d0d_MIT6_172F18_rec4sol.pdf). Read it after you finish.
