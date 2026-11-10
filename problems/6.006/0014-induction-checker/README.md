---
title: Induction as Code (sum of cubes and polynomial verification)
unit: 00 · Warm-up (PS0)
source: 6.006 PS0 Problem 0-4
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 55
---

# 0014 · Induction as Code

**Original problem:** [Problem Set 0 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/60f0a029ab154553cad51b54cc25605b_MIT6_006S20_ps0-questions.pdf)

> **Why this comes up in practice:** passing tests for n = 1…1000 doesn't mean the code is right for every n. Formal verification, loop invariants, and even deriving the closed form behind a report all rest on the same structure: a base case plus an inductive step.

## Original problem

6.006 PS0 Problem 0-4 asks for a written proof by induction: show that for every integer n ≥ 1, the sum of the first n cubes, 1³ + 2³ + … + n³ (that is, Σ i³ for i from 1 to n), equals [n(n+1)/2]². There is only this one part, and there is no code to write.

## Your task

The original problem asks you to prove `1³ + 2³ + … + n³ = (n(n+1)/2)²` by induction (see the Original problem section above). This exercise puts that result to use, then turns induction itself into a check you can run. **The handwritten proof is in the interview follow-ups.**

You compute the sum with the closed form, build a small polynomial type with integer coefficients, and use it to check both the base case and the inductive step of a sum formula for every n at once.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Use the closed form

```cpp
std::optional<std::uint64_t> checked_sum_of_cubes(std::uint64_t n);
std::uint64_t sum_of_cubes_mod(std::uint64_t n, std::uint64_t m);
```

- `checked_sum_of_cubes(n)`: return `1³ + … + n³`, or `std::nullopt` when it doesn't fit in `uint64_t`. `n` can be very large.
- `sum_of_cubes_mod(n, m)`: return `(1³ + … + n³) mod m`. `n` can be up to 4 × 10¹⁸, `m ≥ 1`, and every call must be **O(1)**.

Note that in `n(n+1)/2` you must divide by 2 before taking the mod, and you also have to watch out for overflow.

## Stage 2 — Polynomials with integer coefficients

```cpp
class Poly {
public:
    Poly() = default;
    explicit Poly(std::vector<std::int64_t> coefficients);

    const std::vector<std::int64_t>& coefficients() const;
    int degree() const;
    std::int64_t operator()(std::int64_t x) const;
    Poly shifted() const;

    friend Poly operator+(const Poly& a, const Poly& b);
    friend Poly operator-(const Poly& a, const Poly& b);
    friend Poly operator*(const Poly& a, const Poly& b);
    friend bool operator==(const Poly& a, const Poly& b);
};
```

`Poly`: `coefficients()[i]` is the coefficient of `xⁱ`. The leading coefficient is never 0 (the zero polynomial is an empty `coefficients()`, and its `degree()` is −1):

| Operation | Behavior |
| --- | --- |
| `Poly(coeffs)` | Construct, dropping trailing 0s |
| `p(x)` | Evaluate, using Horner's rule |
| `+`, `-`, `*`, `==` | Polynomial addition, subtraction, multiplication, and comparison |
| `shifted()` | Return the polynomial `q(x) = p(x + 1)` |

Every coefficient and every evaluation result is guaranteed to fit in `int64_t`.

## Stage 3 — An induction checker

```cpp
struct InductionReport {
    bool base_case = false;
    bool inductive_step = false;
    bool proved() const;  // provided
};

InductionReport check_sum_formula(const Poly& term, const Poly& numerator,
                                  std::int64_t denominator);
```

`check_sum_formula(term, numerator, denominator)` checks this claim:

> For every integer `n ≥ 1`: `term(1) + term(2) + … + term(n) == numerator(n) / denominator`

It returns an `InductionReport`:

- `base_case`: whether the claim holds for `n = 1`.
- `inductive_step`: whether, for **every** `k`, the claim holding at `n = k` implies that it holds at `n = k + 1`.
- `proved()`: both hold.

**Do not just try a few values of n.** The inductive step must be a check of a polynomial identity (whether `numerator(x+1) − numerator(x)` and `denominator · term(x+1)` are the same polynomial), so that the conclusion holds for every n. Throw `std::invalid_argument` when `denominator == 0`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Walk through the complete inductive proof of `Σ i³ = (n(n+1)/2)²` out loud: what is the base case? How does the inductive step break the `k+1` case back down into the `k` case?
2. If `inductive_step` is true but `base_case` is false, what does that say about the formula? Why is "the inductive step alone is enough" wrong?
3. At how many points must two polynomials of degree ≤ d agree before they are guaranteed to be identical? How does this relate to randomized testing (polynomial identity testing)?

---

**Official solution:** [Problem Set 0 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cfc7d2d5bc3b9f4df35b5bdc7aa49284_MIT6_006S20_ps0-solutions.pdf). Read it after you finish.
