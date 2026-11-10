---
title: DFS, Topological Sort, and Cycle Detection
unit: 05 · Graph Search (PS5)
source: 6.006 Lecture 10, Recitation 10
pdf: materials/6.006/lectures/lec10.pdf
read: materials/6.006/recitations/rec10.pdf
kind: code
minutes: 100
---

# 0502 · DFS, Topological Sort, and Cycle Detection

**Based on:** [Lecture 10: Depth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/f3e349e0eb3288592289d2c81e0c4f4d_MIT6_006S20_lec10.pdf) · **Read first:** [Recitation 10 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/d9265b3b33d238ff914b0223ac8e7628_MIT6_006S20_r10.pdf)

> **Why this comes up in practice:** build systems, data pipelines, and migration ordering all need a topological order first. When there isn't one, the error message also has to spell out exactly which items form the loop.

## Your task

A graph is an `AdjacencyList` (`std::vector<std::vector<int>>`): `g[u]` holds the outgoing neighbors of `u`, and vertices are numbered `0 … n-1`.

**Nothing may be recursive:** a graph can be a single long chain of a million vertices. Throw `std::out_of_range` when the source vertex doesn't exist.

You build an iterative DFS that matches the recursive one exactly, use it for full traversals and connected components, then for a topological sort that produces a cycle when there is no order, and finally for incremental rebuilds in a build system.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — DFS

```cpp
using AdjacencyList = std::vector<std::vector<int>>;

inline constexpr int kNoParent = -1;

struct DfsForest {
    std::vector<int> parent;
    std::vector<int> finish_order;
};

DfsForest dfs(const AdjacencyList& g, int source);
```

`dfs(g, s)` returns `DfsForest{parent, finish_order}`:

- Try neighbors in the order of `g[u]`, and go down into a vertex the first time you reach it. **The result must be identical to the textbook recursive version.**
- `parent[v]` is the vertex that discovered `v`. It is `kNoParent` for `s` and for vertices the search never reaches.
- `finish_order` holds the vertices reachable from `s`, in the order their visits finish.

O(V + E).

## Stage 2 — Full-DFS and connected components

```cpp
DfsForest full_dfs(const AdjacencyList& g);
std::vector<int> connected_components(const AdjacencyList& g);
```

- `full_dfs(g)`: visit every vertex. Each vertex not reached yet becomes a new root, in increasing order of ID, and a root's `parent` is `kNoParent`.
- `connected_components(g)`: `g` is undirected (if `v` is in `g[u]`, then `u` is in `g[v]`). Return each vertex's component number. Numbers start at 0 and follow the order in which the smallest vertex of each component appears.

## Stage 3 — Topological sort, or produce a cycle

```cpp
struct TopologicalOrder {
    std::vector<int> vertices;
};

struct DirectedCycle {
    std::vector<int> vertices;
};

std::variant<TopologicalOrder, DirectedCycle> topological_sort(const AdjacencyList& g);
```

`topological_sort(g)` returns `std::variant<TopologicalOrder, DirectedCycle>`:

- No cycle: `TopologicalOrder.vertices` is a permutation of all the vertices in which the tail of every edge comes before its head.
- A cycle: `DirectedCycle.vertices` is a **simple** cycle. Every `vertices[i] → vertices[i+1]` is an edge of the graph, and the last vertex connects back to the first. A self-loop is a cycle of length 1.

O(V + E).

## Stage 4 — Real-world extension: incremental builds

```cpp
class CycleError : public std::runtime_error {
public:
    explicit CycleError(std::vector<int> cycle);         // provided
    const std::vector<int>& cycle() const noexcept;      // provided
};

class BuildGraph {
public:
    BuildGraph(int targets, std::span<const std::pair<int, int>> edges);

    std::vector<int> rebuild_order(std::span<const int> changed);
};
```

Build a `BuildGraph`, where an entry `{a, b}` in `edges` means `a` must be built before `b`:

- `BuildGraph(targets, edges)`: throw `CycleError` if the edges contain a cycle; its `cycle()` returns a cycle in the same format as Stage 3. Throw `std::out_of_range` if `edges` mentions a target that doesn't exist.
- `rebuild_order(changed)`: once the targets in `changed` have changed, the ones to rebuild are those targets plus everything downstream of them. Return a valid build order of these targets, with each one appearing exactly once. `changed` may contain duplicates and may be empty; throw `std::out_of_range` if it contains a target that doesn't exist.

The project has two million targets, and developers change files hundreds of times a day: **the cost of each query may only be proportional to the affected subgraph**. It must not scan the whole graph every time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the reverse finishing order a topological order? Which two cases does the proof split into?
2. Why does DFS on an undirected graph never produce forward edges or cross edges?
3. Compared with Kahn's algorithm (repeatedly removing a vertex with in-degree 0), what are the pros and cons of the DFS version? If several threads must be able to call `rebuild_order` at the same time, where should the structure that records visited vertices live?
