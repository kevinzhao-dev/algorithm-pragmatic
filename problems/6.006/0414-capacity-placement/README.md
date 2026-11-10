---
title: Capacity Scheduling (a heap plus a hash table)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 PS4 Problem 4-4
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec08.pdf, materials/6.006/recitations/rec08.pdf
kind: code
minutes: 75
---

# 0414 · Capacity Scheduling

**Original problem:** [Problem Set 4 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bff0aaeeb571a77083f10fd8e31f71d4_MIT6_006S20_ps4-questions.pdf) · **Read first:** [Lecture 8: Binary Heaps notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/40d4851e550507ca14dc778b9b2266cc_MIT6_006S20_lec8.pdf), [Recitation 8 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/701c2f339890cbf9c0c2a3495d007f26_MIT6_006S20_r08.pdf)

> **Why this comes up in practice:** the Kubernetes scheduler, a database's connection-pool allocation, and a CDN's traffic scheduling all come down to fitting demand onto machines that still have capacity. No single data structure solves it; the real engineering is wiring the heap, the hash table, and the pointers between them together correctly.

## Original problem

6.006 PS4 Problem 4-4 is about a town's n solar farms. Farm i is a pair (s_i, c_i): its address s_i is a positive integer that no other farm shares, and its capacity c_i is a positive integer, the highest rate of energy production it can sustain. A building is a pair (b_j, d_j): its name b_j is a string that no other building shares, and its demand d_j is a positive integer, the rate at which it consumes energy. A powered building is connected to exactly one farm, and the demands of all the buildings connected to one farm may add up to at most that farm's capacity. Design a database with the operations below, and state for each one whether its time bound is worst-case, expected, and/or amortized.

- `initialize(S)`: set up the database in O(n) time from the list S = ((s_0, c_0), …, (s_{n−1}, c_{n−1})) of the n farms.
- `power_on(b_j, d_j)`: in O(log n) time, connect building b_j with demand d_j to any farm whose remaining capacity is at least d_j, or report that no such farm exists.
- `power_off(b_j)`: disconnect building b_j in O(log n) time.
- `customers(s_i)`: in O(k) time, list the names of the k buildings that the farm at address s_i supplies.

## Your task

A cluster has `n` machines, each with a fixed capacity. A task (identified by a unique string name) has a resource requirement, `demand`, and placing it on a machine must satisfy one rule: the total demand on a machine must not exceed its capacity.

You should `#include "../0406-updatable-heap/solution.hpp"` and reuse the heap with updatable priorities, together with a hash table.

The stages below give the interface of `CapacityPlanner`. It is the database from the original problem, with machines in place of farms and tasks in place of buildings; Stage 4 adds node draining, which the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Placing tasks

```cpp
class CapacityPlanner {
public:
    using MachineId = std::uint64_t;

    struct Machine {
        MachineId id;
        std::uint64_t capacity;
    };

    explicit CapacityPlanner(std::span<const Machine> machines);
    std::optional<MachineId> place(const std::string& task, std::uint64_t demand);
    std::uint64_t available(MachineId machine) const;
};
```

- `explicit CapacityPlanner(machines)`: O(n).
- `place(task, demand)`: put the task on **any** machine with `available ≥ demand` and return that machine's id. If it doesn't fit anywhere, return `std::nullopt` and leave the state unchanged. Expected O(log n). Throw `std::invalid_argument` if `task` already exists.
- `available(machine)`: how much capacity this machine has left, in expected O(1); throw `std::out_of_range` for an unknown id.

A task fits when `available == demand`.

## Stage 2 — Releasing

```cpp
// in class CapacityPlanner
void release(const std::string& task);
```

`release(task)`: remove the task from its machine and give the capacity back. Expected O(log n). Throw `std::out_of_range` for an unknown task. Once a task is released, its name can be used again.

The key point: you can only update a machine in O(log n) if you hold its position in the heap. This is where the handles you just built come in.

## Stage 3 — Queries

```cpp
// in class CapacityPlanner
std::vector<std::string> tasks_on(MachineId machine) const;
```

`tasks_on(machine)`: the names of all the tasks on this machine (in any order), in O(k), where `k` is the number returned. Throw `std::out_of_range` for an unknown machine.

## Stage 4 — Real-world extension: Node Drain

```cpp
// in class CapacityPlanner
bool drain(MachineId machine);
```

Before maintenance on a machine, its tasks must be moved off first:

`drain(machine)`: move every task on this machine to **other machines** (every task needs somewhere to go, and you may place them in any order; moving a task back onto the same machine doesn't count). If everything moves, return `true`; afterwards the machine has no tasks and can take placements again. If even one task can't be moved, return `false`, and **the whole state must be restored exactly** (every task back on its original machine, and every machine's `available` unchanged). Throw `std::out_of_range` for an unknown machine. If the machine has no tasks, return `true`.

(Whether `drain` succeeds can depend on the placement order. That is the nature of bin packing, so the tests only check cases whose outcome is not in doubt.)

## Interview follow-ups

Once you're done, answer without looking at your code:

1. If `demand` is large, why is looking at the emptiest machine enough for `place` to decide whether the task fits?
2. Which operations of the heap and hash table used here have **expected** bounds, and which have **worst-case** bounds? If the interviewer required worst-case O(log n) for everything, what would you swap in?
3. What does the drain rollback need to record? What is its time complexity when it fails?

---

**Official solution:** [Problem Set 4 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0462839ce975fea99f6a7b8adc8e1477_MIT6_006S20_ps4-solutions.pdf). Read it after you finish.
