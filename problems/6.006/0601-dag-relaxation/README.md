---
title: DAG Relaxation, Shortest-Path Trees, and Critical Path
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 Lecture 11, Recitation 11
pdf: materials/6.006/lectures/lec11.pdf
read: materials/6.006/recitations/rec11.pdf
kind: code
minutes: 100
---

# 0601 · DAG Relaxation, Shortest-Path Trees, and Critical Path

**Based on:** [Lecture 11: Weighted Shortest Paths notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/aa57a9785adf925bc85c1920f53755a0_MIT6_006S20_lec11.pdf) · **Read first:** [Recitation 11 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/77ff36ad54779575d9ba8d8687f91b5b_MIT6_006S20_r11.pdf)

> **Why this comes up in practice:** the total duration of a CI pipeline, the critical path of a project schedule, and the end-to-end latency of a data pipeline are all longest or shortest paths on a DAG. As long as the graph has no cycles, you need neither Bellman-Ford nor Dijkstra.

## Your task

Vertices are numbered `0 … n-1`. Throw `std::out_of_range` when the source doesn't exist; the same holds in every later stage.

```cpp
struct Arc { int to; std::int64_t weight; };
using WeightedGraph = std::vector<std::vector<Arc>>;  // weights may be negative
inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;
```

You compute shortest paths on a DAG with DAG Relaxation, rebuild a shortest-path tree from known distances on any graph, and then plan a CI pipeline and find its critical path.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Shortest paths on a DAG

```cpp
std::vector<std::int64_t> dag_shortest_paths(const WeightedGraph& g, int source);
```

`dag_shortest_paths(g, s)` returns `dist`: `dist[v]` is the shortest-path weight from `s` to `v`, or `kUnreachable` if `v` is unreachable. Weights may be negative. Throw `std::invalid_argument` if the graph has a directed cycle (wherever that cycle is).

Must run in O(V + E); there may be millions of vertices.

## Stage 2 — Recovering the shortest-path tree from distances

```cpp
std::vector<int> shortest_path_tree(const WeightedGraph& g, int source,
                                    std::span<const std::int64_t> dist);
```

`shortest_path_tree(g, s, dist)`: `dist` is known to hold the true shortest distances from `s` in an **arbitrary** graph (not necessarily a DAG, but with no negative cycle reachable from `s`). Return `parent`:

- For every reachable `v != s`, the edge `parent[v] → v` must exist and satisfy `dist[parent[v]] + w == dist[v]`, and following `parent` back from `v` must always lead to `s`.
- `s` and unreachable vertices get `kNoParent`.

Must run in O(V + E). The graph may contain cycles whose total weight is 0.

## Stage 3 — Real-world extension: CI Pipeline

```cpp
struct Job {
    std::int64_t duration;        // >= 0
    std::vector<int> depends_on;  // jobs that must finish before this one starts
};
struct PipelinePlan {
    std::int64_t makespan;
    std::vector<std::int64_t> earliest_start;
    std::vector<std::int64_t> slack;  // how long a job can slip without delaying the pipeline
    std::vector<int> critical_path;   // one longest chain of jobs, in execution order
};
```

```cpp
PipelinePlan plan_pipeline(std::span<const Job> jobs);
```

There is no limit on the number of machines, and a job starts as soon as it can. `plan_pipeline(jobs)` returns:

- `earliest_start[i]`: the earliest time job `i` can start; `makespan` is the time when every job has finished.
- `slack[i]`: how long job `i` can be delayed at most while the whole pipeline still finishes within `makespan`.
- `critical_path`: a chain of jobs in which each job depends on the one before it and starts right when that one ends. The first job starts at time 0, and the last one ends at `makespan`.

With no jobs, everything is empty and `makespan` is 0. Throw `std::invalid_argument` if the dependencies form a cycle, and `std::out_of_range` if `depends_on` names a job that doesn't exist.

Must run in O(V + E).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is it enough for DAG Relaxation to relax each edge once, in topological order? Explain it to the interviewer by induction.
2. In Stage 2, why can't each vertex just pick any incoming edge that satisfies the equation? What would a zero-weight cycle cause?
3. Longest path is NP-hard on general graphs, so why is it this easy on a DAG? Is a job whose `slack` is 0 always on the critical path?
