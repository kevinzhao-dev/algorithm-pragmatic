---
title: Edge-Disjoint Routing (Menger certificates and hub limits)
unit: 07 · Network Flow (PS7)
source: 6.046J PS7 Problem 7-2
pdf: materials/6.046j/psets/ps07/questions.pdf
answer: materials/6.046j/psets/ps07/solutions.pdf
read: materials/6.046j/lectures/lec14a.pdf
kind: code
minutes: 110
---

# 0712 · Edge-Disjoint Routing

**Original problem:** [Problem Set 7](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/93951baaee86c66b1f2fd48e2c493745_MIT6_046JS15_pset7.pdf) · **Read first:** [Incremental Improvement: Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bdafe05f4f3c20cabfd08d2fbd099f63_MIT6_046JS15_lec14A.pdf)

> **Why this comes up in practice:** failure isolation in a multi-tenant data center. k ingest clusters send data to the same sink, and no two of their paths share a link, so a single link failure affects only one tenant. When the routes don't fit, operations needs to know where the bottleneck is, not just "impossible".

## Original problem

6.046J PS7 Problem 7-2 ("Disjoint Roads") is about k trucking companies c1, …, ck that share one road network, which is a directed graph. Company ci ships from its own source vertex si, and every company delivers to the same target vertex t; these k + 1 vertices are all distinct. So that the companies don't interfere with each other, they want k edge-disjoint paths, one from each si to t. Trucks from different companies may pass through the same vertex. Design an algorithm that finds such k paths when they exist, and answers "impossible" otherwise.

## Your task

The graph is directed, with `edges[i] = {u, v}` on vertices `0..n-1`, and it may have parallel edges. The `sources[0..k-1]` are distinct from each other and from `target`. A path is a sequence of edge indices: the first edge starts at `sources[i]`, each edge starts where the previous one ends, the last edge ends at `target`, and no vertex repeats within a path (it is simple).

Stage 1 solves the original problem. Stages 2 and 3 go further: a best-effort routing with a Menger certificate when not every source can be served, and per-vertex hub limits.

Write your code in `solution.hpp`. Each stage shows the declarations it adds. In that file, `edges` has type `EdgeList`, an alias for `std::span<const std::pair<int, int>>`, and Stages 2 and 3 below abbreviate some parameter lists.

Read one stage at a time.

## Stage 1 — k edge-disjoint paths

```cpp
std::optional<std::vector<std::vector<int>>> edge_disjoint_routes(
    int n, std::span<const std::pair<int,int>> edges, std::span<const int> sources, int target);
```

Return `routes`, where `routes[i]` is a path from `sources[i]` to `target` and no two paths share an edge index. Return `std::nullopt` when this is impossible.

## Stage 2 — Best effort and a Menger certificate

When not every source can be routed, return as many paths as possible, plus a certificate of why there can't be more (return a certificate even when every source is served: for example, every source in `free_sources` and no `cut_edges`):

```cpp
struct PartialRouting {
    std::vector<std::vector<int>> routes;   // an empty routes[i] means sources[i] is not served
    std::vector<int> cut_edges;             // edge indices, increasing
    std::vector<int> free_sources;          // indices into sources (0..k-1), increasing
};
PartialRouting best_effort_routes(int n, edges, sources, target);
```

The certificate `(cut_edges, free_sources)` must satisfy two conditions: **every source not in `free_sources`** can no longer reach `target` once `cut_edges` are removed, and the number of paths in `routes` `== cut_edges.size() + free_sources.size()`.

This is exactly Menger's theorem: the upper bound on the number of paths is met exactly by some cut.

## Stage 3 — Hub limits (real-world extension)

Some vertices are relay hubs with limited capacity. `hub_limit[v]` is **the maximum number of paths that may pass through v** (including paths that start or end there). `-1` means no limit.

```cpp
std::optional<std::vector<std::vector<int>>> routes_with_hub_limits(
    int n, edges, sources, target, std::span<const int> hub_limit);
```

The paths must still be edge-disjoint; return `std::nullopt` when this is impossible.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can an integral unit-capacity flow be split into paths? Where is the integrality argument needed?
2. Why do hub limits need node splitting? Why isn't it enough to put the capacity on the incoming and outgoing edges?
3. If the k paths had to be **vertex-disjoint** (with distinct sources/targets too), how would the algorithm change?

---

**Official solution:** [Solutions to Problem Set 7](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/a320dad5988c30172a2b1538536dc0a2_MIT6_046JS15_pset7sols.pdf). Read it after you finish.
