---
title: Pencil-and-Paper Graph Problems as Code (representations, BFS/DFS traces, single-edge cycle fixes)
unit: 05 · Graph Search (PS5)
source: 6.006 PS5 Problem 5-1
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/lectures/lec10.pdf
kind: code
minutes: 60
---

# 0511 · Pencil-and-Paper Graph Problems as Code

**Original problem:** [Problem Set 5 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/8c155b28c87cc1fe3f506de2036bdf1b_MIT6_006S20_ps5_questions.pdf) · **Read first:** [Lecture 9: Breadth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/196a95604877d326c6586e60477b59d4_MIT6_006S20_lec9.pdf), [Lecture 10: Depth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/f3e349e0eb3288592289d2c81e0c4f4d_MIT6_006S20_lec10.pdf)

> **Why this comes up in practice:** reading a BFS/DFS traversal trace is a basic skill for debugging graph code, and "which single dependency can we remove to break the cycle?" is a question you really get asked when cleaning up a monorepo.

## Original problem

6.006 PS5 Problem 5-1 is a set of hand exercises on two small graphs.

- (a) An undirected graph on vertices 0–5 is given as an adjacency matrix `Adj`, with `Adj[u][v] = 1` exactly when u and v are adjacent. Its edges are {0, 2}, {1, 3}, {1, 4}, {1, 5}, {2, 3}, {2, 4} and {4, 5}. Draw it.
- (b) A drawing shows a directed graph on vertices A–F with the edges A→B, B→C, B→D, C→E, C→F, D→E, D→F, F→D and F→E. Give its adjacency-list form as a Python dictionary whose lists name each vertex's out-neighbors alphabetically.
- (c) From A, trace BFS and DFS on the (b) graph, taking out-neighbors alphabetically. Give the tree each search builds and the sequence in which it first reaches the vertices.
- (d) Deleting one edge of the (b) graph can leave a DAG. Find every edge whose deletion does this, and give a topological order of what remains in each case.

## Your task

The original problem is worked by hand; here every part becomes code. Every graph is a directed graph on `0 … n-1` with no parallel edges. In Stages 2 and 3 it is also simple, with no self-loops, but Stage 1 must accept self-loops. Stage 1 covers parts (a) and (b), Stage 2 part (c), and Stage 3 part (d), each for any input graph rather than the two fixed ones, and Stage 3 comes with a time bound.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Converting between representations

```cpp
using AdjacencyMatrix = std::vector<std::vector<int>>;
using AdjacencyList = std::vector<std::vector<int>>;

AdjacencyList to_adjacency_list(const AdjacencyMatrix& matrix);
AdjacencyMatrix to_adjacency_matrix(const AdjacencyList& list);
```

- `to_adjacency_list(matrix)`: `matrix[u][v] == 1` means there is an edge `u → v`. Sort each adjacency list in increasing order. Throw `std::invalid_argument` if the matrix is not `n × n` or holds a value other than 0 or 1.
- `to_adjacency_matrix(list)`: the reverse. Throw `std::invalid_argument` if a neighbor is out of range or repeated.

Self-loops are valid input here: `matrix[u][u] == 1` becomes `u` in the list of `u`, and the reverse, so converting a matrix to lists and back returns the same matrix.

## Stage 2 — Traversal traces

```cpp
inline constexpr int kNoParent = -1;

struct SearchTrace {
    std::vector<int> visit_order;
    std::vector<int> parent;
};

SearchTrace bfs_trace(const AdjacencyList& g, int source);
SearchTrace dfs_trace(const AdjacencyList& g, int source);
```

Both functions return `SearchTrace{visit_order, parent}`: `visit_order` lists the vertices in the order they are first visited, and `parent` holds each vertex's parent in the search tree (`kNoParent` for the source and for vertices that are never visited).

- `bfs_trace(g, s)`, `dfs_trace(g, s)`: **always process neighbors in increasing order**; the input adjacency lists are not guaranteed to be sorted. `dfs_trace` must match the recursive version exactly (go all the way down before backtracking).

Throw `std::out_of_range` if the source doesn't exist.

## Stage 3 — Breaking every cycle with one edge

```cpp
struct EdgeFix {
    int from;
    int to;
    std::vector<int> order;
};

std::vector<EdgeFix> single_edge_fixes(const AdjacencyList& g);
```

`single_edge_fixes(g)`: find **every** edge whose removal alone turns the whole graph into a DAG. Return a `std::vector<EdgeFix>` sorted by `(from, to)`; the `order` of each entry is a topological order of the graph once that edge is removed.

- The graph has no cycle to begin with: return an empty vector (there is nothing to fix).
- No single edge breaks every cycle: return an empty vector as well.

With few vertices and many edges, trying every edge in turn costs O(E · (V + E)). You must achieve **O(V · (V + E))**.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why doesn't Stage 3 need to try every edge? If you were looking for pairs of edges whose removal yields a DAG, would the same idea still work?
2. On what kind of graph are the `visit_order`s of BFS and DFS exactly the same?
3. When you make DFS iterative, why can't you mark a vertex visited at the moment you push it?

---

**Official solution:** [Problem Set 5 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/23d4cf5421c77bb55b3735f7e8f70dfe_MIT6_006S20_ps5_solutions.pdf). Read it after you finish.
