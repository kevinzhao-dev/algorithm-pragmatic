---
title: Modular Arithmetic (floor mod, modular exponentiation, and schedule collisions)
unit: 00 · Warm-up (PS0)
source: 6.006 PS0 Problem 0-3
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 50
---

# 0013 · Modular Arithmetic

**Original problem:** [Problem Set 0 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/60f0a029ab154553cad51b54cc25605b_MIT6_006S20_ps0-questions.pdf)

> **Why this comes up in practice:** ring-buffer indices, hash sharding, and bucketing by time window all use mod, and C++'s `%` hands you a negative remainder when it meets a negative number. Hashing (Rabin-Karp, for example) needs modular exponentiation, and working out when two periodic jobs collide is a congruence equation.

## Original problem

6.006 PS0 Problem 0-3 is a pencil-and-paper warm-up on congruences. It sets A = 600/6 and B = 60 mod 42, and asks you to decide whether each of these three statements is true or false:

- (a) A ≡ B (mod 2).
- (b) A ≡ B (mod 3).
- (c) A ≡ B (mod 4).

## Your task

The original problem asks you to decide whether a few congruences are true or false (the numbers are in the Original problem section above). This exercise turns mod into a component you can trust.

You build a floor mod and a floor division that negative numbers can't fool, and overflow-free modular multiplication and exponentiation; then you find the first time two periodic jobs run together, which the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Don't get fooled by negative numbers

```cpp
std::int64_t floor_mod(std::int64_t a, std::int64_t m);
std::int64_t floor_div(std::int64_t a, std::int64_t m);
bool congruent(std::int64_t a, std::int64_t b, std::int64_t m);
```

The modulus `m` must be > 0; otherwise throw `std::invalid_argument`:

- `floor_mod(a, m)`: return the value in `[0, m)` that is congruent to `a`.
- `floor_div(a, m)`: return `⌊a / m⌋` (rounded toward negative infinity). It must satisfy `a == m * floor_div(a, m) + floor_mod(a, m)`.
- `congruent(a, b, m)`: whether `a ≡ b (mod m)` holds. Be careful: computing `a - b` directly can overflow.

## Stage 2 — Modular exponentiation

```cpp
std::uint64_t mul_mod(std::uint64_t a, std::uint64_t b, std::uint64_t m);
std::uint64_t pow_mod(std::uint64_t base, std::uint64_t exponent, std::uint64_t m);
```

- `mul_mod(a, b, m)`: `(a · b) mod m`. `a, b, m` can all be close to 2⁶⁴ (use a 128-bit intermediate value), and `m ≥ 1`.
- `pow_mod(base, exponent, m)`: `base^exponent mod m`. `exponent` can be 2⁶³ or more, and you must use **O(log exponent)** multiplications. `m ≥ 1`, and `0^0` counts as 1.

## Stage 3 — Real-world extension: when two periodic jobs collide

```cpp
struct PeriodicJob {
    std::int64_t phase;
    std::int64_t period;
};

std::optional<std::int64_t> first_coincidence(PeriodicJob a, PeriodicJob b);
```

In a data center, two periodic batch jobs often need to stay out of each other's way.

`first_coincidence(a, b)`: `a` and `b` are `PeriodicJob{phase, period}` values, meaning the job runs at times `phase + k·period` (`k ≥ 0`), with `phase ≥ 0` and `period ≥ 1`. Return the first time at which both run together; if they never collide, return `std::nullopt`.

- The periods are not necessarily coprime.
- Periods can be up to 10⁹ and phases up to 10¹², so enumerating the run times one by one is too slow (a `PERF` test checks this). This stage calls for extended Euclid (Bézout coefficients).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why do ring buffers often require a capacity that is a power of 2? How do `%` and `&` differ, and how does each behave on negative numbers?
2. When does `first_coincidence` have no solution? Explain why using the gcd.
3. What can Fermat's little theorem be used for (modular inverses, Miller-Rabin)? What do you do when the modulus isn't prime?

---

**Official solution:** [Problem Set 0 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cfc7d2d5bc3b9f4df35b5bdc7aa49284_MIT6_006S20_ps0-solutions.pdf). Read it after you finish.
