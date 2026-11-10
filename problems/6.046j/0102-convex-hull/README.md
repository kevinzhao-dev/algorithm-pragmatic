---
title: Convex Hull (divide and conquer)
unit: 01 · Scheduling and Divide & Conquer (PS1)
source: 6.046J Lecture 2
pdf: materials/6.046j/lectures/lec02.pdf
read: materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 150
---

# 0102 · Convex Hull

**Based on:** [Divide & Conquer: Convex Hull, Median Finding](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7463c413c944ed72b46a3c3d02b49448_MIT6_046JS15_lec02.pdf) · **Read first:** [Divide & Conquer: Convex Hull, Median Finding](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7463c413c944ed72b46a3c3d02b49448_MIT6_046JS15_lec02.pdf)

> **Why this comes up in practice:** geofences, safe zones for robot paths, and the broad phase of collision detection all start by wrapping a set of points in a convex polygon. The most common bugs in this kind of code are not in the main flow but in collinear points, duplicate points, and floating-point error.

## Your task

For simplicity, Lecture 2 assumes that no three points are collinear and that no two points share an x coordinate. **This exercise must handle all of these cases.**

You build the divide-and-conquer hull from Lecture 2 one piece at a time (the orientation test, the tangents, the merge, and the full algorithm), with exact integer arithmetic throughout, and then use it for point-in-region queries.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Numeric and representation contract

- Coordinates are integers with `|x|, |y| ≤ 10^9`. Each term of `(b−a)×(c−a)` is at most `4·10^18`, and their difference also fits in `int64`. **Do not use floating point anywhere.**
- The canonical form of a hull: its vertices are **strictly convex** (remove collinear middle points), listed **counterclockwise**, starting from the lexicographically smallest point (compare x first, then y). Degenerate cases: no points gives an empty hull, one point gives that point, and points that are all collinear give the two endpoints `{smaller, larger}`.

## Stage 1 — Orientation and tangents

```cpp
struct Point {
    std::int64_t x;
    std::int64_t y;
    friend auto operator<=>(const Point&, const Point&) = default;
};

struct Tangent {
    std::size_t left;
    std::size_t right;
};

int orient(Point a, Point b, Point c);
Tangent upper_tangent(std::span<const Point> left, std::span<const Point> right);
Tangent lower_tangent(std::span<const Point> left, std::span<const Point> right);
```

- `orient(a, b, c)`: return `1` if `c` is to the left of the directed line `a→b`, `-1` if it is to the right, and `0` if the three points are collinear.
- `upper_tangent(left, right)`, `lower_tangent(left, right)`: both arguments are hulls in canonical form (with at least one point), and every point of `left` is lexicographically smaller than every point of `right`. Return the indices of the tangent points in the two hulls. If several vertices lie on the tangent line, return the **outermost** ones (the lexicographically smallest in `left` and the lexicographically largest in `right`). O(|left| + |right|) in total.

The lecture compares intercepts on a vertical line, which breaks when x coordinates are equal. Decide with `orient` instead and think of the lexicographic order as an infinitesimally small shear; then you need no special cases. Watch out for hulls with one or two points: your fingers must not go around in circles.

## Stage 2 — Merging

```cpp
std::vector<Point> merge_hulls(std::span<const Point> left, std::span<const Point> right);
```

`merge_hulls(left, right)`: the same preconditions as above, except that either side may be empty. Return the hull of the union in canonical form, in O(|left| + |right|).

## Stage 3 — The full algorithm

```cpp
std::vector<Point> convex_hull(std::span<const Point> points);
```

`convex_hull(points)`: `points` come in any order and may contain duplicates, collinear points, and equal x coordinates. Sort once, then divide and conquer: T(n) = 2T(n/2) + Θ(n).

## Stage 4 — Real-world extension: geofencing

```cpp
class ConvexRegion {
public:
    explicit ConvexRegion(std::span<const Point> points);

    const std::vector<Point>& hull() const;
    bool contains(Point p) const;
};
```

`ConvexRegion(points)` wraps a point set into a region; after construction it must not depend on the `span` passed in.

- `hull()`: return the hull in canonical form.
- `contains(p)`: return `true` if `p` is inside the region or on its boundary, in **O(log h)**, where `h` is the number of hull vertices. When the region degenerates to a point or a segment, a point counts as inside only if it lies on that point or segment. An empty region (built from no points) contains nothing.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't the two fingers, moving in turn, miss the tangent? Explain it in two steps, "never overshoots" and "where they stop is always a supporting line", and point out the case in which `>=` and `>` behave differently.
2. Where does it break if the coordinates become `double`? If the coordinates must be `double`, what does industry do (hint: adaptive exact predicates)?
3. This problem can also be solved with a monotone chain in O(n) after sorting. So what is the divide-and-conquer version still good for? (Think about parallelization and the merges in external sorting.)
