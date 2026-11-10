---
title: Hash Practice (hand calculations as code)
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 PS3 Problem 3-1
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf, materials/6.006/recitations/rec04.pdf
kind: code
minutes: 30
---

# 0311 · Hash Practice

**Original problem:** [Problem Set 3 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/03a34fcf3012a242ffcae5534e9fa511_MIT6_006S20_ps3-questions.pdf) · **Read first:** [Lecture 4: Hashing notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ce9e94705b914598ce78a00a70a1f734_MIT6_006S20_lec4.pdf), [Recitation 4 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bd220fb629aefdf30f416b5abf45d38d_MIT6_006S20_r04.pdf)

> **Why this comes up in practice:** `gperf` and compilers' keyword tables search a small, fixed set of keys for a hash function with no collisions at all, so that every lookup takes a single probe.

## Original problem

6.006 PS3 Problem 3-1 is two small hand calculations.

- (a) Start from an empty 7-slot table with h(k) = (10k + 4) mod 7 and add the keys A = [47, 61, 36, 52, 56, 33, 92] one at a time, in the given order. Each slot keeps a linked list (chaining) and a new key joins the end of its slot's list; draw the final table.
- (b) Replace the hash function with h(k) = ((10k + 4) mod c) mod 7, where c is a positive integer. What is the least c that sends all seven keys of A to distinct slots?

## Your task

The original problem is two hand calculations; here you turn the hand-calculation steps into code. Once it works, you should run it on the numbers from the Original problem section and check the results against Problem 3-1 in `answer`, the official solution linked at the end. Stage 3 goes beyond the original and uses the same search to build a perfect hash set.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Draw the chaining result

```cpp
template <typename HashFn>
std::vector<std::vector<std::int64_t>> chain_layout(std::span<const std::int64_t> keys,
                                                    std::size_t buckets, HashFn hash);
```

`chain_layout(keys, buckets, hash)`: insert `keys` in order into `buckets` chains using `hash(k)`, and return the contents of every chain.

- Within a bucket, **later insertions come later in the chain** (the same rule as in the original problem).
- The returned vector always has length `buckets`, and a bucket with no keys is an empty vector. `hash(k)` is guaranteed to return a value in `[0, buckets)`.

## Stage 2 — Find a collision-free modulus

```cpp
inline std::optional<std::int64_t> smallest_injective_modulus(std::span<const std::int64_t> keys,
                                                              std::int64_t a, std::int64_t b,
                                                              std::int64_t buckets,
                                                              std::int64_t limit);
```

`smallest_injective_modulus(keys, a, b, buckets, limit)`: find the smallest positive integer `c` in `1..=limit` such that the function `h(k) = ((a·k + b) mod c) mod buckets` has no collisions on `keys` (it is injective). If there is none, return `std::nullopt`.

- The `keys` are pairwise distinct.
- When there are more `keys` than `buckets`, the answer is always `nullopt` (the pigeonhole principle), and `limit` may be too large to scan.
- Every key is non-negative, and `a·k + b` does not overflow `int64_t`.

## Stage 3 — Real-world extension: Perfect Hash

```cpp
class PerfectHashSet {
public:
    explicit PerfectHashSet(std::span<const std::int64_t> keys);

    bool contains(std::int64_t key) const;
    std::size_t table_size() const;
    std::size_t slot_of(std::int64_t key) const;
};
```

`PerfectHashSet(keys)`: given a fixed set of keys (`keys.size() ≤ 64`, pairwise distinct, non-negative), build a table whose lookups need no probing.

- `table_size()`: the number of slots, at most `4 × keys.size() + 1`.
- `slot_of(k)`: the slot for `k`, `< table_size()`. The stored keys all get pairwise distinct slots.
- `contains(k)`: answer by looking at a single slot, and answer correctly for keys that were never stored too.

To build it, you can search with Stage 2: `buckets = 4n + 1`, fix `b = 0`, and try one `a` after another until you find a collision-free `(a, c)`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does the search for the smallest c in Stage 2 have to start at 1? When `c < buckets`, at most how many distinct hash values are there?
2. Roughly what is the probability that a random function puts n keys into m slots with no collision? Why does it take thousands of tries at `m = 4n` when n is only a few dozen?
3. How does a perfect hash answer queries for keys that aren't stored? Why must each slot store the key itself?

---

**Official solution:** [Problem Set 3 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9aede1539e086a709a75d604a89f08b6_MIT6_006S20_ps3-solutions.pdf). Read it after you finish.
