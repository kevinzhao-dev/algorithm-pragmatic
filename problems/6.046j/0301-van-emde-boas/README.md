---
title: van Emde Boas Tree
unit: 03 · van Emde Boas (PS3)
source: 6.046J Lecture 4
pdf: materials/6.046j/lectures/lec04.pdf
read: materials/6.046j/lectures/lec04.pdf
kind: code
minutes: 240
---

# 0301 · van Emde Boas Tree

**Based on:** [Divide & Conquer: Van Emde Boas Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/49c8fa24dffce58052c90d46ac800387_MIT6_046JS15_lec04.pdf) · **Read first:** [Divide & Conquer: Van Emde Boas Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/49c8fa24dffce58052c90d46ac800387_MIT6_046JS15_lec04.pdf)

> **Why this comes up in practice:** a routing table must find "the largest start address that is not greater than this IP" in a fixed-size address space. A balanced tree takes O(log n); vEB brings that down to O(log log u), independent of the size of the set. It is one of the few data structures that beat the lower bound of the comparison model.

## Your task

Maintain a set of integers in `0..u-1`, where `u = 2^bits` and `1 ≤ bits ≤ 64` (otherwise throw `std::invalid_argument`). Throw `std::out_of_range` when a key is out of range.

Split a key into two parts: the high `bits - bits/2` bits are the cluster number, and the low `bits/2` bits are the position within the cluster. The structure follows Lecture 4: each node has a `min`, a `max`, a `summary`, and its `cluster`s, and **`min` is not stored in any cluster**. Use `bits == 1` as the base case.

You build the structure in steps: insertion and successor, then deletion and predecessor, then a sparse version that handles 64-bit keys, and finally a routing table on top of it.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Cost model

`visits()` returns the total number of vEB nodes entered so far: every recursive call, including the call on the root, adds `+1`, while merely reading `min` or `max` doesn't count. The tests use it to verify T(u) = T(√u) + O(1). Each operation may enter at most `2·L + 2` nodes, where `L` is the number of recursion levels, `⌈log₂ bits⌉ + 1`.

## Stage 1 — Insertion and successor

```cpp
class VanEmdeBoas {
public:
    explicit VanEmdeBoas(int universe_bits);

    std::size_t size() const;
    bool empty() const;
    std::optional<std::uint64_t> min() const;
    std::optional<std::uint64_t> max() const;
    bool contains(std::uint64_t x) const;
    bool insert(std::uint64_t x);
    std::optional<std::uint64_t> successor(std::uint64_t x) const;

    // Cost model used by the tests: the number of vEB nodes entered so far.
    std::uint64_t visits() const;  // tested from this stage on
};
```

Implement `insert` (return `true` if the key is new, `false` if it already exists), `contains`, `min`, `max`, `size`, `empty`, and `successor(x)`, which returns the smallest element **strictly greater** than `x`, or `std::nullopt` if there is none.

In this stage the clusters can be an array, but don't hard-code 16 bits here; Stage 3 switches to 64. Note that `successor` takes only one path: it recurses either into a cluster or into the summary, never into both.

## Stage 2 — Deletion and predecessor

```cpp
// in class VanEmdeBoas
bool erase(std::uint64_t x);
std::optional<std::uint64_t> predecessor(std::uint64_t x) const;
```

- `erase(x)`: if `x` is present, delete it and return `true`. When `min` is deleted, pull the new `min` up from the first non-empty cluster; when `max` is deleted, rebuild it from the summary.
- `predecessor(x)`: return the largest element strictly less than `x`. Don't forget that `min` is not in any cluster.

## Stage 3 — A sparse universe

```cpp
// in class VanEmdeBoas
std::size_t node_count() const;
```

Change the clusters to store only the non-empty ones (`std::unordered_map`), so that `bits = 64` also works:

- `node_count()`: the number of vEB nodes currently alive (including the root), which must be `O(n log log u)`.
- A summary or cluster exists only while it holds something, so a single key needs only the root (`node_count() == 1`).
- Once every element has been deleted, all nodes must be freed and `node_count()` returns to `1`.

Note that `1ULL << 64` is undefined behavior, and UBSan will catch it.

## Stage 4 — Real-world extension: a routing table

```cpp
class RouteTable {
public:
    bool add(std::uint32_t first, std::uint32_t last, int port);
    bool remove(std::uint32_t first);
    std::optional<int> lookup(std::uint32_t ip) const;
    std::size_t size() const;
};
```

`RouteTable`: a route maps the IPv4 address range `[first, last]` to a `port`.

| Operation | Behavior |
| --- | --- |
| `add(first, last, port)` | If it overlaps an existing route, return `false` and change nothing; throw `std::invalid_argument` if `first > last` |
| `remove(first)` | Delete the route whose start is exactly `first`, and return whether such a route existed |
| `lookup(ip)` | Return the `port` of the route that contains `ip`, or `std::nullopt` if no route does |
| `size()` | The number of routes |

Store the start addresses in a `VanEmdeBoas(32)`, so that the time of each operation doesn't depend on the number of routes.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does keeping `min` out of the clusters let the recursion in `insert` go down only one side?
2. With `unordered_map`, does T(u) = T(√u) + O(1) still hold? What about the worst case?
3. Why is vEB slower than `std::set` or a B-tree in practice? When is it actually worth it?
