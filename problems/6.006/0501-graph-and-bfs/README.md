---
title: Graph Representation and BFS
unit: 05 · Graph Search (PS5)
source: 6.006 Lecture 9, Recitation 9
pdf: materials/6.006/lectures/lec09.pdf
read: materials/6.006/recitations/rec09.pdf
kind: code
minutes: 90
---

# 0501 · Graph Representation and BFS

**Based on:** [Lecture 9: Breadth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/196a95604877d326c6586e60477b59d4_MIT6_006S20_lec9.pdf) · **Read first:** [Recitation 9 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6afd7e9a85d7d36e204533569f9fccf6_MIT6_006S20_r09.pdf)

> **Why this comes up in practice:** the blast radius in a service dependency graph, degrees of separation in a social network, and a web crawler's fetch order are all BFS on an unweighted graph. Later exercises keep running it on ever more abstract graphs, so get to the point where you can write it without thinking.

## Your task

Vertices are always numbered `0 … n-1`. You build an adjacency-list graph and BFS on top of it, then two variants of BFS: the shortest walk with an odd number of edges, and a search from both ends at once.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Adjacency list

```cpp
class Graph {
public:
    explicit Graph(int vertices);

    int vertex_count() const;
    std::size_t edge_count() const;

    void add_directed(int from, int to);
    void add_undirected(int a, int b);

    std::span<const int> neighbors(int u) const;
    int out_degree(int u) const;
    int in_degree(int v) const;
};
```

Implement `Graph`.

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `Graph(n)` | `n` vertices and no edges; throw `std::invalid_argument` if `n < 0` | O(n) |
| `vertex_count()` / `edge_count()` | Each successful call to an `add_*` function counts as one edge; a call that throws adds nothing | O(1) |
| `add_directed(u, v)` | Add `u → v` | O(1) on average |
| `add_undirected(a, b)` | Add one entry to the adjacency list of each end; only one entry when `a == b` | O(1) on average |
| `neighbors(u)` | Return a `std::span<const int>` in insertion order | O(1) |
| `out_degree(u)` / `in_degree(v)` | The number of entries in the adjacency list / the number of entries that point to `v` | O(1) |

No deduplication: store parallel edges and self-loops as they come. Every function throws `std::out_of_range` when it gets a vertex that doesn't exist (in later stages too).

## Stage 2 — BFS and the shortest-path tree

```cpp
inline constexpr int kNoVertex = -1;

struct BfsTree {
    std::vector<int> dist;
    std::vector<int> parent;
};

BfsTree bfs(const Graph& graph, int source);

std::optional<std::vector<int>> shortest_path(const Graph& graph, int source, int target);

std::vector<std::vector<int>> level_sets(const Graph& graph, int source);
```

- `bfs(g, s)` returns `{dist, parent}`. `dist[v]` is the fewest edges from `s` to `v`, or `-1` if `v` is unreachable. `parent[v]` is the vertex before `v` on some shortest path; it is `kNoVertex` for `s` and for unreachable vertices.
- `shortest_path(g, s, t)`: return the sequence of vertices from `s` to `t` (both ends included, so just `s` when `s == t`), or `std::nullopt` if `t` is unreachable.
- `level_sets(g, s)`: `result[i]` holds every vertex at distance exactly `i`, in any order, with no empty level left at the end.

All of them must run in O(V + E).

## Stage 3 — Odd-length paths

```cpp
std::optional<std::vector<int>> shortest_odd_walk(const Graph& graph, int source,
                                                  int target);
```

`shortest_odd_walk(g, s, t)`: return the shortest walk from `s` to `t` with an **odd number of edges**, as the sequence of vertices it passes through with both ends included (it may revisit vertices, and when `s == t` it must take at least one step), or `std::nullopt` if none exists.

Must run in O(V + E).

## Stage 4 — Real-world extension: degrees of separation

```cpp
std::optional<int> separation(const Graph& graph, int a, int b);
```

`separation(g, a, b)`: `g` is an undirected graph. Return the fewest edges between `a` and `b`, or `std::nullopt` if they aren't connected.

A one-directional BFS is too slow here: the graph has a million vertices, but two users are usually only a few levels apart. Search outward from both ends at once and stop as soon as the frontiers touch. The cost must be proportional to the region that the two searches actually cover, not to the whole graph.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When the number of edges approaches V², where does an adjacency matrix win, and where does an adjacency list win? When does the `span` returned by `neighbors` become invalid?
2. Why does BFS guarantee shortest distances? If every edge weighs either 0 or 1, how would you change it?
3. When the two frontiers of a bidirectional BFS first meet, is the answer always the shortest? What does your implementation rely on to guarantee it? Is its worst case any better than a one-directional BFS?
