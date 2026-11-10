---
title: NP-Completeness (writing reductions as code)
unit: 08 · LP and NP-Completeness (PS8)
source: 6.046J Lecture 16, Recitation 8
pdf: materials/6.046j/lectures/lec16.pdf
read: materials/6.046j/lectures/lec16.pdf, materials/6.046j/recitations/rec08.pdf
kind: code
minutes: 150
---

# 0802 · Writing Reductions as Code

**Based on:** [Complexity: P, NP, NP-completeness, Reductions](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/f2829e57caade7e7f21d765b8e7b9058_MIT6_046JS15_lec16.pdf) · **Read first:** [Complexity: P, NP, NP-completeness, Reductions](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/f2829e57caade7e7f21d765b8e7b9058_MIT6_046JS15_lec16.pdf), [Complexity: More Reductions](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/1dee182d301235b901119a0ff57f05e2_MIT6_046JS15_Recitation8.pdf)

> **Why this comes up in practice:** whether a requirement is NP-hard decides whether you should look for an exact solution, an approximation, or a heuristic. Only after writing a reduction as code do you really understand what "both directions must hold" means.

## Your task

Each stage produces three kinds of function: a **reduction** (turns an instance of problem A into an instance of problem B), a **verifier** (checks a witness; this is the NP-membership part), and a **lift** (turns a witness for B back into a witness for A, which proves the "B says yes ⇒ A says yes" direction). On small random instances, the tests use brute-force solutions to check that both sides give the same answer, and they check the witnesses your lifts return.

Every graph is a `Graph{n, edges}` with vertices `0..n-1` and edges `{u, v}` (undirected, `u != v`, no duplicates).

You code the classic reductions from Lecture 16 and Recitation 8, from Clique ≤ₚ Independent Set through 3DM ≤ₚ Subset Sum.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Verifiers

```cpp
struct Graph {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
};

bool is_clique(const Graph& g, std::span<const int> vertices);
bool is_independent_set(const Graph& g, std::span<const int> vertices);
bool is_vertex_cover(const Graph& g, std::span<const int> vertices);
bool is_set_cover(int universe, const std::vector<std::vector<int>>& sets,
                  std::span<const int> chosen);
```

Write the following polynomial-time verifiers:

| Function | What it checks |
| --- | --- |
| `is_clique(g, vertices)` | Every two vertices are adjacent |
| `is_independent_set(g, vertices)` | No two vertices are adjacent |
| `is_vertex_cover(g, vertices)` | Every edge has at least one endpoint in the set |
| `is_set_cover(universe, sets, chosen)` | The union of the sets in `chosen` is `0..universe-1` |

`vertices` and `chosen` hold distinct indices.

## Stage 2 — Clique ≤ₚ Independent Set ≤ₚ Vertex Cover ≤ₚ Set Cover

```cpp
struct CliqueInstance {
    Graph g;
    int k = 0;
};
struct IsInstance {
    Graph g;
    int k = 0;
};
struct VcInstance {
    Graph g;
    int k = 0;
};
struct SetCoverInstance {
    int universe = 0;
    std::vector<std::vector<int>> sets;
    int k = 0;
};
IsInstance clique_to_is(const CliqueInstance& src);
VcInstance is_to_vc(const IsInstance& src);
SetCoverInstance vc_to_set_cover(const VcInstance& src);
std::vector<int> lift_is_to_clique(const CliqueInstance& src, std::span<const int> witness);
std::vector<int> lift_vc_to_is(const IsInstance& src, std::span<const int> witness);
std::vector<int> lift_set_cover_to_vc(const VcInstance& src, const SetCoverInstance& dst,
                                      std::span<const int> chosen);
```

- `IsInstance clique_to_is(const CliqueInstance&)`, where `IsInstance` is `{g, k}`, and so is `CliqueInstance`.
- `VcInstance is_to_vc(const IsInstance&)`.
- `SetCoverInstance vc_to_set_cover(const VcInstance&)`, which returns `{universe, sets, k}`.
- Lifts: `lift_is_to_clique(src, witness)`, `lift_vc_to_is(src, witness)`, and `lift_set_cover_to_vc(src, dst, chosen)` turn a witness for the target problem back into a witness for the source problem.

The tests pin down the standard constructions. `clique_to_is` returns the complement graph on the same `n` vertices (each non-edge listed once) with the same `k`; `is_to_vc` returns the same graph with `n - k`; `vc_to_set_cover` makes the universe the edge indices and returns one set per vertex (the indices of its incident edges, possibly empty) with the same `k`. The brute-force oracles look for witnesses of exactly `k` elements, so keep every vertex's set even when it is empty, and make lifted clique and independent-set witnesses exactly `k` vertices long (a lifted vertex cover may have at most `k`).

## Stage 3 — Subset Sum ≤ₚ Partition

```cpp
struct SubsetSumInstance {
    std::vector<long long> a;
    long long t = 0;
};
struct PartitionInstance {
    std::vector<long long> a;
};
PartitionInstance subset_sum_to_partition(const SubsetSumInstance& src);
std::vector<int> lift_partition_to_subset_sum(const SubsetSumInstance& src,
                                              const PartitionInstance& dst,
                                              std::span<const int> side);
```

`PartitionInstance subset_sum_to_partition(const SubsetSumInstance& s)`, with `SubsetSumInstance{a, t}` and `PartitionInstance{a}`; the values in `a` are nonnegative integers.

`lift_partition_to_subset_sum(src, dst, side)`: `side` holds the indices of `dst.a` that fall on one side (their sum is half of the total); return a subset of the indices of `src.a` whose sum is `src.t`.

## Stage 4 — Hamiltonian Cycle ≤ₚ Hamiltonian Path

```cpp
struct Digraph {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
};
Digraph ham_cycle_to_path(const Digraph& g);
std::vector<int> lift_path_to_cycle(const Digraph& src, const Digraph& dst,
                                    std::span<const int> path);
```

Directed graphs are `Digraph{n, edges}`. `Digraph ham_cycle_to_path(const Digraph& g)`; `std::vector<int> lift_path_to_cycle(const Digraph& src, const Digraph& dst, std::span<const int> path)`: `path` is a Hamiltonian path of `dst` (a sequence of vertices); return a Hamiltonian cycle of `src` (a sequence of vertices of length `n`; the edge back to the start is implied).

## Stage 5 — 3DM ≤ₚ Subset Sum (weak NP-hardness)

```cpp
struct ThreeDm {
    int n = 0;
    std::vector<std::array<int, 3>> triples;
};
SubsetSumInstance three_dm_to_subset_sum(const ThreeDm& t);
std::vector<int> lift_subset_sum_to_3dm(const ThreeDm& src, const SubsetSumInstance& dst,
                                        std::span<const int> chosen);
```

Three-dimensional matching: `ThreeDm{n, triples}`, with `triples[i] = {x, y, z}` and `0 ≤ x,y,z < n`. The question is whether some n triples are pairwise disjoint in every coordinate (a perfect matching).

`SubsetSumInstance three_dm_to_subset_sum(const ThreeDm& t)`; `lift_subset_sum_to_3dm(src, dst, chosen)`: turn a subset of indices of the subset-sum instance back into a subset of triple indices, in increasing order. Emit exactly one number per triple (`a[i]` encodes `triples[i]`). The base of your encoding must be large enough that carries can't contaminate neighboring digits (the numbers in this stage grow exponentially).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Explain YES⇒YES and NO⇒NO for a Karp reduction. Why can't you prove just one direction?
2. Subset Sum is weakly NP-hard. Why does it have a pseudo-polynomial algorithm while 3-Partition doesn't?
3. Why does the definition of NP only require short witnesses for YES instances? What is co-NP?
