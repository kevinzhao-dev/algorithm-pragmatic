---
title: Two Maintenance Windows (a schedule if one exists, evidence if not)
unit: 05 · Graph Search (PS5)
source: 6.006 PS5 Problem 5-3
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 80
---

# 0513 · Two Maintenance Windows

**Original problem:** [Problem Set 5 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/8c155b28c87cc1fe3f506de2036bdf1b_MIT6_006S20_ps5_questions.pdf) · **Read first:** [Lecture 9: Breadth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/196a95604877d326c6586e60477b59d4_MIT6_006S20_lec9.pdf), [Recitation 9 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6afd7e9a85d7d36e204533569f9fccf6_MIT6_006S20_r09.pdf)

> **Why this comes up in practice:** services that share a host can't be restarted in the same batch, and migrations that would deadlock each other can't run at the same time. When things don't fit, your boss doesn't want a flat "no"; they want "these ones conflict with each other, so no."

## Original problem

6.006 PS5 Problem 5-3 has two androids planning an engagement party for their robot friends, except that many pairs of those friends short-circuit when they share a room. So they consider splitting the guests across two parties, with each friend attending exactly one and no short-circuiting pair at the same party. Three friends who short-circuit pairwise already make that impossible.

From the list of the n conflicting pairs, give an O(n)-time algorithm that decides whether such a split exists.

## Your task

You must restart every service within two maintenance windows. The input is a list of conflicts:

```cpp
using Conflict = std::pair<std::string, std::string>;
```

`{a, b}` means `a` and `b` must not restart in the same window. Service names are arbitrary strings (they can be empty, and they can be long). **Only services that appear in the conflict list count**, and you don't know in advance how many there are. The same pair may appear more than once, or in reverse order; `{x, x}` means `x` conflicts with itself, so it can never be scheduled.

`n` is the number of conflicts, and the length of a service name counts as a constant.

Stage 1 is the original yes-or-no question. Stage 2 also asks for the schedule or, failing that, an odd cycle as proof, and Stage 3 finds the first conflict that makes scheduling impossible.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Does it fit?

```cpp
bool can_fit_two_windows(std::span<const Conflict> conflicts);
```

`can_fit_two_windows(conflicts)`: whether all the services can be split between the two windows so that the two ends of every conflict are in different windows.

Expected O(n).

## Stage 2 — A schedule or the evidence

```cpp
struct Partition {
    std::vector<std::string> first;
    std::vector<std::string> second;
};

struct OddCycle {
    std::vector<std::string> members;
};

std::variant<Partition, OddCycle> plan_two_windows(std::span<const Conflict> conflicts);
```

`plan_two_windows(conflicts)` returns `std::variant<Partition, OddCycle>`:

- `Partition{first, second}`: when the services fit, each service appears exactly once, in one of the two, and no window contains a conflict.
- `OddCycle{members}`: when they don't fit, `members` holds **distinct** service names, has odd length, and every two adjacent members conflict (including the last one with the first). For `{x, x}`, return the cycle that contains only `x`.

Expected O(n).

## Stage 3 — Which conflict broke it

```cpp
std::optional<std::size_t> first_breaking_conflict(std::span<const Conflict> conflicts);
```

Conflicts are discovered one at a time, in chronological order. `first_breaking_conflict(conflicts)`: return the smallest index `i` such that the first `i + 1` conflicts can no longer be scheduled, or `std::nullopt` if all of them can.

Redoing the check after every conflict is O(n²). You must achieve **O(n log n)** or better.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is having no odd cycle equivalent to fitting into two windows? Which step of your algorithm relies on this?
2. If conflicts stream in continuously and you must answer "does it still fit?" as soon as each one arrives, what data structure would you use?
3. If you also wanted the two windows to hold as close to the same number of services as possible, what problem would this become? How much would the complexity change?

---

**Official solution:** [Problem Set 5 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/23d4cf5421c77bb55b3735f7e8f70dfe_MIT6_006S20_ps5_solutions.pdf). Read it after you finish.
