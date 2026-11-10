---
title: Changeable Priority Queue and Dijkstra
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 Lecture 13, Recitation 13
pdf: materials/6.006/lectures/lec13.pdf
read: materials/6.006/recitations/rec13.pdf
kind: code
minutes: 120
---

# 0603 · Changeable Priority Queue and Dijkstra

**Based on:** [Lecture 13: Dijkstra’s Algorithm notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/d819e7f4568aced8d5b59e03db6c7b67_MIT6_006S20_lec13.pdf) · **Read first:** [Recitation 13 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ec187276db0faaa047d655979a44de7f_MIT6_006S20_r13.pdf)

> **Why this comes up in practice:** map navigation, network routing (OSPF), and game pathfinding are all built on Dijkstra, and its performance depends almost entirely on the priority queue. A real online service also cares about one more thing: a query must not reset the state of the whole graph.

## Your task

You build an indexed min-heap whose keys can decrease, use it to implement Dijkstra, and then turn Dijkstra into a query engine whose cost per query depends only on what the query explores.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — A priority queue whose keys can change

```cpp
template <typename Key>
class IndexedMinHeap {
public:
    explicit IndexedMinHeap(int capacity);

    bool empty() const;
    std::size_t size() const;
    bool contains(int id) const;
    const Key& key(int id) const;

    void push(int id, Key key);
    std::pair<int, Key> pop_min();
    bool decrease_key(int id, Key key);
    void clear();
};
```

Implement `IndexedMinHeap<Key>`. Each item in it is identified by an `id` in `0 … capacity-1`, and an `id` can be present at most once at a time. `Key` is only guaranteed to have `operator<` and to be movable and copyable.

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `IndexedMinHeap(capacity)` | Throw `std::invalid_argument` if `capacity < 0` | O(capacity) |
| `empty()` / `size()` | | O(1) |
| `contains(id)` | Throw `std::out_of_range` if `id` is invalid | O(1) |
| `key(id)` | Return the current key; throw `std::out_of_range` if the item isn't in the heap | O(1) |
| `push(id, key)` | Throw `std::invalid_argument` if the item is already present, and `std::out_of_range` if `id` is invalid | O(log n) |
| `pop_min()` | Remove and return the `{id, key}` with the smallest key; throw `std::out_of_range` if the heap is empty | O(log n) |
| `decrease_key(id, key)` | Update and return `true` only if the new key is smaller than the current one; otherwise change nothing and return `false`. Throw `std::out_of_range` if `id` isn't in the heap | O(log n) |
| `clear()` | Remove everything | **O(current number of elements)**, not O(capacity) |

## Stage 2 — Dijkstra

```cpp
struct Arc { int to; std::int64_t weight; };
using WeightedGraph = std::vector<std::vector<Arc>>;
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;
```

```cpp
struct ShortestPaths {
    std::vector<std::int64_t> dist;
    std::vector<int> parent;
};

ShortestPaths dijkstra(const WeightedGraph& g, int source);
```

`dijkstra(g, s)` returns `ShortestPaths{dist, parent}`: unreachable vertices get `kUnreachable`, and `parent` is the shortest-path tree, with `kNoParent` for `s` and for unreachable vertices. Throw `std::out_of_range` if the source doesn't exist, and `std::invalid_argument` if any edge in the graph has a negative weight.

Use your `IndexedMinHeap`. On sparse graphs it must run in O(E log V).

## Stage 3 — Real-world extension: a query engine

```cpp
class RouteEngine {
public:
    explicit RouteEngine(WeightedGraph graph);  // takes ownership; negative weight throws invalid_argument
    std::optional<std::int64_t> distance(int from, int to);
    std::optional<std::vector<int>> route(int from, int to);  // the vertices from `from` to `to`
};
```

There are many queries, and most origin and destination pairs are close to each other. Return `std::nullopt` if the destination can't be reached; throw `std::out_of_range` if a vertex doesn't exist.

**The cost of each query may depend only on the number of vertices it actually explores:** stop as soon as you reach the destination, and don't sweep the whole graph to reset state either (a million vertices, hundreds of thousands of queries).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Exactly which step of Dijkstra breaks when it meets a negative edge? Can you give a minimal counterexample?
2. When is an Array (O(V²)) better than a binary heap? When is lazy deletion better than `decrease_key`?
3. How would you design the internal scratch state of `RouteEngine` so that several threads can query it at the same time?
