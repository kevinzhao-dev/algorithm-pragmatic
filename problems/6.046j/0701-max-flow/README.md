---
title: Maximum Flow (Ford-Fulkerson, Edmonds-Karp, min cut)
unit: 07 · Network Flow (PS7)
source: 6.046J Lecture 13, Lecture 14
pdf: materials/6.046j/lectures/lec13.pdf
read: materials/6.046j/lectures/lec13.pdf, materials/6.046j/lectures/lec14a.pdf, materials/6.046j/recitations/rec07.pdf
kind: code
minutes: 120
---

# 0701 · Maximum Flow

**Based on:** [Incremental Improvement: Max Flow, Min Cut](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ac6b6e8ac932bb5dfb6506fd0db9d5ad_MIT6_046JS15_lec13.pdf) · **Read first:** [Incremental Improvement: Max Flow, Min Cut](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ac6b6e8ac932bb5dfb6506fd0db9d5ad_MIT6_046JS15_lec13.pdf), [Incremental Improvement: Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bdafe05f4f3c20cabfd08d2fbd099f63_MIT6_046JS15_lec14A.pdf), [Incremental Improvement: Applications of Network Flow & Matching](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ba74f89434fc261d9366ebf136c04b52_MIT6_046JS15_Recitation7.pdf)

> **Why this comes up in practice:** available bandwidth on a backbone network, replica migration in a data center, and any capacity planning that moves some amount from A to B. The bottleneck is usually a min cut: it tells you exactly which links to upgrade, instead of spending money on a hunch.

## Your task

The graph is directed, with vertices `0..n-1` and nonnegative integer capacities. Parallel edges and reverse edges are allowed (`u→v` and `v→u` may both exist). Edges with `u == v` never appear.

You build one `FlowNetwork` class step by step, following Lectures 13 and 14: Ford-Fulkerson, then Edmonds-Karp, then a min-cut certificate and a path decomposition of the flow.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Residual graph and Ford-Fulkerson

```cpp
class FlowNetwork {
public:
    explicit FlowNetwork(int n);
    int add_edge(int u, int v, long long capacity);   // returns the edge id, increasing from 0
    long long max_flow_dfs(int s, int t);             // finds augmenting paths with DFS
    long long flow_on(int id) const;                  // flow on that edge after the last computation
};
```

- Every call to `max_flow_dfs` recomputes from scratch.
- Throw `std::invalid_argument` when `s == t`.
- The flow must satisfy the capacity constraints and flow conservation; return 0 when `t` is unreachable.

## Stage 2 — Edmonds-Karp

```cpp
// in class FlowNetwork
long long max_flow_bfs(int s, int t);
int augmentations() const;
```

Add `long long max_flow_bfs(int s, int t)`: always augment along a shortest augmenting path (the one with the fewest edges), O(VE²). Add `int augmentations() const`: return how many augmenting paths the last computation used.

Like `max_flow_dfs`, `max_flow_bfs` recomputes from scratch on every call (the tests run both on the same network). The tests check that `augmentations()` after `max_flow_bfs` is at most V·E, where E counts every `add_edge` call.

## Stage 3 — A min-cut certificate

```cpp
// in class FlowNetwork
std::vector<bool> min_cut_side() const;
std::vector<int> cut_edges() const;
```

After computing a max flow:

- `std::vector<bool> min_cut_side() const`: the set `S` (containing `s` but not `t`); the cut's capacity must equal the flow value.
- `std::vector<int> cut_edges() const`: the ids of all edges with `u∈S, v∉S`, in increasing order.

## Stage 4 — Real-world extension: decomposing a flow into a list of paths

```cpp
// in class FlowNetwork
struct PathFlow {
    std::vector<int> edges;
    long long amount = 0;
};

std::vector<PathFlow> decompose() const;
```

A router wants to know "which links this LSP uses and how much it carries". `std::vector<PathFlow> decompose() const` (`PathFlow{edges, amount}`): split the current flow into simple `s→t` paths, where `edges` is a sequence of edge ids and `amount > 0`. The amounts add up to the flow value, no edge is assigned more than its `flow_on`, and there are no more paths than edges. Each path takes the full bottleneck of the flow still unassigned along it, so a flow that uses a single route comes back as a single path.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why may Ford-Fulkerson fail to terminate with irrational capacities, yet always terminate with integer capacities? Which monotonicity property does Edmonds-Karp's O(VE) bound on augmentations rely on?
2. What problems does a real system run into when high-capacity edges keep getting split into small flows? Where does Dinic gain over Edmonds-Karp?
3. Why can max-flow min-cut serve as a "proof of optimality"? Given someone else's max flow, how do you verify in O(E) that it really is maximum?
