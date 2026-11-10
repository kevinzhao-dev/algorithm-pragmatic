---
title: Simplex Solver (dual certificates)
unit: 08 · LP and NP-Completeness (PS8)
source: 6.046J PS8 Problem 8-1
pdf: materials/6.046j/psets/ps08/questions.pdf
answer: materials/6.046j/psets/ps08/solutions.pdf
read: materials/6.046j/lectures/lec15.pdf
kind: code
minutes: 180
---

# 0811 · Simplex Solver

**Original problem:** [Problem Set 8](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/29a28961d8c73e013ba78238e66dc2ab_MIT6_046JS15_pset8.pdf) · **Read first:** [Linear Programming: LP, Reductions, Simplex](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/927a49adf7b3c4a6de8c31393509552c_MIT6_046JS15_lec15.pdf)

> **Why this comes up in practice:** the innermost loop of an integer-programming solver is simplex, which everyone uses every day but few people have ever written. The sequence of vertices simplex visits, cycling under degeneracy, and the dual solution it finally outputs are what you look at most often when debugging an LP model.

## Original problem

6.046J PS8 Problem 8-1 works through one small LP by hand. It has two variables x1 and x2, three constraints, and nonnegativity:

- x1 + x2 ≤ 10
- x2 ≥ 4x1 − 20
- x1 + 3x2 ≤ 24
- x1, x2 ≥ 0

The objective is to maximize p = 4x1 + x2.

- (a) Sketch the feasible region.
- (b) Rewrite the LP in standard form, then convert that into slack form.
- (c) Solve the slack-form LP with simplex: say which pivot you pick at each step, show the LP after every pivot and the sequence of feasible solutions, and mark those solutions on the sketch from (a).
- (d) Write down the dual of your standard-form LP from (b) and give its optimal value; the hint is to use your result from (c).

## Your task

This exercise turns the problem set's hand computation into a real solver. All arithmetic uses the exact rational type `Frac` (from `_shared/06_fraction.hpp`). Standard form is `maximize c·x` subject to `A x ≤ b` and `x ≥ 0`, stored as `StandardLp{c, A, b}` (`A` is m×n).

Instead of drawing the region and pivoting by hand, you enumerate the vertices of any two-variable LP, run simplex on any LP in standard form, and return a dual certificate with the optimum.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Vertices of the feasible region (Part a)

```cpp
struct StandardLp {
    std::vector<Frac> c;
    std::vector<std::vector<Frac>> A;
    std::vector<Frac> b;
};

std::vector<std::pair<Frac, Frac>> feasible_vertices_2d(const StandardLp& lp);
```

`std::vector<std::pair<Frac,Frac>> feasible_vertices_2d(const StandardLp& lp)`: for an LP with exactly 2 variables, return every vertex `(x, y)` of the feasible region (without duplicates, in increasing lexicographic order). Return an empty vector when the feasible region is empty.

Hint: the candidate points are the pairwise intersections of the constraint lines and the lines `x = 0` and `y = 0`.

## Stage 2 — Simplex (Parts b and c)

```cpp
enum class LpStatus { Optimal, Infeasible, Unbounded };
struct LpResult {
    LpStatus status = LpStatus::Infeasible;
    Frac value;
    std::vector<Frac> x;      // length n
    std::vector<Frac> dual;   // used only from Stage 4 on, length m
};
LpResult solve_lp(const StandardLp& lp, std::vector<std::vector<Frac>>* trace = nullptr);
```

This stage only handles `b ≥ 0` (the origin is feasible). Pivot repeatedly in slack form, and always use **Bland's rule** to avoid cycling: the entering variable is the eligible one with the smallest index, and the leaving variable is tie-broken the same way, by the smallest basic index.

When `trace` is not null, append the values of the first n variables of the basic solution after each pivot, in order (including the starting point `0`).

## Stage 3 — Phase I: an initial feasible basis (extending Part c)

`b` may have negative entries. Use an auxiliary LP to find an initial feasible basis, or report `Infeasible`. When the LP is `Unbounded`, `solve_lp` returns `status = Unbounded` (`x`/`value` need not mean anything).

With `trace`, record only Phase II: the first entry is the initial feasible basic solution (the origin when `b ≥ 0`, otherwise the one Phase I finds), followed by one entry per Phase II pivot. Never record Phase I pivots or an infeasible origin, since the tests check that every entry is feasible; an `Infeasible` LP leaves the trace empty.

## Stage 4 — A dual certificate (Part d)

When `status == Optimal`, `dual` must be an optimal solution of the dual LP: `dual ≥ 0`, `Aᵀ·dual ≥ c`, and `b·dual == value`. This is the certificate of optimality: anyone holding `x` and `dual` can check the answer by hand, without having to trust your solver.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does Bland's rule prevent cycling? What does it cost? Which rules do practical solvers use?
2. What is a degenerate pivot? Why doesn't simplex get "stuck" when the objective value doesn't increase (under Bland's rule)?
3. Why is simplex exponential in the worst case (Klee–Minty), yet usually fast in practice? How does it compare with interior-point methods?

---

**Official solution:** [Solutions to Problem Set 8](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/e75cb61dde1769d9d9ea22069b5ce2c4_MIT6_046JS15_pset8sols.pdf). Read it after you finish.
