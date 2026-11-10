---
title: Set-Associative LRU Cache Simulator
unit: 11 · Caching (HW8)
source: 6.172 Lecture 14 (cache structure, 3C misses); Recitation 8 Problems 1–2
pdf: materials/6.172/lectures/lec14.pdf
answer: materials/6.172/recitations/r8/solutions.pdf
read: materials/6.172/recitations/r8/problems.pdf
kind: code
minutes: 120
---

# 1101 · Set-Associative LRU Cache Simulator

**Based on:** [Caching and Cache-Efficient Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/329bfc6e1808c375afa517feb3c4c273_MIT6_172F18_lec14.pdf) · **Read first:** [R8: Caching and Cachegrind Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/ea9f0adaa90fd8c13b3cfcd90c419e5d_MIT6_172F18_rec8.pdf)

> **Why this comes up in practice:** cachegrind isn't available on this machine, but cache behavior can be simulated. Write your own deterministic cache model, and you can ask "how many misses does a different layout of this data structure really save", with reproducible results. It's also where your intuition for false sharing gets built.

## Original problem

6.172 Recitation 8 Problems 1 and 2 are multiple-choice questions about caches:

- Problem 1: what advantages does a set-associative cache have over a direct-mapped cache with the same total capacity? The choices: (A) fewer cold misses, (B) fewer capacity misses, (C) less false sharing, (D) fewer conflict misses, (E) lower latency.
- Problem 2: on a 12-core machine, a program keeps a global array `sums` of 12 `int`s, one per chunk. A `cilk_for` loop calls a function for each of the 12 chunk ids in parallel, and that function sets its chunk's entry to 0 and then adds the integers 0 through 4999 into it one at a time. Which phenomena could make this code use the L1 cache ineffectively? The choices: (A) true sharing, (B) false sharing, (C) capacity misses, (D) conflict misses, (E) TLB misses.

## Your task

The later exercises use this exercise's simulator too (they use `_shared/08_cache_sim.hpp`, which has the same interface). Write it yourself once first, and afterwards you'll know what the miss numbers mean. Multilevel caches and miss-ratio curves come in 1103.

> ⚠️ **Spoiler warning:** `_shared/08_cache_sim.hpp` is a complete simulator, which amounts to the solution to this exercise and most of 1103; don't open it until you're done.

Instead of answering the recitation's multiple-choice questions, you build the simulator that measures the answers: a set-associative LRU cache, the 3C classification of misses, and per-core caches with invalidation.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Direct-mapped

```cpp
class CacheSimulator {
public:
    CacheSimulator(std::size_t capacity_bytes, std::size_t line_bytes, std::size_t ways);

    bool access(std::uint64_t address);
    void access_range(std::uint64_t address, std::size_t bytes);
    std::uint64_t accesses() const;
    std::uint64_t misses() const;
    std::uint64_t hits() const;
};
```

`CacheSimulator(capacity_bytes, line_bytes, ways)`: capacity = sets × ways × line. `line_bytes` must be a power of 2, `sets` must also be a power of 2, and `ways ≥ 1`; otherwise the constructor throws `std::invalid_argument`.

- `access(address)`: access one address and return `true` on a hit.
- `access_range(address, bytes)`: access each line that `[address, address + bytes)` covers exactly once; `bytes == 0` does nothing.
- `accesses()`, `misses()`, `hits()`: running totals. An `access_range` that spans 3 lines counts as 3 accesses.

This stage tests only direct-mapped behavior, `ways == 1` (the geometry checks and the overflow test also build 2- and 4-way caches, but they touch each line only once): each set holds a single line, two addresses within one line must count as a hit, and different lines in the same set must evict each other. When `line_bytes == 1` or there is only one set, the offset or the index has 0 bits. The address `~0` is a legal address too, so you must not use any tag value as an "empty" marker, and `access_range` must not overflow when it reaches the top of the address space.

## Stage 2 — Set-associative LRU

General `ways`: each set holds at most `ways` lines, and when it is full, evict the least recently used one (LRU). A hit must also update the recency. With `ways == capacity / line`, the cache is fully associative. The cost of one access may depend only on `ways`; it must not slow down as the total number of lines grows (a `PERF` test checks this with a million sets).

## Stage 3 — 3C classification

```cpp
// in class CacheSimulator
std::uint64_t cold_misses() const;
std::uint64_t conflict_misses() const;
std::uint64_t capacity_misses() const;
```

Classify every miss into one of three counters:

- `cold_misses()`: the line has never been accessed before.
- `conflict_misses()`: not cold, but a **fully associative LRU cache of the same capacity** would not have missed.
- `capacity_misses()`: not cold, and full associativity wouldn't have saved it either.

So you need to maintain a fully associative shadow cache of the same capacity internally. `cold + conflict + capacity == misses` always holds.

## Stage 4 — Multiple cores and false sharing

```cpp
class CoherentCaches {
public:
    CoherentCaches(std::size_t cores, std::size_t capacity_bytes, std::size_t line_bytes,
                   std::size_t ways);

    bool read(std::size_t core, std::uint64_t address);
    bool write(std::size_t core, std::uint64_t address);
    std::uint64_t coherence_misses() const;
};
```

`CoherentCaches(cores, capacity_bytes, line_bytes, ways)`: one private cache per core (following `CacheSimulator`'s rules).

- `read(core, address)` / `write(core, address)`: return `true` on a hit; a miss loads the line into that core's cache.
- `write` invalidates the same line in every **other** core's cache (invalidation: the line is removed from them).
- `coherence_misses()`: the number of misses a core takes because another core's earlier write invalidated the line (summed over all cores; cold misses don't count).

The tests check Recitation 8 Problem 2's `sums[chunk_id]` array directly: when adjacent elements share a line, coherence misses explode; when every element has a line of its own, there are none at all.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. At the same capacity, which kind of miss does a set-associative cache have fewer of than a direct-mapped one? Why don't cold misses go down? (Recitation 8 Problem 1)
2. Real L1 caches are mostly 8-way rather than fully associative. What is the trade-off?
3. If you turned Stage 4's counts into "time", roughly how many cycles does false sharing cost? If each thread writes only one `int`, how can you fix it? (`alignas`, padding, per-thread counters plus a reducer)

---

**Official solution:** [R8 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/03993e6ee1513a35460c2ffaf7b07fd6_MIT6_172F18_rec8sol.pdf). Read it after you finish.
