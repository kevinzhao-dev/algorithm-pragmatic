---
title: van Emde Boas Layout (cache-oblivious search)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J Lecture 24
pdf: materials/6.046j/lectures/lec24.pdf
read: materials/6.046j/lectures/lec23.pdf, materials/6.046j/lectures/lec24.pdf
kind: perf
minutes: 110
---

# 1021 · van Emde Boas Layout

**Based on:** [Cache-oblivious Algorithms: Searching & Sorting](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/1698f473542d755c89dfe9fce490c12a_MIT6_046JS15_lec24.pdf) · **Read first:** [Cache-oblivious Algorithms: Medians & Matrices](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/ed590b1c1f41cdef211fa49dc62dc8f1_MIT6_046JS15_lec23.pdf), [Cache-oblivious Algorithms: Searching & Sorting](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/1698f473542d755c89dfe9fce490c12a_MIT6_046JS15_lec24.pdf)

> **Why this comes up in practice:** for a static index (SSTables in a log-structured merge tree, a read-only dictionary, a routing table), the cost of a query isn't the number of comparisons but the number of cache misses. A B-tree has to know the block size B; the vEB layout reaches O(log_B n) without knowing B. That is the magic of cache-oblivious design (the same topic as in 6.172).

## Your task

Keys are `long long`, and the input `sorted_keys` is strictly increasing.

Following Lectures 23 and 24, you lay out a static BST in van Emde Boas order, measure its query cost in a block model against a BFS layout, and wrap it in a read-only index.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Building the vEB layout

```cpp
struct VebNode {
    long long key = 0;
    int left = -1;
    int right = -1;
};

std::vector<VebNode> build_veb_layout(std::span<const long long> sorted_keys);
```

`std::vector<VebNode> build_veb_layout(std::span<const long long> sorted_keys)`, with `VebNode{key, left, right}` (`left` and `right` are the children's indices in the returned vector, or `-1` when there is no child).

1. The tree's shape: the balanced BST built by splitting recursively at the median (height `h = ⌈lg(n+1)⌉`).
2. The memory order: the **van Emde Boas layout**. Cut the tree at half its height; lay out the top `⌊h/2⌋` levels first, recursively, followed by each of the subtrees below them, left to right, also recursively. The root must be at index 0.

Return an empty vector when `n = 0`.

## Stage 2 — Measuring query cost in blocks

```cpp
int search_blocks(std::span<const VebNode> nodes, long long key, std::size_t block_bytes);
std::vector<VebNode> build_bfs_layout(std::span<const long long> sorted_keys);
```

There's no cachegrind here, so we measure cost with a deterministic block model:

`int search_blocks(std::span<const VebNode> nodes, long long key, std::size_t block_bytes)`: run a BST search from the root (index 0) until it finds the key or reaches `-1`, and return how many **distinct blocks** the visited nodes span (0 for an empty layout, which has no root). Node `i` occupies bytes `[i·sizeof(VebNode), (i+1)·sizeof(VebNode))`, and block `b` is bytes `[b·block_bytes, (b+1)·block_bytes)` (`block_bytes` is a multiple of `sizeof(VebNode)`).

`std::vector<VebNode> build_bfs_layout(std::span<const long long> sorted_keys)`: a BST of the same shape in an ordinary BFS (level-order) layout, as the baseline.

The tests compare the average number of blocks of the two layouts at `n = 2²⁰`. In a `PERF` test, for blocks of 4096 and 65536 bytes, the vEB layout's average must be below 0.75 times the BFS layout's, and at most `4·⌈log_P n⌉ + 2`, where P = block size / `sizeof(VebNode)` is the number of nodes per block.

## Stage 3 — Real-world extension: a read-only index

```cpp
class VebIndex {
public:
    explicit VebIndex(std::vector<long long> sorted_keys);

    bool contains(long long key) const;
    std::optional<long long> predecessor(long long key) const;
};
```

`VebIndex(std::vector<long long> sorted_keys)`:

- `bool contains(long long key) const`
- `std::optional<long long> predecessor(long long key) const`: the largest key that is not greater than `key`, or `nullopt` if there is none.

O(log n) comparisons.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does every root-to-leaf path in the vEB layout touch only O(log_B n) blocks? What is the intuition behind cutting the tree into subtrees of size `√n`?
2. Why is binary search's O(log(n/B)) worse than O(log_B n)? Why does a B-tree need to know B?
3. When you carry this idea over to the dynamic case (cache-oblivious B-trees, packed memory arrays), what is the main difficulty?
