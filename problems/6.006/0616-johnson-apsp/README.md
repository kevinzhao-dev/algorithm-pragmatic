---
title: Johnson's Algorithm (all-pairs shortest paths and routing tables)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 PS6 Problem 6-6, Lecture 14
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec14.pdf, materials/6.006/recitations/rec14.pdf
kind: code
minutes: 110
---

# 0616 · Johnson's Algorithm

**Original problem:** [Problem Set 6 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0cb5c705752261af9da744c22925a136_MIT6_006S20_ps6_questions.pdf) · **Read first:** [Lecture 14: ASP & Johnson’s Algorithm notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/7d7d5c35490f41b7b037cafbda7019ad_MIT6_006S20_lec14.pdf), [Recitation 14 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6027e2a155c948937fdfb93981f7070c_MIT6_006S20_r14.pdf)

> **Why this comes up in practice:** a router's forwarding table, traffic engineering between data centers, and a map service's distance cache all need answers for any pair of points. Johnson turns a sparse graph with negative weights into V runs of Dijkstra, which saves a great deal over the V³ of Floyd-Warshall.

## Original problem

6.006 PS6 Problem 6-6 is a coding task: Johnson's algorithm from Lecture 14, computing shortest-path weights between all pairs of vertices of an arbitrary weighted directed graph G. It receives n, the number of vertices (labeled 0 through n − 1), plus a tuple S of edges given as triples (u, v, w) for u → v with weight w. It returns an n × n table D, a tuple of n tuples, with D[u][v] = δ(u, v), or None when the graph has a cycle of negative total weight.

Implement it as `johnson(n, S)` in the Python template, which already ships Bellman-Ford and a binary-heap Dijkstra adapted from recitation; reusing them means building your own adjacency lists and weight function.

## Your task

The input is a list of edges over `n` vertices (`0 … n-1`). Weights may be negative, and there may be parallel edges and self-loops. Both ends of every edge are guaranteed to be valid.

```cpp
struct Edge { int from; int to; std::int64_t weight; };
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
```

Stage 1 computes the potentials, Stage 2 is the original problem, and Stage 3 goes further and builds a routing table with next hops on top of it.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Potential

```cpp
std::optional<std::vector<std::int64_t>> johnson_potentials(int n, std::span<const Edge> edges);
```

`johnson_potentials(n, edges)`: return an `h` of length `n` such that **every edge** satisfies `weight + h[from] - h[to] >= 0`. Return `std::nullopt` if there is a negative cycle anywhere in the graph (no matter where it can be reached from).

Must run in O(V · E).

## Stage 2 — All-pairs shortest paths

```cpp
std::optional<std::vector<std::vector<std::int64_t>>> johnson(int n, std::span<const Edge> edges);
```

`johnson(n, edges)`: return an `n × n` matrix `D` in which `D[u][v]` is the shortest-path weight from `u` to `v`, or `kUnreachable` if `v` can't be reached, and `D[u][u]` is 0. Return `std::nullopt` if there is a negative cycle.

Must run in O(V² log V + V · E). On sparse graphs it must be clearly faster than O(V³) and than V runs of Bellman-Ford.

## Stage 3 — Real-world extension: a routing table

```cpp
class RoutingTable {
public:
    static std::optional<RoutingTable> build(int n, std::span<const Edge> edges);

    std::int64_t distance(int from, int to) const;
    std::optional<int> next_hop(int from, int to) const;
    std::optional<std::vector<int>> route(int from, int to) const;
};
```

Build a `RoutingTable`:

- `static std::optional<RoutingTable> build(n, edges)`: return `std::nullopt` if there is a negative cycle.
- `distance(from, to)`: the same as Stage 2, in O(1).
- `next_hop(from, to)`: the next vertex on the way from `from` to `to`; return `std::nullopt` if `from == to` or `to` can't be reached. O(1).
- `route(from, to)`: the full route you get by following `next_hop` one hop at a time (both ends included); return `std::nullopt` if `to` can't be reached, and a single vertex when `from == to`.

Throw `std::out_of_range` if a vertex doesn't exist.

A router only knows its own next hop, so **following next hops one at a time from any vertex must actually arrive at `to`, and the path's total weight must equal `distance(from, to)`**. The graph may contain cycles whose total weight is 0.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does the `h` computed with an added super node always make every weight nonnegative? Why can't you just add the minimum weight to every edge?
2. Why does reweighting leave every shortest path unchanged? How do you convert `D'` back into `D`?
3. When there are zero-weight cycles, what can go wrong with the next hops if every vertex independently picks a shortest path of its own? What do you rely on to avoid that?

---

**Official solution:** [Problem Set 6 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9073af3b8151002c5f63d06dc7ac961e_MIT6_006S20_ps6_solutions.pdf). Read it after you finish.
