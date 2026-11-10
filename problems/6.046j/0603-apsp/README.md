---
title: All-Pairs Shortest Paths, Four Ways
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J Lecture 11
pdf: materials/6.046j/lectures/lec11.pdf
read: materials/6.046j/lectures/lec11.pdf
kind: code
minutes: 130
---

# 0603 · All-Pairs Shortest Paths, Four Ways

**Based on:** [Dynamic Programming: All-pairs Shortest Paths](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/312f4a419009b58f8147b75975db4347_MIT6_046JS15_lec11.pdf) · **Read first:** [Dynamic Programming: All-pairs Shortest Paths](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/312f4a419009b58f8147b75975db4347_MIT6_046JS15_lec11.pdf)

> **Why this comes up in practice:** precomputed routing tables, graph-distance features in recommender systems, and min/max delay constraints in schedulers all come down to one of these APSP algorithms or a variant of it. Which one you pick in practice depends on whether the graph is dense or sparse.

## Your task

Weights may be negative, but `w[i][i] == 0`. A missing edge is `kInf` (already defined in the header). You must report a **negative cycle**, not crash. Vertices are numbered `0..n-1`. In every distance matrix you return, an unreachable pair must be exactly `kInf` (never add a weight or a potential to `kInf`).

You implement the APSP algorithms from Lecture 11 (min-plus repeated squaring, Floyd–Warshall with path recovery, and Johnson's algorithm), then use shortest paths to solve difference constraints.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Min-plus matrices and repeated squaring

```cpp
inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;
using Matrix = std::vector<std::vector<long long>>;

Matrix min_plus(const Matrix& a, const Matrix& b);
std::optional<Matrix> apsp_repeated_squaring(const Matrix& w);
```

- `Matrix min_plus(const Matrix& a, const Matrix& b)`: `(a ⊗ b)[i][j] = min_k (a[i][k] + b[k][j])`, and it must handle `kInf` correctly.
- `std::optional<Matrix> apsp_repeated_squaring(const Matrix& w)`: return the distance matrix, or `std::nullopt` if there is a negative cycle. For an empty graph (n = 0), return an empty `Matrix`, not `std::nullopt`. O(n³ log n) time.

## Stage 2 — Floyd–Warshall and path recovery

```cpp
class FloydWarshall {
public:
    explicit FloydWarshall(Matrix w);      // O(n³)
    bool has_negative_cycle() const;
    long long dist(int u, int v) const;    // kInf means unreachable (meaningful only when there is no negative cycle)
    std::vector<int> path(int u, int v) const;  // {u, ..., v}; empty if unreachable or if there is a negative cycle; {u} when u==v
};
```

## Stage 3 — Johnson (sparse graphs)

```cpp
struct Edge {
    int from = 0;
    int to = 0;
    long long weight = 0;
};
std::optional<Matrix> johnson(int n, std::span<const Edge> edges);
```

`std::optional<Matrix> johnson(int n, std::span<const Edge> edges)`, with `Edge{from, to, weight}`. Compute potentials with Bellman–Ford, then run Dijkstra V times. O(VE + V² log V) time. Return `std::nullopt` if there is a negative cycle. The edge list may contain parallel edges; the lightest one counts.

## Stage 4 — Real-world extension: scheduling with difference constraints

```cpp
struct Constraint {
    int later = 0;
    int earlier = 0;
    long long bound = 0;
};
std::optional<std::vector<long long>> solve_difference_constraints(
    int n, std::span<const Constraint> constraints);
```

Workflow systems often have constraints such as "B must start 5 to 10 minutes after A finishes". Each constraint is written as `Constraint{later, earlier, bound}`, meaning `x[later] - x[earlier] ≤ bound` (the x values are integers and may be negative).

`std::optional<std::vector<long long>> solve_difference_constraints(int n, std::span<const Constraint> constraints)`: return an `x` that satisfies every constraint, or `std::nullopt` if there is none. O(VE) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't the matrix-multiplication version be sped up with Strassen? What is the min-plus semiring missing?
2. Why does Johnson's reweighting preserve every shortest path? Bellman–Ford aside, what condition must h satisfy?
3. A system of difference constraints has no solution exactly when the graph has a negative cycle. Explain in one sentence where the contradiction comes from (sum the inequalities around the cycle).
