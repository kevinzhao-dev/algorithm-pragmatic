---
title: Perfect Hashing (two-level FKS hashing)
unit: 05 · Skip Lists, Hashing, and DP (PS5)
source: 6.046J Lecture 8
pdf: materials/6.046j/lectures/lec08.pdf
read: materials/6.046j/lectures/lec08.pdf
kind: code
minutes: 100
---

# 0503 · Perfect Hashing

**Based on:** [Randomization: Universal & Perfect Hashing](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/5e4c05ec33ad4e733c1e6a247f21d1de_MIT6_046JS15_lec08.pdf) · **Read first:** [Randomization: Universal & Perfect Hashing](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/5e4c05ec33ad4e733c1e6a247f21d1de_MIT6_046JS15_lec08.pdf)

> **Why this comes up in practice:** routing tables, language keyword tables, and read-only feature flags never change once they are built, but they are queried enormously often. Rather than settle for O(1) on average, spend a little more time building the table and get a worst case that touches only two memory slots.

## Your task

Using the idea behind `LinearHash` from [`0502`](../0502-universal-hashing/README.md) (you may copy the code over), build a static dictionary, `PerfectHashMap<V>`.

You build the two-level FKS scheme from Lecture 8, first level and then second level, and then wrap it into a dictionary with string keys.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Building the table

```cpp
inline constexpr std::uint64_t kHashPrime = (std::uint64_t{1} << 61) - 1;  // provided

template <typename V>
class PerfectHashMap {
public:
    using Item = std::pair<std::uint64_t, V>;

    explicit PerfectHashMap(std::vector<Item> items, std::uint64_t seed = 6046);

    std::size_t size() const;
    std::size_t total_slots() const;
};
```

`PerfectHashMap(items, seed)`: `items` are `(key, value)` pairs with key `< kHashPrime`. Throw `std::invalid_argument` if a key is duplicated or out of range. **If you don't reject duplicate keys up front, they collide under every hash function you could pick, and the program never finishes.**

The first level hashes the `n` keys into `n` buckets, with `l_j` keys in bucket `j`. Keep drawing a new first-level function until `Σ l_j² ≤ 4n`.

`total_slots()`: return the total number of second-level slots, that is, `Σ l_j²`, which must be `≤ 4n`. `seed` determines every random choice, so the same input and seed must give the same result.

## Stage 2 — The second level and lookups

```cpp
// in class PerfectHashMap<V>
const V* find(std::uint64_t key) const;
bool contains(std::uint64_t key) const;

// Cost model used by the tests: the number of table slots examined so far.
std::uint64_t probes() const;
```

Give bucket `j` a table of `l_j²` slots and pick a second-level function for it, drawing again until the bucket has **no collisions at all**.

- `find(key)`: return a pointer to the value, or `nullptr` if the key is absent; `contains` works the same way. Any key outside the set the table was built from (including keys `≥ kHashPrime`) must be correctly reported as absent, so the slots must store the keys for comparison.
- `probes()`: the total number of slots examined so far. **Each lookup may examine at most 2** (one slot in the first level and one in the second), with no chaining. The tests check every single lookup.

## Stage 3 — Real-world extension: a string dictionary

```cpp
template <typename V>
class StaticStringMap {
public:
    using Item = std::pair<std::string, V>;

    explicit StaticStringMap(std::vector<Item> items, std::uint64_t seed = 6046);

    std::size_t size() const;
    const V* find(std::string_view key) const;
};
```

`StaticStringMap<V>(items, seed)`: the keys are strings; throw `std::invalid_argument` on duplicates.

Turn each string into a 61-bit fingerprint with a polynomial hash whose base is random, then hand the fingerprints to `PerfectHashMap`. These points are easy to overlook:

- Fingerprints can collide: if you find, while building the table, that two different strings have the same fingerprint, switch to another base and start over.
- `find(key)` must compare the full string; otherwise a string that isn't in the dictionary might hit a matching fingerprint and be wrongly reported as present.
- Strings of NUL characters with different lengths must also be told apart (think about the fingerprint's starting value and how bytes are encoded).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the expectation of `Σ l_j²` at the first level O(n)? Work it out once with indicator variables and the definition of universal. Why must the second level use `l_j²` slots rather than `l_j`?
2. What is the expected time to build the table? Why can the number of random retries be estimated with a geometric distribution?
3. If keys occasionally need to be updated, what is the idea behind dynamic perfect hashing (Dietzfelbinger)? How does this structure trade off against cuckoo hashing?
