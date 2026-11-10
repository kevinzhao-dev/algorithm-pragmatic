---
title: DAG Path Counting and Residues (state augmentation)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 PS7 Problem 7-3
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec15.pdf
kind: code
minutes: 70
---

# 0713 · DAG Path Counting and Residues

**Original problem:** [Problem Set 7 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/3dff7a1ecbc241e9b91b48e69a1588d4_MIT6_006S20_ps7_questions.pdf) · **Read first:** [Lecture 15: Dynamic Programming, Part 1: SRBOT, Fib, DAGs, Bowling notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9eb3e9a51a7b5b60b0f67c2277f8b0ee_MIT6_006S20_lec15.pdf)

> **Why this comes up in practice:** static timing analysis of a digital circuit needs to know whether a timing path passes through an odd or an even number of inverters (whether the signal ends up inverted), and build graphs and workflow engines often need to count the paths that satisfy some condition. There can be exponentially many paths, so you must never list them, but there are only k possible "residues mod k", so just put the residue into the state.

## Original problem

6.006 PS7 Problem 7-3 gives a weighted directed acyclic graph G = (V, E, w) with integer edge weights and two vertices s, t ∈ V. It asks for a linear-time algorithm that counts how many paths from s to t have an odd total weight. You may assume that every integer the algorithm computes fits in a single machine word. As with every PS7 problem, the solution must be a dynamic program presented through the SRT BOT steps.

## Your task

The graph is a weighted directed graph, with its edges stored as adjacency lists:

```cpp
struct Edge { int to; int weight; };          // weight may be negative
using Graph = std::vector<std::vector<Edge>>; // graph[v] lists edges out of v
```

A path's weight is the sum of all its edges. Parallel edges count as different paths, and when `source == target`, the empty path of length 0 counts as one path (with weight 0). When the number of paths exceeds 2⁶⁴, count with the wrap-around semantics of `std::uint64_t` (just write the code normally).

Every function must satisfy these rules:

- If the graph has any cycle at all (including in parts that `source` can't reach), throw `std::invalid_argument`.
- If `source` or `target` is out of range, throw `std::out_of_range`.
- Vertex numbers are not guaranteed to be in topological order, and the total time must be linear (`|V| + |E|`, times the number of residues in the stages below).
- Do not use recursion: the tests include long chains of hundreds of thousands of vertices.

The original promises a DAG and asks only for the number of odd-weight paths. Here you must reject cycles yourself; you count all paths, then split the count by parity and by any modulus, and finally return one path with a given residue.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Number of paths

```cpp
std::uint64_t count_paths(const Graph& graph, int source, int target);
```

`std::uint64_t count_paths(const Graph& graph, int source, int target)`.

## Stage 2 — Parity

```cpp
struct ParityCounts { std::uint64_t even; std::uint64_t odd; };
ParityCounts count_paths_by_parity(const Graph& graph, int source, int target);
```

Weights can be negative, so be careful when deciding parity.

## Stage 3 — Any modulus

```cpp
std::vector<std::uint64_t> count_paths_by_residue(const Graph& graph, int source, int target,
                                                  int modulus);
```

`std::vector<std::uint64_t> count_paths_by_residue(const Graph& graph, int source, int target, int modulus)`: return a vector of size `modulus` whose entry `r` is the number of paths with weight congruent to `r` (modulo `modulus`, normalized to `[0, modulus)`). Throw `std::invalid_argument` when `modulus < 1`. O(modulus · (|V| + |E|)).

## Stage 4 — Real-world extension: finding a path that meets the condition

```cpp
std::optional<std::vector<int>> find_path_with_residue(const Graph& graph, int source,
                                                       int target, int modulus,
                                                       int residue);
```

`std::optional<std::vector<int>> find_path_with_residue(graph, source, target, modulus, residue)`: return any path whose weight is congruent to `residue` (as a sequence of vertices, including both ends), or `std::nullopt` if there is none. Error handling is the same as before.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Replacing the weight with its parity only doubles the number of states. When does this trick make the number of states explode? (For example, "the total is exactly W".)
2. Why does C++'s `%` get the parity wrong on negative numbers? Under what assumption is `x & 1` correct?
3. If you really need exact path counts beyond 64 bits, how do you redo the time analysis (the word-RAM assumption has changed)?

---

**Official solution:** [Problem Set 7 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ed996e1569867432dc1fbdeae53e309f_MIT6_006S20_ps7_solutions.pdf). Read it after you finish.
