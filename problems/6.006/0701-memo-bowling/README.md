---
title: Memo and Bowling (SRT BOT linear DP)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 Lecture 15
pdf: materials/6.006/lectures/lec15.pdf
read: materials/6.006/lectures/lec15.pdf, materials/6.006/recitations/rec15.pdf
kind: code
minutes: 75
---

# 0701 · Memo and Bowling

**Based on:** [Lecture 15: Dynamic Programming, Part 1: SRBOT, Fib, DAGs, Bowling notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9eb3e9a51a7b5b60b0f67c2277f8b0ee_MIT6_006S20_lec15.pdf) · **Read first:** [Lecture 15: Dynamic Programming, Part 1: SRBOT, Fib, DAGs, Bowling notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9eb3e9a51a7b5b60b0f67c2277f8b0ee_MIT6_006S20_lec15.pdf), [Recitation 15 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6022dc36557c5e5df2ad0a77e9463855_MIT6_006S20_r15.pdf)

> **Why this comes up in practice:** caching the results of an expensive function is the top-down form of DP, and "sweep once along one dimension, asking only a local question at each cell" is the prototype of almost every DP. Start by walking a small problem through all of SRT BOT (Subproblem, Relate, Topological order, Base, Original, Time); every exercise in this unit follows the same recipe.

## Your task

You first build a reusable memo table, then apply SRT BOT to the bowling problem from Lecture 15: the best score, a plan that reaches it, and finally a streaming version that needs only O(1) memory.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — A generic `Memo`

```cpp
template <typename Key, typename Value, typename Hash = std::hash<Key>>
class Memo {
public:
    // relation is called as relation(key, recurse); recurse(k) returns const Value&.
    template <typename Relation>
    const Value& get(const Key& key, Relation&& relation);

    std::size_t size() const;
};
```

`Memo<Key, Value, Hash>` separates the recurrence from the cache, so the caller writes only the relation:

```cpp
Memo<int, std::int64_t> memo;
auto fib = [](int n, auto& recurse) -> std::int64_t {
    return n < 2 ? n : recurse(n - 1) + recurse(n - 2);
};
memo.get(90, fib);
```

| Operation | Behavior |
| --- | --- |
| `get(key, relation)` | If the key was already computed, return its `const Value&`; otherwise call `relation(key, recurse)`, store the result, and return it. `recurse(k)` returns another subproblem's `const Value&` |
| `size()` | The number of finished subproblems |

Each key is computed at most once, and a returned reference stays valid for as long as the `Memo` lives. Get these edge cases right:

- **Cyclic dependencies:** when `relation` loops back to a key that is still being computed, throw `std::logic_error`. This is exactly the case where the T of SRT BOT fails.
- **Exception safety:** when `relation` throws, let the exception propagate. Keys still being computed leave no trace and can be retried later; finished keys are kept.
- `recurse` may make the table rehash, so don't hold on to an iterator that can be invalidated.

## Stage 2 — The best Bowling score

```cpp
std::int64_t bowling_max_score(std::span<const int> pins);
```

There is a row of `n` pins, and pin `i` has value `v[i]` (possibly negative). Each pin is handled at most once: you can skip it, hit it alone (scoring `v[i]`), or hit it together with the next pin (scoring `v[i]·v[i+1]`).

`std::int64_t bowling_max_score(std::span<const int> pins)`: return the highest total score, in O(n) time. Values satisfy `|v| ≤ 10^6`, and `n ≤ 2·10^6`.

Write down the six SRT BOT steps yourself before you start coding.

## Stage 3 — Recovering the plan

```cpp
struct Throw {
    std::size_t first;
    std::size_t last;  // first, or first + 1
};

std::vector<Throw> bowling_plan(std::span<const int> pins);
```

`std::vector<Throw> bowling_plan(std::span<const int> pins)`: return a set of throws that reaches the highest score, in O(n) time. In `Throw{first, last}`, either `last == first` (one pin alone) or `last == first + 1` (two pins together). The order doesn't matter, and when several plans tie, any of them is fine.

## Stage 4 — A streaming version with O(1) memory

Pins arrive one at a time, and you can't look at all of them first:

```cpp
class BowlingStream {
    void push(int pin);         // O(1), no heap allocation
    std::int64_t best() const;  // the best score over the pins that have arrived so far
};
```

The suffix form from Stage 2 needs to know where the input ends, so the stream needs a prefix form instead. Think carefully about how many states the relation really depends on.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When is top-down better, and when is bottom-up? When `n` reaches a million, what kills top-down first?
2. Why can Stage 4 throw away the table while Stage 3 can't? If the stream had to report the current plan at any moment, what is the least information you would need to keep?
3. `get` returns `const Value&` rather than `Value`. If the underlying storage changed from `unordered_map` to `vector`, what would break?
