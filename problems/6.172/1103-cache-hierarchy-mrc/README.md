---
title: Multilevel Caches and the Miss-Ratio Curve
unit: 11 · Caching (HW8)
source: 6.172 Lecture 14 (multilevel caches); replaces Cachegrind in HW2 §2.2
pdf: materials/6.172/lectures/lec14.pdf
read: materials/6.172/homework/hw02/handout.pdf
kind: code
minutes: 80
---

# 1103 · Multilevel Caches and the Miss-Ratio Curve

**Based on:** [Caching and Cache-Efficient Algorithms](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/329bfc6e1808c375afa517feb3c4c273_MIT6_172F18_lec14.pdf) · **Read first:** [HW2: Profiling](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/796439e646c02f44348d50b1836ff7f9_MIT6_172F18hw2.pdf)

> **Why this comes up in practice:** deciding how big an L2 should be, how much memory to give Redis, or how many objects each CDN node should hold all come down to the same question: "if the capacity doubles, how many fewer misses are there?" Rerunning the simulation one capacity at a time is too slow; with stack distances, a single pass over the trace draws the whole curve.

## Original problem

6.172 HW2 §2.2 introduces Cachegrind, a Valgrind tool that simulates how a program uses the machine's cache hierarchy and branch predictor. That makes it useful on virtual machines where hardware performance counters aren't available, although its results depend on the machine it runs on.

- Checkoff 2: run `sum`, a program that allocates an array of U = 10 million elements and then adds up N = 100 million elements chosen at random, under Cachegrind, and look at the D1 misses (the first-level data cache) and the LLd misses (the last-level data cache, usually L3). Do the numbers match what you expect? Try other values of N and U: how can you bring the number of cache misses down?

## Your task

This continues 1101. Each cache level uses `practice::cachesim::Cache` from `_shared/08_cache_sim.hpp` directly (its interface is almost the same as the one you wrote in 1101: `access`, `access_range`, `accesses()`, `misses()`), so this exercise focuses on the "levels" and the "curve".

Instead of running Cachegrind on a real program, you build the multilevel model yourself and compute the miss counts for every cache size in a single pass over a trace.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Two (or more) cache levels and multi-byte accesses

```cpp
struct LevelConfig {
    std::size_t capacity_bytes;
    std::size_t line_bytes;
    std::size_t ways;
};

class CacheHierarchy {
public:
    explicit CacheHierarchy(std::vector<LevelConfig> levels);

    std::size_t access(std::uint64_t address);
    std::size_t access_range(std::uint64_t address, std::size_t bytes);
    std::uint64_t accesses(std::size_t level) const;
    std::uint64_t misses(std::size_t level) const;
};
```

`CacheHierarchy(levels)`: `levels[0]` is closest to the CPU, and each level is a `LevelConfig{capacity_bytes, line_bytes, ways}`. If `levels` is empty, or the levels' `line_bytes` differ, throw `std::invalid_argument` (invalid parameters for a single level make `Cache` throw).

- `access(address)`: look it up starting at level 0, **going down a level only on a miss**. Return the index of the level that satisfied the access; if every level misses (the access goes to memory), return `levels.size()`. Every level that missed must load the line.
- `access_range(address, bytes)`: do one `access` for each line that `[address, address + bytes)` covers, and return how many of those lines went to memory. `bytes == 0` does nothing. An 8-byte access that crosses a line boundary touches two lines, and nothing may overflow when `address + bytes - 1` is exactly the top of the address space.
- `accesses(level)`, `misses(level)`: the number of accesses and misses each level sees. A lower level sees only the misses of the level above it.

## Stage 2 — Miss-Ratio Curve

```cpp
inline std::vector<std::uint64_t> miss_counts(std::span<const std::uint64_t> addresses,
                                              std::size_t line_bytes,
                                              std::span<const std::size_t> capacities);
```

`miss_counts(addresses, line_bytes, capacities)`: for a fully associative LRU cache, return an array of the same length as `capacities` whose i-th entry is the miss count with a capacity of `capacities[i]` lines. `capacities` may be in any order, and a capacity of 0 means every access misses.

Method: for each access, compute its **stack distance**: how many **distinct** lines have been touched since this line was last touched (infinite for a first occurrence). An LRU cache of capacity `C` misses exactly when the distance is `≥ C`.

The total time must be **O(n log n)** (n is the number of accesses); you must not simulate each capacity separately. Hint: put a 1 at "the position where each line last occurred"; the range sum between two occurrences is then the distance, and a Fenwick tree does exactly that.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Stack distance predicts a fully associative LRU cache exactly, but only approximates a set-associative one. Why?
2. Real L2 caches are usually inclusive or exclusive. Which kind is this exercise's model? Which counter would show the difference?
3. Why can a program compiled with `-O2` execute fewer instructions yet show a higher miss rate in cachegrind? What are the pros and cons of using instruction counts as a cost model?
