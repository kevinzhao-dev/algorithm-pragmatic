---
title: Attacking Hash Families (who can guarantee a collision)
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 PS3 Problem 3-2
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf, materials/6.006/recitations/rec04.pdf
kind: code
minutes: 50
---

# 0312 · Attacking Hash Families

**Original problem:** [Problem Set 3 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/03a34fcf3012a242ffcae5534e9fa511_MIT6_006S20_ps3-questions.pdf) · **Read first:** [Lecture 4: Hashing notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ce9e94705b914598ce78a00a70a1f734_MIT6_006S20_lec4.pdf), [Recitation 4 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bd220fb629aefdf30f416b5abf45d38d_MIT6_006S20_r04.pdf)

> **Why this comes up in practice:** if a distributed system's routing hash is public and users decide the keys (a self-chosen session id, a URL), an attacker can pour all the traffic onto a single machine. In this exercise you are the attacker: for each family, find a way to pick a batch of keys that are certain to collide **before the hash function is drawn**.

## Original problem

6.006 PS3 Problem 3-2 is about picking keys before the hash function is chosen. A university houses 2n incoming students in n rooms, labeled 0 through n − 1. Every student's ID is distinct and is a positive integer below u (with u ≫ 2n), and new students get to pick their own IDs. The university announces a hash family H before anyone picks an ID; once the IDs are fixed, it draws one function from H uniformly at random and sends each student to the room their ID hashes to. Two freshmen want to room together. For each family below, either give IDs k1 and k2 they can pick that guarantee they become roommates, or prove that no such pair exists and work out the best probability of sharing a room that they can reach.

- (a) H = { h_ab(k) = (ak + b) mod n | a, b ∈ {0, …, n − 1} and a ≠ 0 }.
- (b) H = { h_a(k) = (⌊kn/u⌋ + a) mod n | a ∈ {0, …, u − 1} }.
- (c) H = { h_ab(k) = ((ak + b) mod p) mod n | a, b ∈ {0, …, p − 1} and a ≠ 0 } for a fixed prime p > u (Lecture 4's universal family).

## Your task

A service draws a random hash function from a family to map keys into `n` buckets, but **the attacker picks the keys first**. Keys are positive integers in `[1, u)`, and `n ≥ 2` in every function. The original is a proof problem; here you write the attack as code, and your answers are verified by enumerating every function in the family. Stages 1–3 cover parts (a)–(c), and Stage 4 goes beyond the original by generating whole batches of colliding keys.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Linear family

```cpp
using KeyPair = std::pair<std::uint64_t, std::uint64_t>;

inline std::optional<KeyPair> break_linear_family(std::uint64_t n, std::uint64_t u);
```

`h_ab(k) = (a·k + b) mod n`, where `a, b ∈ {0, …, n-1}` and `a ≠ 0`.

`break_linear_family(n, u)`: return two distinct keys that collide under **every** function in the family. If no such pair of keys exists in `[1, u)`, return `std::nullopt`. `n` and `u` can be huge (the tests use `n = 2^30` and `u = 2^62`), so don't search for the pair.

## Stage 2 — Bucket family

```cpp
inline std::optional<KeyPair> break_bucket_family(std::uint64_t n, std::uint64_t u);
```

`h_a(k) = (⌊k·n/u⌋ + a) mod n`, where `a ∈ {0, …, u-1}`.

`break_bucket_family(n, u)`: same contract as above. `n` goes up to `2^31` and `u` up to `2^62`, so you must not scan `[1, u)` (and you must not let `k·n` overflow).

## Stage 3 — The limits of the universal family

```cpp
struct Fraction {
    std::uint64_t num = 0;
    std::uint64_t den = 1;
    friend bool operator==(const Fraction&, const Fraction&) = default;
};

inline Fraction universal_collision_probability(std::uint64_t p, std::uint64_t n);
```

`h_ab(k) = ((a·k + b) mod p) mod n`, where `p` is prime (`2 ≤ n ≤ p < 2^31`), `a ∈ {1, …, p-1}`, and `b ∈ {0, …, p-1}`.

Here you can't guarantee a collision; you can only compute its probability. `universal_collision_probability(p, n)` returns the **exact probability** that two distinct keys (`< p`) collide, as a fraction in lowest terms, `Fraction{num, den}` (in lowest terms, `0` is `{0, 1}`).

Hint: for two fixed distinct keys, which pairs does `(a·k1 + b mod p, a·k2 + b mod p)` run through as `(a, b)` ranges over the family? Does the probability depend on the keys? Write it in closed form using `p = q·n + t`; don't enumerate.

## Stage 4 — Real-world extension: flood key generator

```cpp
inline std::vector<std::uint64_t> flood_linear(std::uint64_t n, std::uint64_t u, std::size_t count);
inline std::vector<std::uint64_t> flood_bucket(std::uint64_t n, std::uint64_t u, std::size_t count);
```

- `flood_linear(n, u, count)`: return `count` distinct keys in `[1, u)` that land in the same bucket under **every** function of the linear family.
- `flood_bucket(n, u, count)`: the same, for the bucket family.

If no such `count` keys exist, return an empty vector (also return an empty one when `count == 0`). As in Stage 2, `u` can be huge (up to `2^50` in the tests), so don't enumerate `[1, u)`. This is real flood-attack data. The control group is in the tests: the same keys spread out under the universal family.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the Stage 3 probability the same for every pair of keys? Which step of the definition of "universal" does that correspond to?
2. Real systems hash with SipHash plus a random seed rather than `(ak+b) mod p`. What's the difference?
3. If the attacker can **observe** where their own keys land (for example, by inferring it from response latency), does the guarantee of universal hashing still hold?

---

**Official solution:** [Problem Set 3 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9aede1539e086a709a75d604a89f08b6_MIT6_006S20_ps3-solutions.pdf). Read it after you finish.
