---
title: Cable Tray Routing (shortest path along tile boundaries)
unit: 05 · Graph Search (PS5)
source: 6.006 PS5 Problem 5-4
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 70
---

# 0514 · Cable Tray Routing

**Original problem:** [Problem Set 5 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/8c155b28c87cc1fe3f506de2036bdf1b_MIT6_006S20_ps5_questions.pdf) · **Read first:** [Lecture 9: Breadth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/196a95604877d326c6586e60477b59d4_MIT6_006S20_lec9.pdf), [Recitation 9 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6afd7e9a85d7d36e204533569f9fccf6_MIT6_006S20_r09.pdf)

> **Why this comes up in practice:** PCB routing, data center cabling, and pipe planning on a map often run along the grid lines rather than through the cells. What this exercise really trains is working out what the graph's vertices and edges are before you start coding.

## Original problem

6.006 PS5 Problem 5-4 is about an old map, recorded on clay tablets, of an n × n grid of squares. Each square is either river, labeled `'euphris'` or `'tigrates'` for the two rivers, or farmland marked with its owner's name. Merchants once linked the rivers with a trade route that follows grid lines: it starts at a corner of some `'euphris'` square and ends at a corner of some `'tigrates'` square. The route never runs along a grid line that separates two plots of the same farmer, and among all routes with that property it is the shortest; assume the shortest one is unique.

Describe how to recover the route from the map in O(n²) time.

## Your task

The data center floor is an `n × n` grid of tiles, and `tiles[row][col]` is the owner of a tile:

| Value | Meaning |
| --- | --- |
| `kZoneA` (-1) | Zone A server room |
| `kZoneB` (-2) | Zone B server room |
| `>= 0` | A tenant's ID |

Cable trays run along the **boundaries** of the tiles. An intersection is written `Corner{r, c}`, with `0 ≤ r, c ≤ n`; the four corners of tile `(row, col)` are `(row, col)`, `(row+1, col)`, `(row, col+1)`, and `(row+1, col+1)`. A tray segment joins two adjacent intersections, and its two sides are:

- Horizontal segment `(r, c)–(r, c+1)`: the tile above is `(r-1, c)` and the tile below is `(r, c)`.
- Vertical segment `(r, c)–(r+1, c)`: the tile on the left is `(r, c-1)` and the tile on the right is `(r, c)`.

Rules:

- If the tiles on both sides of a segment belong to the **same tenant**, the segment can't be used (it would cut through someone else's rack area).
- A segment on the outer edge of the floor has a tile on only one side, so it can always be used. Zone A and Zone B tiles don't count as tenants and never restrict a segment.

A route must start at a corner of some Zone A tile and end at a corner of some Zone B tile, using as few segments as possible. `n` goes up to several thousand.

This is the original problem with the rivers as Zone A and Zone B and the farmers as tenants. Stage 1 asks only for the length and Stage 2 for the route itself, and since the shortest route isn't assumed to be unique, any shortest route is accepted.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Shortest length

```cpp
inline constexpr int kZoneA = -1;
inline constexpr int kZoneB = -2;

using TileMap = std::vector<std::vector<int>>;

std::optional<int> shortest_tray_length(const TileMap& tiles);
```

`shortest_tray_length(tiles)`: return the fewest segments. The start and the end may be the same intersection (length 0). Return `std::nullopt` if they can't be connected, or if the map has no Zone A or no Zone B.

Must run in O(n²).

## Stage 2 — The route itself

```cpp
using Corner = std::pair<int, int>;  // (r, c)

std::optional<std::vector<Corner>> shortest_tray_route(const TileMap& tiles);
```

`shortest_tray_route(tiles)`: return the sequence of intersections along a shortest route (both ends included, so a single intersection when the length is 0), or `std::nullopt` if there is no route. There may be more than one shortest route, and returning any of them is correct.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can't you use the tiles as vertices? If the trays ran through the centers of the tiles instead, how would the graph change?
2. Why don't you need to run a separate BFS from every start? In one sentence, say what kind of graph your approach is equivalent to.
3. If every tray segment had its own cost, or some segments could be closed temporarily, what in your approach would have to change?

---

**Official solution:** [Problem Set 5 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/23d4cf5421c77bb55b3735f7e8f70dfe_MIT6_006S20_ps5_solutions.pdf). Read it after you finish.
