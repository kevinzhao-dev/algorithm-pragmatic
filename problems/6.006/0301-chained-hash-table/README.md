---
title: Chained Hash Table (universal hashing)
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 Lecture 4, Recitation 4
pdf: materials/6.006/recitations/rec04.pdf
read: materials/6.006/lectures/lec04.pdf
kind: code
minutes: 75
---

# 0301 · Chained Hash Table

**Based on:** [Recitation 4 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bd220fb629aefdf30f416b5abf45d38d_MIT6_006S20_r04.pdf) · **Read first:** [Lecture 4: Hashing notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ce9e94705b914598ce78a00a70a1f734_MIT6_006S20_lec4.pdf)

> **Why this comes up in practice:** Python's dict, C++'s `unordered_map`, and a database's hash join are all this structure at heart. In the 2011 HashDoS attacks, attackers picked a batch of keys that flooded the chains into long linked lists and knocked over the default implementations of a whole list of languages; a randomized hash family is exactly the countermeasure.

## Your task

Implement a universal hash and a chaining hash table yourself. Do not use `std::unordered_map` (a chain may be a `std::forward_list` or your own linked list).

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Universal hash

```cpp
class UniversalHash {
public:
    static constexpr std::uint64_t kPrime = (std::uint64_t{1} << 61) - 1;

    UniversalHash(std::uint64_t a, std::uint64_t b);
    std::size_t operator()(std::uint64_t key, std::size_t buckets) const;
};
```

`UniversalHash(a, b)` represents one member of the family, `h(k) = ((a*k + b) mod p) mod m`, where `p = kPrime = 2^61 - 1`.

- The constructor checks `1 ≤ a < p` and `0 ≤ b < p`, and throws `std::invalid_argument` otherwise.
- `operator()(key, buckets)`: `buckets ≥ 1`, the key can be any `uint64_t`, and the return value must be `< buckets`.

`a*k` overflows 64 bits, so use `unsigned __int128`. There is a fast way to reduce modulo a Mersenne prime without `%`, and it's worth thinking about.

## Stage 2 — Basic operations

```cpp
template <typename K, typename V, typename Prehash = std::hash<K>>
class ChainedHashMap {
public:
    explicit ChainedHashMap(std::uint64_t seed = 0x9E3779B97F4A7C15ULL, Prehash prehash = Prehash{});

    ChainedHashMap(const ChainedHashMap&) = delete;
    ChainedHashMap& operator=(const ChainedHashMap&) = delete;

    bool insert(K key, V value);
    V* find(const K& key);
    const V* find(const K& key) const;
    std::optional<V> erase(const K& key);
    std::size_t size() const;
    bool empty() const;
};
```

`ChainedHashMap<K, V, Prehash>` (`Prehash` turns a key into a `uint64_t`, and defaults to `std::hash<K>`):

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `explicit ChainedHashMap(seed, prehash)` | `seed` determines the hash parameters | O(1) |
| `insert(k, v)` | Return `true` for a new key; if the key already exists, **overwrite** its value and return `false` | expected O(1) |
| `find(k)` | A pointer to the value, or `nullptr` if the key is absent | expected O(1) |
| `erase(k)` | Return the removed value, or `std::nullopt` if the key is absent | expected O(1) |
| `size()` / `empty()` | | O(1) |

No leaks: the destructor must release every key and value.

## Stage 3 — Dynamic resizing

```cpp
// in class ChainedHashMap<K, V, Prehash>
std::size_t bucket_count() const;
double load_factor() const;
```

- `bucket_count()`: the current number of buckets. `load_factor()`: `size / bucket_count`.
- At all times, both `size() ≤ bucket_count()` and `bucket_count() ≤ 8 × size() + 64` must hold (the table must shrink as well as grow).
- **The grow and shrink thresholds must not be the same:** when the number of keys hovers around a threshold, you must not rehash on every operation.

`insert` / `erase` must be amortized expected O(1).

## Stage 4 — Adversarial input and ownership

```cpp
// in class ChainedHashMap<K, V, Prehash>
ChainedHashMap(ChainedHashMap&& other);             // should be noexcept once implemented
ChainedHashMap& operator=(ChainedHashMap&& other);  // should be noexcept once implemented

std::size_t bucket_of(const K& key) const;
std::size_t max_chain_length() const;
```

- `bucket_of(k)`: the bucket the key currently falls in, `< bucket_count()`.
- `max_chain_length()`: the length of the longest chain right now.
- Different seeds must produce different distributions. If you insert keys that differ only in their high bits (for example, all multiples of `2^20`), the longest chain must stay short (the tests insert 20,000 multiples of `2^20` through an identity prehash and allow a longest chain of at most 32).
- If `Prehash` maps every key to the same value, the table must still work correctly (just more slowly).
- Move constructor / move assignment: these must be `noexcept`. The moved-from table becomes empty and can still take inserts afterwards.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. In "expected O(1)", what is the expectation taken over? The input? Does it still hold when an attacker knows which family you use?
2. Why must `erase` shrink the table too? What happens if the shrink threshold is `n < m/2`?
3. `prehash` squeezes a string into 64 bits before it reaches the universal hash. Why doesn't this count as breaking the universal guarantee? (Or what does it actually break?)
