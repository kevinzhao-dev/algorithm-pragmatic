---
title: Four-Machine Makespan Balancing (multidimensional subset sum)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 PS8 Problem 8-3
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/recitations/rec18.pdf, materials/6.006/lectures/lec18.pdf
kind: code
minutes: 90
---

# 0813 · Four-Machine Makespan Balancing

**Original problem:** [Problem Set 8 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/740643e3711574870a83ebfceee9a8ae_MIT6_006S20_ps8_questions.pdf) · **Read first:** [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf), [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf)

> **Why this comes up in practice:** spreading a batch of training jobs across a few GPUs, or map tasks across workers, means making the machine that finishes last finish as early as possible (the makespan). With an arbitrary number of machines this is NP-hard, but for a fixed number of machines there is an exact pseudopolynomial solution; a real scheduler usually tries the exact solution first, falls back to a greedy method such as LPT when that's too expensive, and knows that it fell back.

## Original problem

6.006 PS8 Problem 8-3 gives a set A = {a_0, …, a_{n−1}} of n distinct positive integers whose sum is m = Σ_{a_i∈A} a_i. It asks for an O(m³n)-time algorithm that returns a partition of A into four subsets A_1, A_2, A_3, A_4 ⊊ A, with A_1 ∪ A_2 ∪ A_3 ∪ A_4 = A, that makes the largest of the four subset sums as small as possible, that is, minimizes max{Σ_{a_i∈A_j} a_i | j ∈ {1, 2, 3, 4}}.

Like every PS8 problem, it must be solved with a dynamic program presented through the SRT BOT steps, and you must also say whether the requested running time is polynomial, pseudopolynomial, or exponential in the size of the input. PS8 has no coding part.

## Your task

`jobs` holds the processing times of `n` jobs (positive integers, which may repeat). Their sum is `m = Σ jobs`. Assign each job to one machine; a machine's load is the sum of its jobs, and the `makespan` is the largest load. The goal is to minimize the makespan.

```cpp
struct Assignment { std::int64_t makespan; std::vector<int> machine; };
```

`machine[i]` is the number of the machine (counting from 0) that `jobs[i]` is assigned to. With no jobs, `makespan` is 0.

Stage 3 is the original problem, with jobs as the elements of A and the four machines as the four subsets, except that job sizes may repeat. The exercise warms up with two machines and finishes with a scheduler that falls back to a greedy method when the exact solution would cost too much.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Two machines

```cpp
Assignment balance_two(std::span<const int> jobs);
```

`Assignment balance_two(std::span<const int> jobs)`: O(n·m) time.

## Stage 2 — The best makespan on four machines

```cpp
std::int64_t min_makespan_four(std::span<const int> jobs);
```

`std::int64_t min_makespan_four(std::span<const int> jobs)`: O(n·m³) time (`m` is small: at most a few hundred in the tests).

It is a three-dimensional subset sum: the state is the load of each of the first three machines, and the fourth follows from the total. Rolling the `n` dimension saves memory.

## Stage 3 — The assignment on four machines

```cpp
Assignment balance_four(std::span<const int> jobs);
```

`Assignment balance_four(std::span<const int> jobs)`: return an actual assignment whose makespan is the minimum. Also O(n·m³) time.

As with the witness in 0802, don't store `n` layers of the table (the tests allow at most 8·(m+1)³ bytes of heap). Think about "which job made this state reachable for the first time".

## Stage 4 — Real-world extension: a scheduler

```cpp
struct Plan { std::int64_t makespan; std::vector<int> machine; bool exact; };
Plan plan_jobs(std::span<const int> jobs, std::size_t machines, std::uint64_t work_limit);
```

- `machines == 1`: put everything on machine 0, and `exact` is `true`.
- `machines == 2` or `4`: if the work of the exact solution (`n·(m+1)` when `machines == 2`, and `n·(m+1)³` when `machines == 4`) doesn't exceed `work_limit`, use the exact solution and set `exact` to `true`; otherwise use LPT and set `exact` to `false`.
- Any other `machines`: always use LPT, with `exact` set to `false`.
- Throw `std::invalid_argument` when `machines == 0`.

LPT: sort the jobs from largest to smallest, and put each one on the machine with the smallest current load. LPT's makespan is at most `4/3 − 1/(3k)` times the optimum, where `k` is the number of machines, and the tests check this.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is this problem pseudopolynomial? When the number of bits in `m` grows by 1, roughly how many times longer does it take?
2. Why is `first` in Stage 3 enough to rebuild the assignment? When does the fourth machine get a job?
3. If you first compute an upper bound `U` with LPT and then trim every dimension of the DP to at most `U`, how much does the complexity actually drop? What are the risks of this trick?

---

**Official solution:** [Problem Set 8 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/39b3ead18e480e4322a8a7308daf8fa1_MIT6_006S20_ps8_solutions.pdf). Read it after you finish.
