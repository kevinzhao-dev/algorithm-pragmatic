---
title: 2D Range Tree (rectangle range queries)
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J Lecture 9
pdf: materials/6.046j/lectures/lec09.pdf
read: materials/6.046j/lectures/lec09.pdf
kind: code
minutes: 120
---

# 0601 · 2D Range Tree

**Based on:** [Augmentation: Range Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/fc870caae0e6812787bb5d50ea4d5e24_MIT6_046JS15_lec09.pdf) · **Read first:** [Augmentation: Range Trees](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/fc870caae0e6812787bb5d50ea4d5e24_MIT6_046JS15_lec09.pdf)

> **Why this comes up in practice:** a monitoring system asks "in the past hour, how many requests had a latency of 200–500 ms, and how many bytes did they send?" A game server asks "which units are inside this rectangle?" Both are axis-aligned range queries over a point set, and a linear scan can't keep up with a hundred thousand points and a hundred thousand queries.

## Your task

The point set never changes after it is built (it is static), so you may spend O(n log n) space to buy query speed. The coordinate type `T` is only guaranteed to be copyable and to support `<` (the other comparison operators exist too). The tests count comparisons with `practice::Counted<int>`, so the complexity bounds are checked as numbers of comparisons.

You build the static range trees from Lecture 9 in three steps: a 1D tree with its canonical decomposition, a 2D tree, and a 2D tree that sums weights.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — 1D range tree and canonical decomposition

```cpp
template <typename T = int>
class RangeTree1D {
public:
    explicit RangeTree1D(std::vector<T> keys);

    std::size_t count(const T& a, const T& b) const;
    std::vector<std::size_t> report(const T& a, const T& b) const;
    // Half-open ranges [first, second) of positions in sorted order.
    std::vector<std::pair<std::size_t, std::size_t>> canonical(const T& a, const T& b) const;
};
```

`RangeTree1D<T>(std::vector<T> keys)`: keys may repeat, and indices refer to positions in the input given to the constructor.

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `count(a, b)` | How many keys satisfy `a ≤ key ≤ b` | O(log n) |
| `report(a, b)` | Return the original indices of those keys, in any order | O(log n + k) |
| `canonical(a, b)` | Return O(log n) half-open ranges `[l, r)` of positions in sorted order; they are pairwise disjoint and their union is exactly the answer | O(log n) |

When `a > b` the range is empty. `canonical` is the array version of the lecture notes' "nodes and subtrees in between": it must not return more than `2⌈lg n⌉ + 2` ranges, and every range it returns must be nonempty (`l < r ≤ n`).

The tests hold `count` to at most `4⌈lg n⌉ + 8` comparisons of `T` per call (checked at n = 2¹⁶).

Edge cases: an empty key set, all keys equal, and a query that covers no key at all.

## Stage 2 — 2D range tree

```cpp
template <typename T = int>
struct Point {
    T x{};
    T y{};
};

template <typename T = int>
class RangeTree2D {
public:
    explicit RangeTree2D(std::vector<Point<T>> pts);

    std::size_t count(const T& x1, const T& x2, const T& y1, const T& y2) const;
    std::vector<std::size_t> report(const T& x1, const T& x2, const T& y1, const T& y2) const;
};
```

`RangeTree2D<T>(std::vector<Point<T>> pts)`, where each point is a `Point<T>{x, y}`. Points may coincide or share an x or y coordinate; each input point is counted and reported separately, by its index.

| Operation | Behavior | Complexity |
| --- | --- | --- |
| `count(x1, x2, y1, y2)` | The number of points in the closed rectangle; 0 when `x1 > x2` or `y1 > y2` | O(log² n) |
| `report(x1, x2, y1, y2)` | Return the original indices of those points, in any order | O(log² n + k) |

Construction takes O(n log n) time and O(n log n) space. The tests hold `count` to at most `8⌈lg n⌉²` comparisons of `T` per call, including the search on x (checked at n = 2¹⁴).

## Stage 3 — Real-world extension: weighted range aggregation

```cpp
template <typename T, typename V>
struct WeightedPoint {
    T x{};
    T y{};
    V weight{};
};

template <typename T, typename V>
class WeightedRangeTree {
public:
    explicit WeightedRangeTree(std::vector<WeightedPoint<T, V>> pts);

    V sum(const T& x1, const T& x2, const T& y1, const T& y2) const;
    std::size_t count(const T& x1, const T& x2, const T& y1, const T& y2) const;
};
```

`WeightedRangeTree<T, V>(std::vector<WeightedPoint<T, V>> pts)`, where each point is a `WeightedPoint<T, V>{x, y, weight}` and `V` is a numeric type that supports addition and subtraction.

- `V sum(x1, x2, y1, y2) const`: the total weight of all points in the closed rectangle; return `V{}` for an empty rectangle. O(log² n).
- `std::size_t count(x1, x2, y1, y2) const`: the same query, but return the number of points.

This is exactly the component behind ad hoc queries such as "total traffic within a given timestamp × latency window".

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't you simply store the rank in each node? What happens with dynamic inserts?
2. How does fractional cascading bring a 2D query down from O(log² n) to O(log n)? How much extra space does it take?
3. If you had to support inserts, would you choose the logarithmic method (Bentley–Saxe) or a balanced tree of balanced trees? What is the worst-case cost of each?
