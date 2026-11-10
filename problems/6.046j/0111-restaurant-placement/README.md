---
title: Tree Placement (counterexample, tree DP, greedy, NP-hard)
unit: 01 · Scheduling and Divide & Conquer (PS1)
source: 6.046J PS1 Problem 1-1
pdf: materials/6.046j/psets/ps01/questions.pdf
answer: materials/6.046j/psets/ps01/solutions.pdf
read: materials/6.046j/lectures/lec01.pdf
kind: code
minutes: 120
---

# 0111 · Tree Placement

**Original problem:** [Problem Set 1](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/43d974c6c4bfc5d88086a67e023fd88d_MIT6_046JS15_pset1.pdf) · **Read first:** [Overview, Interval Scheduling](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ff2a7015ff913fc01f0381d8f5126a9a_MIT6_046JS15_lec01.pdf)

> **Why this comes up in practice:** you deploy a service on a tree formed by a network or an org hierarchy. Adjacent nodes share a failure domain, so they can't both host it. Each position has a different payoff, so which positions should you pick? The same structure also shows up in scheduling exclusions and in deduplicating candidates.

## Original problem

6.046J PS1 Problem 1-1 ("Restaurant Location") is about a restaurant chain that wants to open branches on street corners. The street network is an undirected graph G = (V, E) whose vertices are the candidate sites, and every vertex u carries a nonnegative integer profit p_u. Two branches may never sit on adjacent vertices, since they would compete with each other. You must output a set U ⊆ V of sites that maximizes the total profit Σ_{u∈U} p_u. Parts (a)–(c) assume that G is a tree.

- (a) A "greedy" placement repeatedly takes the remaining vertex with the highest profit (breaking ties by some fixed order on vertex names), adds it to U, and discards it together with its neighbors, until no vertex is left. Find a counterexample on which this greedy misses the maximum profit.
- (b) Design an efficient algorithm that finds a placement with maximum profit.
- (c) If all sites are worth the same, maximizing profit means opening as many branches as possible. Design a simple greedy algorithm for that case and prove it correct.
- (d) Now G may be any graph, not necessarily acyclic. Solve the problem correctly, as fast as you can manage.

Each answer is a written algorithm with a correctness argument and a running-time analysis.

## Your task

`Network` is an undirected graph with vertices numbered `0..n-1`; `value[v] ≥ 0` is a vertex's payoff, and `edges` lists every edge. **Two adjacent vertices must not both be chosen.** Every function returns vertex numbers **sorted in increasing order**; when there are several optimal answers, return any one of them.

Each stage turns one part, (a) through (d), into code: you return the counterexample from a function, and the tests check optimality instead of a written proof. Unlike the original, part (d) is limited to graphs with at most 40 vertices.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Why greedy is wrong

```cpp
struct Network {
    std::vector<std::int64_t> value;
    std::vector<std::pair<int, int>> edges;

    std::size_t size() const { return value.size(); }  // provided
};

std::vector<int> greedy_by_value(const Network& network);
Network greedy_counterexample();
```

- `greedy_by_value(network)`: repeatedly pick the vertex with the highest payoff (on ties, the one with the lower number), and remove it and all of its neighbors, until no vertices remain. It must work on any graph, in O((n+m) log n).
- `greedy_counterexample()`: return a **tree with at most 12 vertices** on which the greedy above **does not** reach the maximum payoff.

## Stage 2 — Tree DP

```cpp
std::vector<int> max_value_placement(const Network& tree);
```

`max_value_placement(tree)`: `tree` is guaranteed to be a tree (the empty graph counts as one). Return a valid selection with the maximum total payoff, in **O(n)**.

You choose the root. The input may be a single path of one million vertices, which would overflow the stack if you recurse, so use iteration.

## Stage 3 — Counting only

```cpp
std::vector<int> max_count_placement(const Network& tree);
```

`max_count_placement(tree)`: every position has the same payoff, so you only want the **largest number** of vertices. Do not use `value`; use an O(n) greedy rather than reusing Stage 2.

The input sizes are the same as in Stage 2.

## Stage 4 — Arbitrary graphs

```cpp
std::vector<int> max_value_placement_general(const Network& network);
```

`max_value_placement_general(network)`: the graph need not be a tree, and `n ≤ 40`. Return a valid selection with the maximum payoff.

On general graphs this is the NP-hard maximum weight independent set problem, so don't look for a polynomial-time solution. Enumerating all 2^n subsets won't finish within the time limit, so find a more efficient way to search.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What is the state of the tree DP? When you reconstruct the answer, why must you work down from the root instead of deciding each vertex independently?
2. Why is the Stage 3 greedy correct? (Hint: argue that some optimal solution contains every leaf.) Why does it break once there are weights?
3. If the general-graph problem had to be solved "well enough" at `n = 10^5`, what would you do, and how would you judge the quality of the answer?

---

**Official solution:** [Solutions to Problem Set 1](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/b14791a78432996a1d403e8b1deb0749_MIT6_046JS15_pset1sols.pdf). Read it after you finish.
