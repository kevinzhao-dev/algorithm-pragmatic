---
title: Recurrence Solver (Master Theorem and recursion trees)
unit: 02 · Sorting & Recurrences (PS2)
source: 6.006 PS2 Problem 2-1
pdf: materials/6.006/psets/ps2/questions.pdf
answer: materials/6.006/psets/ps2/solutions.pdf
read: materials/6.006/recitations/rec03.pdf
kind: code
minutes: 60
---

# 0211 · Recurrence Solver

**Original problem:** [Problem Set 2 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/70d9c6b212c6dfce6723259ffb429521_MIT6_006S20_ps2-questions.pdf) · **Read first:** [Recitation 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/1869dbf640ded6b31f1bd369d2001ef5_MIT6_006S20_r03.pdf)

> **Why this comes up in practice:** when an interviewer asks "what's the complexity of this divide-and-conquer algorithm?", you need to see at a glance whether it is leaf-heavy, balanced, or root-heavy, so that you know which level to optimize. The doubling experiment in performance analysis (double n and see how many times longer it takes) is the measured version of the same idea.

## Original problem

6.006 PS2 Problem 2-1 is about solving recurrences. For each of the four recurrences below, find the tightest upper and lower bounds it allows, assuming T(1) = Θ(1). Solve (a), (b), and (c) in two ways, once with a recursion tree and once with the Master Theorem; solve (d) by substitution only.

- (a) T(n) = 4T(n/2) + O(n).
- (b) T(n) = 3T(n/√2) + O(n^4).
- (c) T(n) = 2T(n/2) + 5n log n.
- (d) T(n) = T(n − 2) + Θ(n).

The answers are written; there is no code.

## Your task

The original problem asks you to solve four recurrences and draw their recursion trees. This exercise turns that process into code: the Master Theorem, the work on each level of a recursion tree, and finally a numerical check of your answers. **Working the original four parts by hand (see the Original problem section above) is in the interview follow-ups.**

`Growth{exponent, log_power}` stands for `Θ(n^exponent · log^log_power n)`. Compare exponents with a tolerance of 1e-9 (`b` can be irrational, √2 for example).

Stages 3 and 4 handle two shapes from the original that the plain Master Theorem can't settle on its own: a driving term known only as `O(...)`, and a recurrence that subtracts a constant.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Master Theorem

```cpp
struct Growth {
    double exponent;
    int log_power;
};

enum class MasterCase { Leaves, Balanced, Root };

struct MasterResult {
    MasterCase which;
    Growth growth;
};

MasterResult solve_master(double a, double b, double c, int k);
```

`solve_master(a, b, c, k)` solves `T(n) = a·T(n/b) + Θ(n^c · log^k n)` with `T(1) = Θ(1)`. Return `MasterResult{which, growth}`:

| `which` | Condition (`e = log_b a`) | `growth` |
| --- | --- | --- |
| `MasterCase::Leaves` | `c < e` | `Θ(n^e)` |
| `MasterCase::Balanced` | `c == e` | `Θ(n^c · log^(k+1) n)` |
| `MasterCase::Root` | `c > e` | `Θ(n^c · log^k n)` |

The parameters must satisfy `a ≥ 1`, `b > 1`, `c ≥ 0`, and `k ≥ 0`; otherwise throw `std::invalid_argument`.

## Stage 2 — The recursion tree

```cpp
std::vector<double> level_work(double a, double b, double c, int k, int depth);
double tree_total(double a, double b, double c, int k, int depth);
```

- `level_work(a, b, c, k, depth)`: let `n = b^depth`, and return the total work on each level of the recursion tree (a `vector<double>`, where level 0 is the root). Level `i` (`i < depth`) has `a^i` subproblems, each of size `s = b^(depth−i)` and cost `s^c · (log₂ s)^k`; the last level, `i == depth`, is the leaves: `a^depth` subproblems that cost 1 each.
- `tree_total(a, b, c, k, depth)`: the sum over all levels.

The tests check the shape of the per-level work (growing / flat / shrinking from one level to the next), and use it to double-check Stage 1: the ratio of `tree_total` to `Θ(growth)` must level off as depth gets large.

## Stage 3 — An f(n) with only an upper bound

```cpp
struct Bounds {
    Growth lower;
    Growth upper;
    bool tight() const;  // provided
};

Bounds bounds_with_big_o(double a, double b, double c, int k);
```

When the original problem writes `f(n)` as `O(...)`, all you can pin down is an upper bound. `bounds_with_big_o(a, b, c, k)` solves `T(n) = a·T(n/b) + O(n^c · log^k n)` and returns `Bounds{lower, upper}`:

- `upper`: the Master Theorem result when `f` is treated as `Θ(n^c log^k n)`.
- `lower`: `f` can be as small as 0, so this is `Θ(n^e)` (the contribution of the leaves).
- `tight()`: whether `lower` and `upper` are the same (meaning the `O` doesn't actually affect the result).

The parameters must satisfy the same conditions as in Stage 1 (`a ≥ 1`, `b > 1`, `c ≥ 0`, and `k ≥ 0`); otherwise throw `std::invalid_argument`.

## Stage 4 — Decrease-by-a-constant recurrences (chains)

```cpp
Growth solve_chain(double c, int k);
std::uint64_t chain_work(std::uint64_t s, int c, std::uint64_t n);
```

The Master Theorem doesn't apply to `T(n) = T(n − s) + Θ(n^c · log^k n)` (where `s ≥ 1` is a constant):

- `solve_chain(c, k)`: return a `Growth`; the answer is `Θ(n^(c+1) · log^k n)`. The parameters must satisfy `c ≥ 0` and `k ≥ 0`; otherwise throw `std::invalid_argument`.
- `chain_work(s, c, n)`: compute `Σ (n − i·s)^c` exactly (`i` starts at 0, and the sum continues as long as `n − i·s ≥ 1`), where `c` is a non-negative integer, and return a `uint64_t`. The tests confirm that when n doubles, the ratio matches the answer from `solve_chain`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Work the four parts of Problem 2-1 by hand (they are in the Original problem section above; don't look at the solutions).
2. Why does Case 3 of the Master Theorem need a regularity condition? Why does it hold automatically for a polynomial like `f(n) = n^c`?
3. On a recursion tree, the work per level forms a geometric series: what determines the common ratio? Which case does a ratio > 1, = 1, or < 1 correspond to, and how does each one map to the Θ result?

---

**Official solution:** [Problem Set 2 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/b06b8a8baf9b15f98947e75bae55b8bf_MIT6_006S20_ps2-solutions.pdf). Read it after you finish.
