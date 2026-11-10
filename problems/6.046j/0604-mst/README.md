---
title: Minimum Spanning Tree (Kruskal, Prim, clustering)
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J Lecture 12
pdf: materials/6.046j/lectures/lec12.pdf
read: materials/6.046j/lectures/lec12.pdf
kind: code
minutes: 80
---

# 0604 · Minimum Spanning Tree

**Based on:** [Greedy Algorithms: Minimum Spanning Tree](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/4a7fdddff3bc419c70bb470106a1663a_MIT6_046JS15_lec12.pdf) · **Read first:** [Greedy Algorithms: Minimum Spanning Tree](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/4a7fdddff3bc419c70bb470106a1663a_MIT6_046JS15_lec12.pdf)

> **Why this comes up in practice:** data center cabling costs, image segmentation, single-linkage clustering, and any requirement like "build the cheapest backbone that connects scattered nodes" are all MSTs.

## Your task

The graph is undirected, with edges `WeightedEdge{u, v, w}`, and weights may repeat. You return indices into the input `edges`. The graph may be disconnected, in which case you compute a minimum spanning **forest**.

Reuse your Union-Find from 0402 (or write it again); it is not tested on its own.

You implement Kruskal and Prim from Lecture 12, then use them for single-linkage clustering.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Kruskal

```cpp
struct WeightedEdge {
    int u = 0;
    int v = 0;
    long long w = 0;
};

struct SpanningForest {
    long long total = 0;
    std::vector<int> edges;
};

SpanningForest kruskal(int n, std::span<const WeightedEdge> edges);
```

`SpanningForest kruskal(int n, std::span<const WeightedEdge> edges)`, where `SpanningForest{total, edges}`. O(E log E).

Edge cases: no edges, parallel edges, and a disconnected graph.

## Stage 2 — Prim

```cpp
SpanningForest prim(int n, std::span<const WeightedEdge> edges);
```

`SpanningForest prim(int n, std::span<const WeightedEdge> edges)`: the binary-heap version, O(E log V). If the graph is disconnected, run it once from each connected component that hasn't been visited yet. The resulting `total` must equal Kruskal's.

## Stage 3 — Real-world extension: single-linkage clustering

```cpp
std::vector<int> cluster(int n, std::span<const WeightedEdge> edges, int k);
```

`std::vector<int> cluster(int n, std::span<const WeightedEdge> edges, int k)`: split the vertices into exactly `k` clusters (`1 ≤ k ≤ n`, and the input is guaranteed to be connected) so that the lightest edge between two different clusters is as heavy as possible (maximum spacing). Return each vertex's cluster number in `0..k-1`, numbering the clusters in increasing order of their smallest vertex.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What does the cut property guarantee, and what does the cycle property guarantee? Which one does Prim rely on, and which one does Kruskal?
2. Is the MST still unique when weights repeat? How can you tell during a Kruskal run?
3. On a dense graph, how much difference does it make whether Prim uses a binary heap or a Fibonacci heap? Why is the latter rarely used in practice?
