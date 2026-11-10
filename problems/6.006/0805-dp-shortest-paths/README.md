---
title: A DP View of Shortest Paths (Bellman-Ford and Floyd-Warshall)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 Lecture 17
pdf: materials/6.006/lectures/lec17.pdf
read: materials/6.006/lectures/lec17.pdf
kind: code
minutes: 80
---

# 0805 · A DP View of Shortest Paths

**Based on:** [Lecture 17: Dynamic Programming, Part 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/665523227a175e9e9ce26ea8d3e5b51c_MIT6_006S20_lec17.pdf) · **Read first:** [Lecture 17: Dynamic Programming, Part 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/665523227a175e9e9ce26ea8d3e5b51c_MIT6_006S20_lec17.pdf)

> **Why this comes up in practice:** "the cheapest flight route with at most k connections" and the distance vectors in routing tables are both Bellman-Ford's DP table. Floyd-Warshall gets all-pairs shortest paths on small, dense graphs in a few lines of code, but handling negative cycles and overflow is what's really being tested.

## Your task

Edges are given as `Edge{from, to, weight}` (`weight` may be negative, and there may be parallel edges), and the vertices are `0..vertices−1`.

```cpp
inline constexpr std::int64_t kUnreachable  = std::numeric_limits<std::int64_t>::max();
inline constexpr std::int64_t kMinusInfinity = std::numeric_limits<std::int64_t>::min();
```

Lecture 17 recasts Bellman-Ford and Floyd-Warshall as dynamic programs. Here you implement both, recover the paths, and handle negative cycles without overflowing.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — At most k edges

```cpp
struct Edge {
    int from;
    int to;
    std::int64_t weight;
};

std::vector<std::int64_t> shortest_within_edges(int vertices, std::span<const Edge> edges,
                                                int source, int max_edges);
```

`std::vector<std::int64_t> shortest_within_edges(int vertices, std::span<const Edge> edges, int source, int max_edges)`: `result[v]` is the minimum weight of going from `source` to `v` using **at most `max_edges` edges** (a walk, which may use the same edge more than once); it is `kUnreachable` when `v` can't be reached that way. O(max_edges · |E|) time.

Each round may depend only on the results of the previous round; otherwise a single round could take several edges.

## Stage 2 — All-pairs shortest paths

```cpp
class AllPairs {
public:
    AllPairs(int vertices, std::span<const Edge> edges);   // O(V^3)
    std::int64_t distance(int from, int to) const;
    std::vector<int> path(int from, int to) const;         // Stage 3
    bool has_negative_cycle() const;                       // Stage 4
};
```

This stage assumes there are no negative cycles. `distance(v, v) == 0`, and unreachable pairs give `kUnreachable`. O(V³) time, O(V²) memory. Be careful never to add anything to `kUnreachable`.

## Stage 3 — Paths

`path(from, to)`: return every vertex on one shortest path, including both ends; it is `{from}` when `from == to`, and an empty vector when `to` can't be reached. The graph may contain zero-weight cycles, but the path must not go around one, so it has at most `V` vertices.

## Stage 4 — Negative cycles

The graph may now contain negative cycles.

- `has_negative_cycle()`: return `true` if there is a negative cycle anywhere in the graph.
- `distance(from, to)`: if walks from `from` to `to` can be made arbitrarily cheap (because they can go around a negative cycle on the way), return `kMinusInfinity`; otherwise return the correct shortest distance or `kUnreachable`, unaffected by negative cycles elsewhere.
- `path` returns an empty vector when `distance` is `kMinusInfinity`.
- Don't overflow: a negative cycle can make distances double downward every round, so remember to clamp them.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why must the rolling array in Stage 1 be copied, while the original Bellman-Ford can update in place?
2. Why is Floyd-Warshall still correct after dropping the third dimension (`k`)? Where does this argument break down when there are negative cycles?
3. When should you use Johnson's algorithm instead of Floyd-Warshall?
