---
title: Close Stations (finding close points by divide and conquer)
unit: 01 · Scheduling and Divide & Conquer (PS1)
source: 6.046J PS1 Problem 1-2
pdf: materials/6.046j/psets/ps01/questions.pdf
answer: materials/6.046j/psets/ps01/solutions.pdf
read: materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 120
---

# 0112 · Close Stations

**Original problem:** [Problem Set 1](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/43d974c6c4bfc5d88086a67e023fd88d_MIT6_046JS15_pset1.pdf) · **Read first:** [Divide & Conquer: Convex Hull, Median Finding](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7463c413c944ed72b46a3c3d02b49448_MIT6_046JS15_lec02.pdf)

> **Why this comes up in practice:** when you deploy wireless base stations or place sensors on a rack, the rule is often "every two points must be farther apart than a threshold". At scale you can't compare every pair, and the divide-and-conquer and strip techniques are also the standard approach to closest pair and collision detection.

## Original problem

6.046J PS1 Problem 1-2 ("Radio Frequency Assignment") has a regulator holding a large batch of requests from radio stations that all want to broadcast on one frequency, 88.1 FM. It approves the whole batch only if every two station locations are more than Euclidean distance 1 apart; if even one pair is that close, it rejects every request. Each request carries the (x, y) coordinates of its station. You may assume that no two requests share an x coordinate and no two share a y coordinate, and the input arrives as two sorted lists: L_x, sorted by x, and L_y, sorted by y.

- (a) Divide the map into a grid of squares of size 1/2 × 1/2. Explain why the batch must be rejected if two requests fall in the same square, including its boundary.
- (b) Design an algorithm that decides whether two of the requests are at distance at most 1 and, if they are, returns one such pair. It should run in O(n lg n) time for n requests; the hint is to combine divide and conquer with part (a).
- (c) Explain how to change your part (b) solution so that it decides, also in O(n lg n) time, whether there are three requests that are pairwise within distance 1.

## Your task

Given station coordinates, decide whether any stations are too close together, and return an example.

Stage 1 is part (b) and Stage 2 is part (c). Unlike the original, the distance threshold is a parameter, coordinates may repeat, and you get a single unsorted array; the contract below gives the details.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Numeric contract

- Coordinates are integers with `|x|, |y| ≤ 10^9`.
- "Within distance" means `dx² + dy² ≤ radius_sq`, where `radius_sq` is a non-negative `int64`.
- The squared distance between two points is at most `8·10^18`, which fits in `int64`. **Do not use floating point anywhere:** the tests include cases that differ by only 1, which `hypot` can't tell apart.
- Stations may coincide (distance 0), and x or y coordinates may be equal. The original problem assumes distinct coordinates; this exercise makes no such guarantee.

The input comes in any order, and the return values are indices into the input array.

## Stage 1 — A pair

```cpp
struct Station {
    std::int64_t x;
    std::int64_t y;
};

std::optional<std::pair<int, int>> find_close_pair(std::span<const Station> stations,
                                                   std::int64_t radius_sq);
```

`find_close_pair(stations, radius_sq)`: if there are `i < j` such that `i` and `j` are within distance, return any such `{i, j}`; otherwise return `std::nullopt`.

**O(n log n)**. Use divide and conquer: split in half by x; if neither half contains a pair, you only need to check the strip within distance `r` of the dividing line, and each point there only needs to be compared with a constant number of the points after it in y order. If the strip's points aren't already sorted by y, you have to sort them at every level, or maintain that order as a by-product of merging.

## Stage 2 — A triple

```cpp
std::optional<std::array<int, 3>> find_close_triple(std::span<const Station> stations,
                                                    std::int64_t radius_sq);
```

`find_close_triple(stations, radius_sq)`: if there are `i < j < k` whose three points are pairwise within distance, return any such `{i, j, k}`, sorted; otherwise return `std::nullopt`. **O(n log n)**. You can't enumerate every close pair first and then assemble triangles, because there may be Θ(n²) close pairs.

Re-derive here the reason Stage 1 could get away with a constant number of following points: the two halves now **may** contain close pairs, just no close triple, so the limit on the number of points per grid cell changes.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Cut the plane into cells with side `r/2`. Why can a cell hold at most one point (Stage 1) or two points (Stage 2)? At most how many points ahead must each point in the strip look?
2. If you may use a hash table, how do you solve this in expected O(n)? What is the bad case for that approach, and how do you defend against adversarial input? (Compare universal hashing in PS5.)
3. Why is the algorithm still correct when the dividing line passes right through several points with the same x?

---

**Official solution:** [Solutions to Problem Set 1](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/b14791a78432996a1d403e8b1deb0749_MIT6_046JS15_pset1sols.pdf). Read it after you finish.
