---
title: LP Modeling (standard form, slack form, duality)
unit: 08 · LP and NP-Completeness (PS8)
source: 6.046J Lecture 15
pdf: materials/6.046j/lectures/lec15.pdf
read: materials/6.046j/lectures/lec15.pdf
kind: code
minutes: 110
---

# 0801 · LP Modeling

**Based on:** [Linear Programming: LP, Reductions, Simplex](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/927a49adf7b3c4a6de8c31393509552c_MIT6_046JS15_lec15.pdf) · **Read first:** [Linear Programming: LP, Reductions, Simplex](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/927a49adf7b3c4a6de8c31393509552c_MIT6_046JS15_lec15.pdf)

> **Why this comes up in practice:** ad budget allocation, production scheduling, and network traffic engineering all start by writing the business problem as an LP. Get a direction wrong in the standard form or forget one slack variable, and the solver gives you an answer that is mathematically correct but not at all what you wanted.

## Your task

All numbers are exact rationals, `Frac` (from `_shared/06_fraction.hpp`, provided). The standard form used here is `maximize c·x` subject to `A x ≤ b` and `x ≥ 0`.

Following Lecture 15, you convert general LPs to standard form, implement the slack-form pivot, model max flow and shortest paths as LPs, and take duals.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Converting to standard form

```cpp
struct GeneralLp {
    bool maximize = true;
    std::vector<Frac> objective;
    std::vector<GeneralConstraint> constraints;   // GeneralConstraint{coeffs, relation, rhs}
    std::vector<bool> nonnegative;                // nonnegative[j] == false means x_j has no sign constraint
};
```

```cpp
enum class Relation { LessEq, GreaterEq, Equal };

struct GeneralConstraint {
    std::vector<Frac> coeffs;
    Relation relation = Relation::LessEq;
    Frac rhs;
};

struct StandardLp {
    std::vector<Frac> c;
    std::vector<std::vector<Frac>> A;
    std::vector<Frac> b;
};

struct StandardForm {
    StandardLp lp;
    // Add whatever you need here to recover the original solution.
};

StandardForm to_standard(const GeneralLp& lp);
std::vector<Frac> recover_solution(const StandardForm& form, std::span<const Frac> x);
Frac recover_value(const StandardForm& form, const Frac& standard_value);
```

- `StandardForm to_standard(const GeneralLp& lp)`: return a `StandardForm` whose `.lp` is a `StandardLp{c, A, b}` (the LP in standard form). Convert it with the four rules from the lecture notes (minimize, free variables, equalities, ≥). You may add fields to `StandardForm` if you need them. Only free variables get split and no slack columns are added, so an LP that is already in standard form keeps exactly its `n` columns.
- `std::vector<Frac> recover_solution(const StandardForm& form, std::span<const Frac> x)`: turn a solution `x` of the standard LP back into a solution over the original problem's `n` variables.
- `Frac recover_value(const StandardForm& form, const Frac& standard_value)`: turn the standard LP's objective value back into the original problem's objective value.

## Stage 2 — Slack form and pivoting

```cpp
struct SlackForm {
    std::vector<int> nonbasic, basic;      // variable indices
    std::vector<std::vector<Frac>> a;
    std::vector<Frac> b;
    std::vector<Frac> c;                   // objective z = v + Σ_j c[j]·x_nonbasic[j]
    Frac v;
};
```

```cpp
SlackForm to_slack(const StandardLp& lp);
SlackForm pivot(const SlackForm& form, int e, int l);
```

- The equations: `x_basic[i] = b[i] - Σ_j a[i][j]·x_nonbasic[j]`.
- `SlackForm to_slack(const StandardLp& lp)`: variables `0..n-1` are nonbasic and the slacks `n..n+m-1` are basic (`basic[i] = n + i`).
- `SlackForm pivot(const SlackForm& form, int e, int l)`: `e` is a position in `nonbasic`, `l` is a position in `basic`, and `a[l][e] != 0`. Return the equivalent slack form after swapping `nonbasic[e]` and `basic[l]` (everything else keeps its order: the new basic variable stays at `basic[l]` and the new nonbasic variable at `nonbasic[e]`).

## Stage 3 — Modeling: max flow and shortest paths

```cpp
struct FlowEdge {
    int u = 0;
    int v = 0;
    long long capacity = 0;
};
struct WeightedArc {
    int from = 0;
    int to = 0;
    long long weight = 0;
};
GeneralLp max_flow_lp(int n, std::span<const FlowEdge> edges, int s, int t);
GeneralLp shortest_path_lp(int n, std::span<const WeightedArc> arcs, int s);
```

- `GeneralLp max_flow_lp(int n, std::span<const FlowEdge> edges, int s, int t)`: the variables are the flows on the edges (`FlowEdge{u, v, capacity}`, with the variables in the same order as the edges), and the LP maximizes the flow value, which is the net flow out of `s` (flow on edges leaving `s` minus flow on edges entering `s`). Edges may enter `s` or leave `t`, and capacities may be 0.
- `GeneralLp shortest_path_lp(int n, std::span<const WeightedArc> arcs, int s)`: the variables are the distances `d(v)` between each vertex and `s`, modeled the way the lecture notes do it (`WeightedArc{from, to, weight}`). Every vertex is guaranteed to be reachable from `s`. That model has one variable per vertex and maximizes Σ_v d(v) subject to d(v) − d(u) ≤ w(u, v) for every arc and d(s) = 0, so the tests expect its optimum to equal the sum of all shortest-path distances from `s`.

## Stage 4 — Duality

```cpp
StandardLp dual_of(const StandardLp& lp);
```

`StandardLp dual_of(const StandardLp& lp)`: return the dual problem rewritten in standard form, so that the primal's maximum equals the negation of the maximum of `dual_of` (strong duality). The dual has one variable per primal constraint and one constraint per primal variable: its `c` has m entries, its `A` is n×m, and its `b` has n entries.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does splitting an equality constraint into two ≤ constraints leave the solution set unchanged? Why doesn't splitting a free variable change the objective value?
2. Take the dual of the max-flow LP: what does each dual variable mean? How do you read a min cut off an optimal dual solution?
3. A pivot is just an algebraically equivalent rewrite. Why doesn't it change the feasible set, even though it changes the current basic solution?
