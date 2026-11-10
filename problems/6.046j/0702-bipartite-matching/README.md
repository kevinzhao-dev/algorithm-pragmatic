---
title: Bipartite Matching (König's theorem and Hall certificates)
unit: 07 · Network Flow (PS7)
source: 6.046J Lecture 14, Recitation 7
pdf: materials/6.046j/lectures/lec14a.pdf
read: materials/6.046j/lectures/lec14a.pdf, materials/6.046j/recitations/rec07.pdf
kind: code
minutes: 110
---

# 0702 · Bipartite Matching

**Based on:** [Incremental Improvement: Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bdafe05f4f3c20cabfd08d2fbd099f63_MIT6_046JS15_lec14A.pdf) · **Read first:** [Incremental Improvement: Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bdafe05f4f3c20cabfd08d2fbd099f63_MIT6_046JS15_lec14A.pdf), [Incremental Improvement: Applications of Network Flow & Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ba74f89434fc261d9366ebf136c04b52_MIT6_046JS15_Recitation7.pdf)

> **Why this comes up in practice:** assigning jobs to machines, matching ads to ad slots, and scheduling Pods onto nodes. When not everything can be placed, a real system has to answer "why not?", and that is what a set violating Hall's condition is for.

## Your task

Left vertices are `0..left-1`, right vertices are `0..right-1`, and edges are `{l, r}`. The input is a `std::span<const std::pair<int,int>>` and may contain duplicate edges.

You compute maximum matchings two ways (any augmenting-path method, then Hopcroft-Karp) and produce the two certificates from Lecture 14 and Recitation 7: a minimum vertex cover (König) and a left-side set that violates Hall's condition.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Maximum matching

```cpp
struct Matching {
    int size = 0;
    std::vector<int> match_left;   // match_left[l]  = the matched right vertex, or -1
    std::vector<int> match_right;  // match_right[r] = the matched left vertex, or -1
};
Matching max_matching(int left, int right, std::span<const std::pair<int,int>> edges);
```

O(VE) is enough (augmenting paths or flow both work).

## Stage 2 — König: minimum vertex cover

```cpp
struct VertexCover {
    std::vector<int> left;
    std::vector<int> right;
};
VertexCover min_vertex_cover(int left, int right,
                             std::span<const std::pair<int, int>> edges);
```

Recitation 7 reduces bipartite vertex cover to min cut. Implement `VertexCover min_vertex_cover(int left, int right, edges)`, which returns `VertexCover{left, right}`: the chosen left and right vertices (in increasing order), which cover every edge and have the smallest possible total size.

König's theorem guarantees that this size equals the size of a maximum matching.

## Stage 3 — Hopcroft-Karp

```cpp
Matching hopcroft_karp(int left, int right, std::span<const std::pair<int, int>> edges);
```

`Matching hopcroft_karp(int left, int right, edges)`: O(E√V). Each phase first builds layers with BFS, then uses DFS along the layers to find a maximal set of vertex-disjoint augmenting paths.

## Stage 4 — Real-world extension: Hall violators

```cpp
std::vector<int> hall_violator(int left, int right,
                               std::span<const std::pair<int, int>> edges);
```

`std::vector<int> hall_violator(int left, int right, edges)`: if some matching covers every left vertex, return an empty vector. Otherwise return a subset `S` of the left side (nonempty, in increasing order) with `|N(S)| < |S|` (fewer neighbors than members).

The tests pin down which subset: take a maximum matching and return exactly the left vertices that can be reached from an unmatched left vertex along an alternating path (for edges `{0,0}`, `{1,0}`, `{2,1}` that is `{0, 1}`, not `{0, 1, 2}`).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. How do you read König's vertex cover off a min cut of the flow network? (Why are `S ∩ R` and `T ∩ L` the right sets?)
2. Why does Hopcroft-Karp need only O(√V) phases?
3. If you wanted a maximum-weight matching instead, which of these methods would still work? (Hungarian / min-cost flow)
