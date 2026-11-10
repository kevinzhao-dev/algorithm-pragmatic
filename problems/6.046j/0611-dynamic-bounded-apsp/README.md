---
title: Dynamic and Bounded-Hop All-Pairs Shortest Paths
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J PS6 Problem 6-1
pdf: materials/6.046j/psets/ps06/questions.pdf
answer: materials/6.046j/psets/ps06/solutions.pdf
read: materials/6.046j/lectures/lec11.pdf
kind: code
minutes: 150
---

# 0611 · Dynamic and Bounded-Hop APSP

**Original problem:** [Problem Set 6](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d33d30d2402b35cc4094df7cac04d113_MIT6_046JS15_pset6.pdf) · **Read first:** [Dynamic Programming: All-pairs Shortest Paths](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/312f4a419009b58f8147b75975db4347_MIT6_046JS15_lec11.pdf)

> **Why this comes up in practice:** WAN link weights change with congestion, and a routing engine doesn't want to recompute the whole table each time. SLAs also often impose limits like "at most h hops". This exercise turns static APSP into a routing-table engine that can be updated online.

## Original problem

6.046J PS6 Problem 6-1 extends the all-pairs shortest-paths algorithms from lecture and CLRS Chapter 25 in two directions. Throughout, edge weights are nonnegative reals and the vertices are numbered 1 to n. The motivating example is a communication network whose link weights change over time, so the shortest distances used for routing have to be kept up to date. Every part is answered on paper, with an algorithm and its analysis or with a construction.

- (a) You are given a weighted digraph G = (V, E, W), a correct distance matrix D, the matching predecessor matrix Π, and a triple (i, j, r) with i, j ∈ V and r a nonnegative real. Design an efficient algorithm that updates D and Π when w(i,j) is set to r. Treat the cases r = w(i,j), r < w(i,j) and r > w(i,j) separately and analyze each one; one of them is allowed to need O(V³) time in the worst case.
- (b) Construct a graph, its distance matrix D and a triple (i, j, r) for which every algorithm that updates D must take Ω(V²) time.

The remaining parts add a hop limit h, with 0 ≤ h ≤ n − 1: D must now hold the shortest distance over paths that use at most h edges.

- (c) Modify Floyd–Warshall to solve this bounded-hop APSP problem, and analyze its running time as a function of the graph's size and h.
- (d) Modify the matrix-multiplication method of CLRS Section 25.1 for the same problem, aiming for a running time that grows only logarithmically in h, and analyze it.
- (e) Make bounded-hop APSP dynamic. The input is G, h, a correct at-most-h-hop distance matrix D (plus any auxiliary distance data you choose to maintain), and a triple (i, j, r). Update D and your auxiliary data when w(i,j) is set to r, again handling and analyzing the three cases of part (a) in terms of the graph's size and h.

## Your task

All weights are nonnegative integers, `w[i][i] == 0`, and a missing edge is `kInf` (already defined in the header). An update with `i == j` is always a no-op. The classes in different stages are independent and don't interfere with each other.

Each stage implements one part, (a) through (e); for part (b) you build the lower-bound instance in code instead of describing it. Unlike the original, weights are integers, and Stage 1's `update` also returns how many distances changed.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Dynamic APSP (Part a)

```cpp
class DynamicApsp {
public:
    explicit DynamicApsp(Matrix w);         // O(V³)
    long long dist(int u, int v) const;     // kInf means unreachable
    std::vector<int> path(int u, int v) const;     // {u, ..., v}; empty if unreachable; {u} when u==v
    // Set w(i,j) to r (r may be kInf, which deletes the edge). Return the number of ordered pairs whose distance actually changed.
    long long update(int i, int j, long long r);
};
```

The three cases have different complexity requirements:

| Case | Complexity |
| --- | --- |
| `r == w(i,j)` | O(1) |
| `r < w(i,j)` (the edge gets lighter, or a new edge is added) | O(V²) |
| `r > w(i,j)` | O(V³) is acceptable |

After an update, both `dist` and `path` must give correct answers for the new graph.

## Stage 2 — A lower bound (Part b)

```cpp
struct Scenario {
    Matrix w;
    int i = 0;
    int j = 0;
    long long r = 0;
};
Scenario worst_update_scenario(int n);
```

`struct Scenario { Matrix w; int i; int j; long long r; };`

`Scenario worst_update_scenario(int n)`: given `n ≥ 2`, build a graph on n vertices and a single update that forces any algorithm to change Ω(n²) entries of `D` (concretely, `update` must return at least `n*n/4`). Think about how many ordered pairs can have their distance go from `kInf` to a finite value because of one edge.

The tests check this by running your Stage 1 `DynamicApsp` on the scenario: its `update` must return the exact number of ordered pairs that changed, and `dist` and `path` must be correct afterwards, so finish Stage 1 first.

## Stage 3 — Bounded-hop Floyd-Warshall (Part c)

```cpp
Matrix bounded_hop_floyd(const Matrix& w, int h);
```

`Matrix bounded_hop_floyd(const Matrix& w, int h)`: return `D` where `D[i][j]` is the weight of the shortest i→j path with at most h edges, for `0 ≤ h ≤ n-1`. O(V³ h²) time and O(V² h) space.

## Stage 4 — Bounded-hop repeated squaring (Part d)

```cpp
Matrix bounded_hop_doubling(const Matrix& w, int h);
```

`Matrix bounded_hop_doubling(const Matrix& w, int h)`: the same definition as above, in O(V³ log h) time.

## Stage 5 — Dynamic bounded-hop APSP (Part e)

```cpp
class DynamicBoundedApsp {
public:
    DynamicBoundedApsp(Matrix w, int h);
    long long dist(int u, int v) const;                 // at most h hops
    long long dist_at_most(int u, int v, int k) const;  // at most k hops, 0 ≤ k ≤ h
    void update(int i, int j, long long r);
};
```

Construction takes O(V³ h). An update takes O(1) when `r == w`, O(V² h²) when `r < w`, and O(V³ h) when `r > w`. Unlike Stages 3 and 4, this class must accept any `h ≥ 1`, including `h = n` (the tests use `1 ≤ h ≤ n`), and as in Stage 1 an update may set `r` to `kInf`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. For an update with `r < w(i,j)`, why can you safely use `d(x,i)` and `d(j,y)` without recomputing them? What assumption does this rely on (would negative edges be OK)?
2. Why is repeated squaring better than Floyd-Warshall when hops are bounded? Isn't it the other way around for ordinary APSP?
3. Does `r > w(i,j)` really need O(V³)? Could you recompute only the sources whose shortest-path tree uses this edge? How much does that save in the worst case?

---

**Official solution:** [Solutions to Problem Set 6](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/832add4a924cdca6111fe115e6bba85e_MIT6_046JS15_pset6sols.pdf). Read it after you finish.
