---
title: Release Planning (four constrained selection DPs)
unit: 05 · Skip Lists, Hashing, and DP (PS5)
source: 6.046J PS5 Problem 5-2
pdf: materials/6.046j/psets/ps05/questions.pdf
answer: materials/6.046j/psets/ps05/solutions.pdf
read: materials/6.046j/lectures/lec01.pdf, materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 180
---

# 0512 · Release Planning

**Original problem:** [Problem Set 5](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/45952953cb13d077bd35d4397d67fa7c_MIT6_046JS15_pset5.pdf) · **Read first:** [Overview, Interval Scheduling](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ff2a7015ff913fc01f0381d8f5126a9a_MIT6_046JS15_lec01.pdf), [Divide & Conquer: Convex Hull, Median Finding](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7463c413c944ed72b46a3c3d02b49448_MIT6_046JS15_lec02.pdf)

> **Why this comes up in practice:** a release can ship at most `m` changes, each with a different payoff, but rollout order and dependencies limit which combinations are feasible. Change a few constraints on the same budget problem, and the algorithm that applies is completely different.

## Original problem

6.046J PS5 Problem 5-2 ("Choosing Prizes") offers n prizes, of which you may take at most m, where m < n. Every prize p has a nonnegative integer value, p.value, and you want the chosen prizes to be worth as much as possible in total. Each part below is a variant: for each one, give an efficient algorithm and analyze how much time and space it needs. In parts (a)–(c) the prizes arrive as a sequence P = ⟨p1, p2, ..., pn⟩, and the answer must be a subsequence S of P, that is, the chosen prizes in their order in P.

- (a) Find a subsequence S = ⟨s1, s2, ...⟩ of P with at most m elements that maximizes Σ_j s_j.value, and analyze the algorithm in terms of n and m.
- (b) Each prize now also has a type, A or B, given by p.type. Maximize the same sum subject to one more rule: within S, every type-A prize comes before every type-B prize. Analyze it in terms of n and m.
- (c) Back to a single type, but now the values of the prizes in S must be non-decreasing. Maximize the same sum and analyze it in terms of n and m.
- (d) The prizes now hang on a rooted binary tree T with root r, one prize u.prize per vertex u, and each prize again has a nonnegative integer value. Find a set S of at most m prizes with the largest total value such that, whenever S contains the prize of a non-root vertex u, it also contains the prize at u.parent. In other words, the chosen vertices form a connected subtree of T that contains r.

## Your task

Each change's payoff is a non-negative integer; throw `std::invalid_argument` if any is negative. "Choose at most `m` items", where `m` may be 0 and may also exceed the number of items. Every function returns indices **sorted in increasing order**; when there are several optimal answers, return any one of them, as long as the total payoff is the maximum.

Stages 1 to 4 are parts (a) to (d), recast as release planning: prizes become changes, type A is a schema migration, and type B is a service launch. Unlike the original, you return the actual selection, and Stage 5 then tightens the running times of Stages 3 and 4.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — No constraints

```cpp
std::vector<int> pick_top(std::span<const std::int64_t> values, std::size_t m);
```

`pick_top(values, m)`: pick at most `m` items with the highest payoffs, in O(n) or O(n log m). Don't sort the whole array and then sort the result back into the original order.

## Stage 2 — Migrate first, launch later

```cpp
struct Prize {
    std::int64_t value;
    char type;
};

std::vector<int> pick_a_then_b(std::span<const Prize> prizes, std::size_t m);
```

`pick_a_then_b(prizes, m)`: `type` is only ever `'A'` (a schema migration) or `'B'` (a service launch); throw `std::invalid_argument` for any other value. Among the chosen items, every `A` must come before every `B` (in original index order). O(nm) time.

Split the subproblems into two states: still choosing only `A`s, or already allowed to choose `B`s. You return the actual selection, so record enough information to backtrack.

## Stage 3 — Impact can only grow

```cpp
std::vector<int> pick_non_decreasing(std::span<const std::int64_t> values, std::size_t m);
```

`pick_non_decreasing(values, m)`: the payoffs of the chosen items, in index order, must be **non-decreasing**. In this stage you only need to be correct, and any polynomial running time is accepted; the performance requirement comes in Stage 5.

## Stage 4 — Feature dependencies

```cpp
struct TreePrize {
    std::int64_t value;
    int left;
    int right;
};

std::vector<int> pick_rooted_subtree(std::span<const TreePrize> tree, int root, std::size_t m);
```

`pick_rooted_subtree(tree, root, m)`: `tree` is a binary tree in which `tree[u].left` and `right` are the indices of the children, or `-1` when a child is absent. For every node except `root`, choosing the node means you must also choose its parent; that is, the chosen nodes must form a connected subtree that contains `root` (or be empty).

An empty tree returns an empty result, and `root` is ignored in that case. Throw `std::invalid_argument` if the tree is non-empty and `root` is out of range. The input may be a single chain of 500,000 nodes, so you must not recurse.

## Stage 5 — Lowering the complexity

There is no new interface, only `PERF` tests:

- `pick_non_decreasing` must run in **O(nm log n)**. For a fixed length, "the best sum whose last value is at most `x`" is a prefix-maximum query.
- `pick_rooted_subtree` must run in **O(nm)**. Each node's table needs only `min(subtree_size, m) + 1` entries, and merging two children costs the product of the two tables' lengths.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why are two states enough in Stage 2? If the type order became `A < B < C`, or the types had to alternate, how would you extend the states?
2. Why do the tree merges in Stage 5 add up to only O(nm) rather than O(nm²)? (Think of each product as counting pairs of nodes.)
3. What is the space cost of the Stage 3 DP when it keeps backtracking information? How far can you cut it if you only need the total and not the selection?

---

**Official solution:** [Solutions to Problem Set 5](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/bbcc32bdd480116ca12b065cf6d6c454_MIT6_046JS15_pset5sols.pdf). Read it after you finish.
