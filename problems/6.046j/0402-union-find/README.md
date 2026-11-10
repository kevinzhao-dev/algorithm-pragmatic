---
title: Union-Find (from linked lists to path compression)
unit: 04 · Amortization and Randomization (PS4)
source: 6.046J Recitation 3
pdf: materials/6.046j/recitations/rec03.pdf
read: materials/6.046j/recitations/rec03.pdf, materials/6.046j/lectures/lec05.pdf
kind: code
minutes: 100
---

# 0402 · Union-Find

**Based on:** [Amortization: Union-find](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d25d9d3ba96321326601c8f6dd073e60_MIT6_046JS15_Recitation3.pdf) · **Read first:** [Amortization: Union-find](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d25d9d3ba96321326601c8f6dd073e60_MIT6_046JS15_Recitation3.pdf), [Amortization: Amortized Analysis](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/13e2c7d165259712327af0af312a068e_MIT6_046JS15_lec05.pdf)

> **Why this comes up in practice:** merging accounts, Kruskal's minimum spanning tree, connected regions in an image. The only requirements are "merge two groups" and "ask whether two things are in the same group", yet once amortized, each operation is almost constant time, a result every engineer should verify by hand once.

## Your task

Elements are numbered from `0`. Throw `std::out_of_range` when given an element that doesn't exist.

You build two disjoint-set structures, weighted linked lists and a forest with both classic optimizations, measure their costs, and then use the forest to merge duplicate account records.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Weighted linked lists

```cpp
class ListDsu {
public:
    explicit ListDsu(std::size_t n);

    int find(int x) const;
    bool unite(int x, int y);
    std::size_t set_size(int x) const;
    std::vector<int> members(int x) const;

    // Cost model used by the tests: the number of representative pointers rewritten.
    std::uint64_t rep_updates() const;
};
```

`ListDsu(n)`: `n` singleton sets. Each set is a linked list, and each element stores its representative directly.

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `find(x)` | Return the representative | O(1) |
| `unite(x, y)` | Merge the two sets; return `false` if they are already the same set, otherwise `true` | O(size of the smaller set) |
| `set_size(x)`, `members(x)` | The size of the set, and all of its members (in any order) | O(1), O(size) |
| `rep_updates()` | The total number of representative pointers rewritten so far | O(1) |

**Always merge the smaller set into the larger one.** The tests check this with `rep_updates()`. Once all `n` elements are merged, the total must be `≤ n lg n`; when sets of equal size are merged in pairs, `≤ (n/2) lg n + n`; and when singletons join one growing set one at a time, in either argument order, `≤ n`. Otherwise it degrades to Θ(n²).

## Stage 2 — A forest of trees

```cpp
class ForestDsu {
public:
    explicit ForestDsu(std::size_t n = 0);

    int make_set();
    int find(int x);
    bool unite(int x, int y);
    bool connected(int x, int y);
    std::size_t set_size(int x);
    std::size_t count() const;
    std::size_t size() const;

    // Parent links from x to its root, without compressing anything.
    int depth(int x) const;

    // Cost model used by the tests: the number of parent links followed by find.
    std::uint64_t hops() const;
};
```

`ForestDsu`: `union by rank` plus `path compression`.

- `ForestDsu(n)`: create `n` elements up front; `make_set()` adds one element and returns its number.
- `find(x)`, `unite(x, y)` (with the same semantics as in Stage 1), `connected(x, y)`, `set_size(x)` (O(α)), `count()` (the current number of sets), and `size()` (the total number of elements).
- `depth(x)`: return the number of edges from `x` to its root, without compressing.
- `hops()`: the total number of steps `find` has taken along `parent` links (`depth` doesn't count).

You must do both optimizations. Without union by rank, the `depth` test notices the trees growing tall; without path compression, the `hops` test notices `find` taking too many steps.

## Stage 3 — Real-world extension: an account deduplication index

```cpp
// in class ForestDsu
std::vector<int> members(int x);

class DedupIndex {
public:
    int add_record(const std::vector<std::string>& identifiers);
    bool same_entity(int a, int b);
    std::size_t entity_count() const;
    std::vector<int> entity_members(int record);
};
```

First add `members(x)` to `ForestDsu`, in **O(size of the set)** and without an extra vector of members: give each element a circular `next` pointer, and when merging, swapping the two sides' `next` pointers joins the two rings into one.

`DedupIndex`: merging account records.

- `add_record(identifiers)`: add a record (which may have several emails and phone numbers) and return its number (`0, 1, 2, ...`). Records that share any identifier belong to the same entity, and this relation is transitive. `identifiers` may be empty and may contain duplicates.
- `same_entity(a, b)`, `entity_count()`.
- `entity_members(record)`: return the numbers of all records of that record's entity, **sorted in increasing order**.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does Stage 1's "merge small into large" guarantee that each element is rewritten at most `lg n` times? Explain it once each with the aggregate, accounting, and potential methods.
2. What is the amortized complexity with only path compression, and with only union by rank? With both, where does the inverse Ackermann function come from?
3. Does this data structure support "splitting"? If account merges had to be undoable, how would you change it?
