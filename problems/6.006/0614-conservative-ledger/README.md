---
title: Conservative State Machine (every loop must net zero points)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 PS6 Problem 6-4
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec12.pdf, materials/6.006/lectures/lec14.pdf
kind: code
minutes: 90
---

# 0614 · Conservative State Machine

**Original problem:** [Problem Set 6 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0cb5c705752261af9da744c22925a136_MIT6_006S20_ps6_questions.pdf) · **Read first:** [Lecture 12: Bellman-Ford notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/2430d7903a5529451d80c17f89a41fe8_MIT6_006S20_lec12.pdf), [Lecture 14: ASP & Johnson’s Algorithm notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/7d7d5c35490f41b7b037cafbda7019ad_MIT6_006S20_lec14.pdf)

> **Why this comes up in practice:** billing systems, game economies, and quota management are all state machines plus points that go up and down. If even one loop that can be repeated forever earns points (or loses them), the system will be gamed sooner or later; static analysis can catch it before launch.

## Original problem

6.006 PS6 Problem 6-4 is set in a made-up physics in which a force field charges a particle some amount of work, possibly negative, to move along a path. A field is conservative when every closed loop has net work 0. The discrete version has n locations ℓᵢ and O(n) one-way transitions: transition (ℓᵢ, ℓⱼ, wᵢⱼ) takes a particle from ℓᵢ to ℓⱼ for an integer amount of work wᵢⱼ, which can be negative, and it need not have a reverse counterpart.

Give an O(n²)-time test for whether the field is conservative.

## Your task

A state machine has `states` states (`0 … states-1`), and every transition changes the user's points:

```cpp
struct Transition { int from; int to; std::int64_t delta; };
```

The system is **conservative** when, starting from any state and going around any loop back to that state, the total change in points is 0 (only loops you can actually follow in the direction of the transitions count; transitions are one-way, and `a → b` doesn't imply `b → a`). A transition can be a self-loop, and there can be several parallel transitions.

The state machine isn't necessarily connected. There may be millions of states, so **do not use recursion**.

Stage 1 is the original problem. Stage 2 also returns a loop that breaks the rule, and Stage 3 brings the check down to linear time.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — The verdict

```cpp
bool is_conservative(int states, std::span<const Transition> transitions);
```

`is_conservative(states, transitions)`: return whether the system is conservative.

Must run in O(states · |transitions|), and must hold up when `states` reaches a few thousand.

## Stage 2 — Finding the offending loop

```cpp
std::optional<std::vector<std::size_t>> find_unbalanced_cycle(
    int states, std::span<const Transition> transitions);
```

`find_unbalanced_cycle(states, transitions)`: when the system is not conservative, return a **simple** loop whose total change in points is not 0, listed in order as **indices** into `transitions` (the `to` of entry `i` is the `from` of entry `i+1`, the last one connects back to the first, and the loop passes through each state at most once). Return `std::nullopt` when the system is conservative.

Must run in O(states · |transitions|).

## Stage 3 — Linear time

Make `is_conservative` run in **O(states + |transitions|)**. At this data size (millions), Bellman-Ford won't finish in time.

(Stage 2's `find_unbalanced_cycle` doesn't need to get faster.)

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is it enough to check that there is no negative cycle and no positive cycle? How does that map to the definition of conservative?
2. Why can Stage 3 use the idea of a potential? Why can't you assign potentials over the whole graph directly? (The two edges `a → b` and `c → b` form no cycle, but their potentials may conflict.)
3. Suppose you now had to return a certificate that the system is conservative. A correct potential function is such a certificate: how long does it take to verify one?

---

**Official solution:** [Problem Set 6 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9073af3b8151002c5f63d06dc7ac961e_MIT6_006S20_ps6_solutions.pdf). Read it after you finish.
