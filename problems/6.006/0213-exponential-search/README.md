---
title: Exponential Search and Finger Cursor
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 PS2 Problem 2-3
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/lectures/lec03.pdf
kind: code
minutes: 70
---

# 0213 · Exponential Search

**Original problem:** [Problem Set 2 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/70d9c6b212c6dfce6723259ffb429521_MIT6_006S20_ps2-questions.pdf) · **Read first:** [Lecture 3: Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6d1ae5278d02bbecb5c4428928b24194_MIT6_006S20_lec3.pdf)

> **Why this comes up in practice:** when you look up a point in time in terabytes of sorted logs, the most recent data is queried most often, and `git bisect` hunting for a commit that "only just broke" is the same: the target is usually close to one end. Exponential search makes the cost of a query depend on "how close" the target is, not on "how big" the whole thing is.

## Original problem

6.006 PS2 Problem 2-3, "Friend Finder," has Jean-Locutus Πcard searching Gravity Island for his incapacitated friend Datum. The island is a thin strip that runs n kilometers from north to south, and Πcard must locate Datum to the nearest whole kilometer. A tracking device tells him whether Datum is north or south of where he is standing, but not how far away; a teleporter takes him to any coordinate on the island in constant time. The island is sinking, ends first and middle last, so speed matters most when Datum is near an end.

Describe an algorithm that finds Datum after visiting O(log k) locations, where k is his distance in kilometers from the nearer end of the island (so he is at kilometer k or kilometer n − k, counted from the north). The answer is written; there is no code.

## Your task

The original problem searches for a target along a line of length `n`: each time you teleport to a position, a device tells you whether the target is ahead of you or behind you, and when the target is `k` from the nearer end, you must find it within O(log k) steps. Here the device becomes an oracle function, and **the tests count the probes**.

`Direction` has `Less` (the target is before this position), `Here` (it's right here), and `Greater` (the target is after this position). The oracle is a callable `Direction oracle(std::int64_t position)`. **Do not probe positions outside the range** (the tests check this).

You build binary search, then one-sided and two-sided exponential search, and finish with a finger cursor that gallops from where it last stopped, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Binary Search

```cpp
enum class Direction { Less, Here, Greater };

template <typename Oracle>
std::int64_t locate_in_range(std::int64_t low, std::int64_t high, Oracle oracle);
```

`locate_in_range(low, high, oracle)`: the target is in the half-open interval `[low, high)` (it is guaranteed to exist, and `low < high`). Return the target's position, using at most `⌈log₂(high − low + 1)⌉ + 1` probes.

## Stage 2 — Exponential search from the start

```cpp
template <typename Oracle>
std::int64_t locate_from_start(std::int64_t n, Oracle oracle);
```

`locate_from_start(n, oracle)`: the target is in `[0, n)` but is usually close to 0. If the target is `t`, the number of probes must be **O(log(t + 2))** and independent of `n` (`n` can be as large as 2⁶⁰).

Hint: start at 0 and jump 1, 2, 4, 8… until you pass the target, then binary search the last stretch.

## Stage 3 — Two-sided exponential search (the original problem)

```cpp
template <typename Oracle>
std::int64_t locate_two_sided(std::int64_t n, Oracle oracle);
```

`locate_two_sided(n, oracle)`: the target is in `[0, n)`, and may be close to 0 or close to `n − 1`. With `k = min(t, n − 1 − t)`, the number of probes must be **O(log(k + 2))** (`n` can again be as large as 2⁶⁰). Alternate between the two ends as you probe inward; do not finish one side first.

## Stage 4 — Real-world extension: Finger Cursor

```cpp
template <typename T>
class FingerCursor {
public:
    explicit FingerCursor(std::span<const T> sorted);

    std::size_t seek(const T& key);
    std::size_t position() const;
};
```

On sorted column data, queries often come one after another, each close to the previous one (in a merge join, or when reading a posting list in order, for example).

`FingerCursor<T>`:

- `explicit FingerCursor(std::span<const T> sorted)`: the cursor starts at 0.
- `seek(key)`: return the first index with `sorted[i] >= key` (return `size()` when every element is less than key), and move the cursor there.
- `position()`: the cursor's current position.

If the answer is `d` positions away from the current cursor, the number of comparisons must be **O(log(d + 2))**: so you have to gallop **in both directions**, and you must not binary search the whole array from scratch every time. Elements may repeat (return the first one that is `>= key`).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does O(log k) rather than O(log n) matter in practice? Give an example you would actually run into.
2. What happens if the doubling uses a factor of 3 instead? Why is 2 a good choice, and what happens with 1 (a linear scan)?
3. What do finger search and the cursors of a skip list or a B-tree have in common?

---

**Official solution:** [Problem Set 2 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/b06b8a8baf9b15f98947e75bae55b8bf_MIT6_006S20_ps2-solutions.pdf). Read it after you finish.
