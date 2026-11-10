---
title: MST Variants (Borůvka, a D&C counterexample, cycle-breaking)
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J PS6 Problem 6-2
pdf: materials/6.046j/psets/ps06/questions.pdf
answer: materials/6.046j/psets/ps06/solutions.pdf
read: materials/6.046j/lectures/lec12.pdf
kind: code
minutes: 130
---

# 0612 · MST Variants

**Original problem:** [Problem Set 6](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d33d30d2402b35cc4094df7cac04d113_MIT6_046JS15_pset6.pdf) · **Read first:** [Greedy Algorithms: Minimum Spanning Tree](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/4a7fdddff3bc419c70bb470106a1663a_MIT6_046JS15_lec12.pdf)

> **Why this comes up in practice:** Borůvka is the basis of distributed and GPU MST algorithms: in each round every component picks its own lightest edge, so it is naturally parallel. The cycle property is the core of maintaining a backbone incrementally. Once you have run all of these MST variants that go beyond the lecture notes, you can tell which approaches that "look right" are actually wrong.

## Original problem

6.046J PS6 Problem 6-2 considers an undirected graph G = (V, E) whose edge weights are nonnegative reals and pairwise distinct.

- (a) Prove that with distinct weights, G has exactly one minimum spanning tree.

Parts (b)–(d) each sketch an MST algorithm for graphs with distinct weights. For each one, decide whether it is correct. If it is, prove it, refine it into an efficient algorithm, and analyze that algorithm; if it isn't, show a concrete counterexample.

- (b) Batched edge addition: grow a set A of MST edges in phases, starting from the empty set. In each phase, find for every component tree C of the forest A the lightest edge e_C that leaves C, then add all of these edges to A together. Stop once A is a spanning tree.
- (c) Divide and conquer: split the vertices in any way into two disjoint sets V1 and V2 of about V/2 vertices each. Let G1 = (V1, E1) keep only the edges with both endpoints in V1, and define G2 = (V2, E2) the same way. Recursively compute the MSTs T1 of G1 and T2 of G2, then join them with the lightest edge between V1 and V2.
- (d) Cycle breaking: in each phase, choose a nonempty set of simple cycles in the graph and delete the heaviest edge of every chosen cycle. Stop once a spanning tree remains.

## Your task

Every graph is given as `Edge{u, v, w}` edges on vertices `0..n-1`, and the input is guaranteed to be connected (except in Stage 5). Return edge indices in increasing order. Except in Stage 1, **all weights are distinct** (Stage 5 too). Graphs may contain parallel edges, but never self-loops.

Stage 1 decides MST uniqueness even when weights repeat, which goes beyond part (a). Stages 2–4 implement the algorithms of parts (b)–(d) as described, and Stage 5 adds incremental MST maintenance, which the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — MST uniqueness (Part a)

```cpp
struct Edge {
    int u = 0;
    int v = 0;
    long long w = 0;
};

bool has_unique_mst(int n, std::span<const Edge> edges);
```

`bool has_unique_mst(int n, std::span<const Edge> edges)`: weights **may repeat**; decide whether this connected graph has a unique MST. O(E log E). Two MSTs count as different when they use different edge indices, so two parallel edges of equal weight can make the MST non-unique.

## Stage 2 — Batched edge addition (Part b, Borůvka)

```cpp
struct BoruvkaResult {
    std::vector<int> edges;
    int phases = 0;
};
BoruvkaResult boruvka(int n, std::span<const Edge> edges);
```

`BoruvkaResult boruvka(int n, std::span<const Edge> edges)`, with `BoruvkaResult{edges, phases}`: in each phase, find the lightest edge crossing the cut around each current component, and add them all at once, until you have a single tree. `phases` is the total number of phases, which must be ≤ ⌈lg n⌉. O(E log V).

## Stage 3 — Why divide and conquer fails (Part c)

```cpp
std::vector<int> divide_and_conquer_mst(int n, std::span<const Edge> edges);
```

Implement it as the problem describes: split the vertices in the range `[lo, hi)` by number into `[lo, mid)` and `[mid, hi)` with `mid = lo + (hi - lo) / 2`, keep in each subgraph only the edges with both ends in that half, find each half's MST recursively, and then add the lightest edge that crosses between the two halves. Recurse down to single vertices. Only the whole graph is guaranteed to be connected: when no edge crosses a split, add nothing for that split.

```cpp
struct DcTrap { int n; std::vector<Edge> edges; };
DcTrap divide_and_conquer_trap();
```

Return a connected graph with distinct weights such that (with the split above) both halves are connected on their own, yet `divide_and_conquer_mst` does not return the true MST. The tests also require its result on your graph to be a spanning tree that is strictly heavier than the true MST, so every split in the recursion needs a crossing edge.

## Stage 4 — Cycle-breaking (Part d)

```cpp
std::vector<int> cycle_breaking_mst(int n, std::span<const Edge> edges);
```

`std::vector<int> cycle_breaking_mst(int n, std::span<const Edge> edges)`: as long as the graph still has a cycle, find one and delete its heaviest edge. Return the edges that remain. O(E²) is acceptable. Two parallel edges form a cycle of length two.

## Stage 5 — Real-world extension: maintaining a backbone incrementally

```cpp
class IncrementalMst {
public:
    explicit IncrementalMst(int n);
    // The edge from the k-th call has id k (counting from 0). Return the id of the edge this pushes out of the forest:
    // either an old edge or the new edge itself. If the new edge just joins two components, return nullopt.
    std::optional<int> add_edge(int u, int v, long long w);
    long long weight() const;            // total weight of the current forest
    std::vector<int> edges() const;      // edge ids in the current forest, increasing
};
```

Each `add_edge` takes O(V). Weights are distinct. The graph may be disconnected, in which case you maintain a forest.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Use an exchange argument to prove that the MST is unique when all weights are distinct (the premise of Stage 1). How does Stage 1 decide when weights tie?
2. Why does Borůvka at least halve the number of components in every round? How parallel can each round be, and where is the bottleneck?
3. Which step of divide and conquer is wrong? Why isn't adding a single edge across the cut enough, even though the cut property isn't violated?

---

**Official solution:** [Solutions to Problem Set 6](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/832add4a924cdca6111fe115e6bba85e_MIT6_046JS15_pset6sols.pdf). Read it after you finish.
