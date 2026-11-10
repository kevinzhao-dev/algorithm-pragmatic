---
title: Tournament Feedback Arc (kernel and FPT search)
unit: 09 · Approximation and FPT (PS9)
source: 6.046J PS9 Problem 9-2
pdf: materials/6.046j/psets/ps09/questions.pdf
answer: materials/6.046j/psets/ps09/solutions.pdf
read: materials/6.046j/lectures/lec18.pdf
kind: code
minutes: 160
---

# 0912 · Tournament Feedback Arc

**Original problem:** [Problem Set 9](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bb6a432d876819c618c5be099dc5e2cb_MIT6_046JS15_pset9.pdf) · **Read first:** [Complexity: Fixed-parameter Algorithms](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/2cf0d5a0689efafd2bf2d13954d8ad9b_MIT6_046JS15_lec18.pdf)

> **Why this comes up in practice:** repairing a pile of pairwise comparisons (A beats B) into one consistent ranking, as in model evaluation or pairwise preferences between search results. "What is the fewest comparisons you must flip to get a total order?" is exactly feedback arc set, and when there are few inconsistencies, FPT beats brute force by several orders of magnitude.

## Original problem

6.046J PS9 Problem 9-2 is about the Tournament Edge Reversal problem. A **tournament** is a directed graph T = (V, E) in which, for every pair of vertices u, v, exactly one of (u, v) and (v, u) is an edge. A **cycle cover** is a set A ⊂ E of edges such that every directed cycle of T contains at least one edge of A.

- (a) A minimal cycle cover is one with the fewest edges. Prove that reversing every edge of a minimal cycle cover A makes T acyclic. The hint: each edge of A must be the only edge of A on some directed cycle of T.

Tournament Edge Reversal takes a tournament T and a positive integer k and asks whether T has a cycle cover of size at most k.

- (b) Show that this problem has a kernel with at most k² + 2k vertices. The hint: call a directed cycle of length 3 a triangle, and count how many triangles each vertex or edge belongs to.
- (c) Give an FPT algorithm for Tournament Edge Reversal.

## Your task

A `Tournament` is a `std::vector<std::vector<char>>`: `t[u][v] == 1` means `u → v`. For `u ≠ v`, exactly one direction is 1, and the diagonal is 0.

A **cycle cover** is a set of edges A such that every directed cycle has at least one edge in A. The problem asks whether T has a cycle cover of size ≤ k. Once every edge in A is reversed, T becomes acyclic (Part a).

Stage 1 builds the triangle tools behind part (a), Stages 2 and 3 implement the kernel of part (b) and the FPT search of part (c), and Stage 4 turns the result into ranking repair, which the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Triangles (the basis of Part a)

```cpp
using Tournament = std::vector<std::vector<char>>;

bool is_acyclic(const Tournament& t);
std::optional<std::array<int, 3>> find_triangle(const Tournament& t);
```

- `bool is_acyclic(const Tournament& t)`: O(n²).
- `std::optional<std::array<int,3>> find_triangle(const Tournament& t)`: find `a → b → c → a`, or return `nullopt` if there is none. A tournament has a cycle ⟺ it has a triangle. O(n²).

## Stage 2 — Kernelization (Part b)

```cpp
struct TournamentKernel {
    bool no_instance = false;
    Tournament reduced;
    int k = 0;
    std::vector<int> to_original;            // vertex i of reduced maps to this original id
    std::vector<std::pair<int,int>> reversed;  // edges (u,v) already chosen for reversal; u→v is their direction in T, original ids
};
TournamentKernel kernelize(const Tournament& t, int k);
```

Rules (apply repeatedly): Rule 1 — if an edge lies in ≥ `k+1` triangles, reverse it and set `k` to `k-1`; Rule 2 — delete every vertex that lies in no triangle. If `k < 0` at any point, or more than `k(k+2)` vertices remain at the end, return `no_instance`.

## Stage 3 — Solving with FPT (Part c)

```cpp
std::optional<std::vector<std::pair<int, int>>> feedback_arcs(const Tournament& t, int k);
```

`std::optional<std::vector<std::pair<int,int>>> feedback_arcs(const Tournament& t, int k)`: if one exists, return a set of edges `(u, v)` (with `u→v` in T, and at most k of them) whose reversal makes T acyclic; otherwise return `nullopt`. Kernelize first, then run a bounded search on the kernel: find a triangle and branch three ways, reversing each of its edges in turn, with recursion depth at most k (O(3^k·n²)). If `k < 0`, return `nullopt`, even when T is already acyclic.

## Stage 4 — Real-world extension: repairing a ranking

```cpp
std::optional<std::vector<int>> repair_ranking(const Tournament& t, int max_upsets);
```

`std::optional<std::vector<int>> repair_ranking(const Tournament& t, int max_upsets)`: if it can be done by reversing at most `max_upsets` edges, return the final total order (rank 0 first) with the fewest violations of the original comparisons (upsets); otherwise return `nullopt`. An edge `u → v` means u ranks ahead of v, so an upset is an original edge `u → v` with v placed ahead of u.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does reversing a "minimal" cycle cover guarantee an acyclic result? (Hint: every edge in A has a cycle of its own.)
2. Why does the safety of Rule 1 depend on the `k+1` threshold? Why doesn't Rule 2 affect the answer?
3. Where does the kernel size `k² + 2k` come from? How could this exercise's FPT bound be improved, in theory and in practice (branch on a packing of disjoint triangles)?

---

**Official solution:** [Solutions to Problem Set 9](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ff14ef9418b742bf99cfa9f52c6accd7_MIT6_046JS15_pset9sols.pdf). Read it after you finish.
