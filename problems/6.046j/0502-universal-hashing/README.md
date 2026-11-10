---
title: Universal Hashing (defending against hash flooding)
unit: 05 · Skip Lists, Hashing, and DP (PS5)
source: 6.046J Lecture 8
pdf: materials/6.046j/lectures/lec08.pdf
read: materials/6.046j/lectures/lec08.pdf
kind: code
minutes: 100
---

# 0502 · Universal Hashing

**Based on:** [Randomization: Universal & Perfect Hashing](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/5e4c05ec33ad4e733c1e6a247f21d1de_MIT6_046JS15_lec08.pdf) · **Read first:** [Randomization: Universal & Perfect Hashing](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/5e4c05ec33ad4e733c1e6a247f21d1de_MIT6_046JS15_lec08.pdf)

> **Why this comes up in practice:** once an attacker knows a hash table's hash function, they can craft keys that all land in the same slot and knock the service back to O(n²). This is hash flooding, and it is why Python and Rust randomize their hashing by default.

## Your task

You build the two universal hash families from Lecture 8, then a chained hash table that draws a new random function every time it grows, and finally a rate limiter whose keys an attacker controls.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — The dot-product family

```cpp
class DotProductHash {
public:
    DotProductHash(std::uint64_t slots, std::mt19937_64& rng);

    std::uint64_t slots() const;
    std::uint64_t operator()(std::uint64_t key) const;
};
```

`DotProductHash(slots, rng)`: `slots` is a prime `< 2^31` (otherwise throw `std::invalid_argument`). Treat the key as a number written in base `slots`, draw a random vector with the same number of digits, and let `h(k) = a·k mod slots`.

- Cover all 64 bits: the key's most significant digit must take part in the computation too.
- The tests measure the collision rate directly: for a fixed pair of keys, they draw the function independently many times, and the collision probability must not noticeably exceed `1/slots`.

## Stage 2 — A practical family

```cpp
inline constexpr std::uint64_t kHashPrime = (std::uint64_t{1} << 61) - 1;  // p, provided

class LinearHash {
public:
    LinearHash(std::uint64_t slots, std::mt19937_64& rng);

    std::uint64_t slots() const;
    std::uint64_t operator()(std::uint64_t key) const;
};
```

`LinearHash(slots, rng)`: `h(k) = ((a·k + b) mod p) mod slots`, with `p = 2^61 − 1`, `a ∈ [1, p)`, and `b ∈ [0, p)`. `slots ≥ 1` (otherwise throw `std::invalid_argument`). Keys are guaranteed to be `< p`.

## Stage 3 — The hash table

```cpp
template <typename V>
class UniversalHashMap {
public:
    explicit UniversalHashMap(std::uint64_t seed = 6046);

    std::size_t size() const;
    bool empty() const;
    std::size_t bucket_count() const;
    std::size_t longest_chain() const;

    bool insert(std::uint64_t key, V value);
    V* find(std::uint64_t key);
    const V* find(std::uint64_t key) const;
    bool erase(std::uint64_t key);
};
```

`UniversalHashMap<V>`: resolve collisions by chaining (one `std::vector` per slot). Every operation throws `std::invalid_argument` when `key ≥ p`.

| Operation | Behavior |
| --- | --- |
| `insert(key, value)` | Return `true` for a new key; if the key already exists, overwrite its value and return `false` |
| `find(key)` | Return a pointer to the value, or `nullptr` if the key is absent |
| `erase(key)` | Return whether a key was actually deleted |
| `size()`, `empty()`, `bucket_count()` | |
| `longest_chain()` | The length of the longest slot (for diagnostics; O(m) is fine) |

**Keep the load factor ≤ 1**: when the table is full, double the number of buckets and rehash with a **new random function**. Without this step, once an attacker has worked out the structure of the function, the next rebuild is no safer. The tests flood the table with keys that all differ by multiples of `2^40` and check that the longest chain is `≤ 3 lg n`.

## Stage 4 — Real-world extension: an attack-resistant rate limiter

```cpp
class RateLimiter {
public:
    RateLimiter(std::uint64_t limit, std::int64_t window, std::uint64_t seed = 6046);

    bool allow(std::uint64_t client, std::int64_t now);
    std::size_t client_count() const;
    std::size_t longest_chain() const;
};
```

`RateLimiter(limit, window, seed)`: a fixed-window counter. Both `limit` and `window` must be positive; otherwise throw `std::invalid_argument`.

- `allow(client, now)`: time is cut into windows of length `window` (`floor(now / window)`, which must also be right for negative times). Within one window, each client is let through at most `limit` times; beyond that, return `false`.
- `client_count()`: how many clients have been seen. `longest_chain()` is as in Stage 3.

Client IDs are chosen from outside, so they are input that an attacker can control.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What is the definition of universal? Why do neither `h(k) = k mod m` nor `h(k) = (a·k) mod m` qualify?
2. Why does the dot-product hash function require `m` to be prime? Why doesn't `LinearHash` need a prime `m`, yet needs an extra `b`?
3. The rate limiter's map keeps growing; how would you handle that in a real system? What are the trade-offs between a fixed window and a sliding log or GCRA?
