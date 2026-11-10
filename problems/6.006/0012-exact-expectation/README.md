---
title: Exact Expected Values (Fraction and discrete distributions)
unit: 00 · Warm-up (PS0)
source: 6.006 PS0 Problem 0-2
pdf: materials/6.006/psets/ps0/questions.pdf
answer: materials/6.006/psets/ps0/solutions.pdf
kind: code
minutes: 60
---

# 0012 · Exact Expected Values

**Original problem:** [Problem Set 0 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/60f0a029ab154553cad51b54cc25605b_MIT6_006S20_ps0-questions.pdf)

> **Why this comes up in practice:** "the probability that at least two of three replicas are up" and "the average cost of a retry policy" are both expected values of discrete distributions. Add up thirty-six copies of 1/36 in a `double` and the result may not be the exact answer, and some systems (accounting, quota calculations) can't tolerate any error at all.

## Original problem

6.006 PS0 Problem 0-2 is a pencil-and-paper warm-up on expectation. It defines two random variables:

- X counts how many heads come up in three tosses of a fair coin.
- Y is the product of the two numbers shown when you roll two fair six-sided dice.

It then asks for three expected values:

- (a) E[X], the expected number of heads.
- (b) E[Y], the expected product of the dice.
- (c) E[X + Y], the expected value of the sum of the two variables.

## Your task

The original problem asks you to compute expected values for coin flips and for the product of two dice (the setup is in the Original problem section above). This exercise turns "discrete distribution" into a component you can actually use: probabilities are exact fractions, and the whole computation has no floating-point error at all.

You build an exact `Fraction` type, a distribution type `Pmf<T>` on top of it, and joint distributions, and you finish with a quorum-availability calculation that the original problem doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Fraction

```cpp
class Fraction {
public:
    Fraction();
    Fraction(std::int64_t numerator, std::int64_t denominator = 1);

    std::int64_t numerator() const;
    std::int64_t denominator() const;

    friend Fraction operator+(const Fraction& a, const Fraction& b);
    friend Fraction operator-(const Fraction& a, const Fraction& b);
    friend Fraction operator*(const Fraction& a, const Fraction& b);
    friend Fraction operator/(const Fraction& a, const Fraction& b);

    friend bool operator==(const Fraction& a, const Fraction& b);
    friend std::strong_ordering operator<=>(const Fraction& a, const Fraction& b);

    friend std::ostream& operator<<(std::ostream& os, const Fraction& f);  // provided
};
```

A `Fraction` is always in lowest terms with a positive denominator, and 0 is written as `0/1`:

- `Fraction()` is 0. `Fraction(p, q)` throws `std::invalid_argument` when the denominator is 0.
- `numerator()`, `denominator()`: return the numerator and denominator after reduction.
- `+ - * /`: return the exact result. Division by zero throws `std::domain_error`. When the result doesn't fit in `int64_t`, throw `std::overflow_error`.
- `==`, `<=>`: exact comparison. When numerators and denominators are close to 2⁶³, cross-multiplying must not overflow (use `__int128`).

Intermediate values may exceed 64 bits. As long as the **reduced result** fits, you must compute it, for example `(2^62 / 3) · (3 / 2^61) = 2`.

## Stage 2 — The discrete distribution Pmf

```cpp
template <typename T>
class Pmf {
public:
    explicit Pmf(const std::vector<std::pair<T, Fraction>>& outcomes);
    static Pmf uniform(const std::vector<T>& outcomes);

    std::vector<T> support() const;
    Fraction prob(const T& outcome) const;

    Fraction expected() const
        requires std::convertible_to<T, Fraction>;

    template <typename F>
    Fraction expected_of(F f) const;

    template <typename F>
    Pmf<std::decay_t<std::invoke_result_t<F&, const T&>>> map(F f) const;
};
```

`Pmf<T>` (probability mass function) maps each outcome to its probability. `T` is only guaranteed to be comparable.

| Operation | Behavior |
| --- | --- |
| `Pmf(pairs)` | Build from `(outcome, probability)` pairs. The probabilities of equal outcomes add up, and an outcome with probability 0 is not in the support. Probabilities must not be negative and must sum to exactly 1; otherwise throw `std::invalid_argument` |
| `Pmf::uniform(outcomes)` | Every position gets the same probability (the probabilities of repeated values add up) |
| `support()` | Return the outcomes in the support, in increasing order |
| `prob(x)` | The probability of `x`; return 0 if it is not in the support |
| `expected()` | The expected value, when `T` is convertible to `Fraction` |
| `expected_of(f)` | `E[f(X)]`, where the return value of `f` is convertible to `Fraction` |
| `map(f)` | The distribution of `f(X)`, returned as a `Pmf<return type of f>` |

## Stage 3 — Joint distributions and linearity

```cpp
template <typename A, typename B>
Pmf<std::pair<A, B>> independent(const Pmf<A>& a, const Pmf<B>& b);
```

- `independent(a, b)`: return a `Pmf<pair<A, B>>` that represents the joint distribution of two **independent** random variables.
- The tests use `expected_of` to verify that `E[X+Y] = E[X] + E[Y]` (it holds for independent and dependent variables alike), and that `E[XY] = E[X]E[Y]` holds only under independence.

## Stage 4 — Real-world extension: quorum availability

```cpp
Pmf<int> convolve(const Pmf<int>& a, const Pmf<int>& b);

Fraction at_least_k_up(int replicas, const Fraction& p_up, int k);
```

- `convolve(a, b)`: return the distribution of `X + Y` for independent X and Y (`T` is `int`).
- `at_least_k_up(replicas, p_up, k)`: each machine is up independently with probability `p_up`. Return the exact probability that at least `k` machines are up. When `k ≤ 0`, return 1; when `k > replicas`, return 0.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. `E[X+Y] = E[X]+E[Y]` doesn't require independence. What about `E[XY] = E[X]E[Y]`? Give a counterexample of your own.
2. Why do large systems usually multiply and add probabilities as `double` values or as logarithms? When do exact fractions blow up (how fast does the number of digits in the numerator and denominator grow)?
3. Machines in a real cluster don't fail independently (same rack, same deployment). Does correlated failure make the result of `at_least_k_up` too optimistic or too pessimistic? How would you model it?

---

**Official solution:** [Problem Set 0 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cfc7d2d5bc3b9f4df35b5bdc7aa49284_MIT6_006S20_ps0-solutions.pdf). Read it after you finish.
