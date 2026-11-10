---
title: Failure Domain Analysis (where a backup pays off most, and what happens when a link breaks)
unit: 05 · Graph Search (PS5)
source: 6.006 PS5 Problem 5-2
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec10.pdf
kind: code
minutes: 80
---

# 0512 · Failure Domain Analysis

**Original problem:** [Problem Set 5 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/8c155b28c87cc1fe3f506de2036bdf1b_MIT6_006S20_ps5_questions.pdf) · **Read first:** [Lecture 10: Depth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/f3e349e0eb3288592289d2c81e0c4f4d_MIT6_006S20_lec10.pdf)

> **Why this comes up in practice:** data center power, network topologies, and microservice dependencies all have single points of failure. Before going live you have to answer two questions: which upstream should the money go to, and which broken link would take down the most downstream?

## Original problem

6.006 PS5 Problem 5-2 models a city's power grid with n power plants and n² buildings, any two of which may be linked by a two-way wire. A building has power when it is wired straight to a plant, or wired to a building that has power, and the grid ensures that no building can draw power from more than one plant. The city can pay for one emergency generator at a single plant; if that plant failed, the generator would keep every building it supplies running.

The input is the list W of every wire. Give an O(n⁴)-time algorithm that picks the plant whose generator would back up the most buildings.

## Your task

A data center has `feeds` power feeds and `racks` racks, joined by bidirectional links. Vertex IDs: the feeds are `0 … feeds-1`, and the racks are `feeds … feeds+racks-1`.

```cpp
struct Topology {
    int feeds;
    int racks;
    std::vector<std::pair<int, int>> links;  // undirected; order arbitrary, may repeat
};
```

Guarantees: `feeds >= 1`; the two ends of a link are different; every rack can reach at least one feed (directly, or through other racks), and in Stages 1 and 3 it reaches **exactly** one. The graph may have millions of vertices, and racks can form one very long chain, so **do not use recursion**.

Stage 1 is the original problem, with plants as feeds and buildings as racks. Stages 2 and 3 go further and ask which racks lose power when links are cut.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Where the backup goes

```cpp
struct BackupChoice {
    int feed;
    int racks_covered;
};

BackupChoice choose_backup_feed(const Topology& topology);
```

You have the budget to add backup power to one feed.

`choose_backup_feed(t)` returns `BackupChoice{feed, racks_covered}`: the feed with the most racks under it, and how many. On a tie, take the feed with the smallest ID. You must still return a result when there are no racks at all (`racks_covered == 0`).

Must run in O(V + E).

## Stage 2 — When these links break

```cpp
std::vector<int> racks_without_power(const Topology& topology,
                                     std::span<const std::size_t> cut_links);
```

`racks_without_power(t, cut_links)`: `cut_links` holds indices into `t.links` (possibly repeated). Once all those links are broken, return the IDs of the racks that can no longer reach any feed, in increasing order.

Note that an index refers to that one particular link: if the same pair of vertices has two links, cutting only one of them doesn't cut the power.

In this stage a rack may be connected to more than one feed. It keeps power as long as it can still reach any feed.

Must run in O(V + E + |cut_links|).

## Stage 3 — Taking stock of single points of failure

```cpp
std::vector<int> blast_radius(const Topology& topology);
```

`blast_radius(t)` returns a vector of length `t.links.size()`: the `i`-th value is the number of racks that lose power when **only link `i` breaks**.

Cutting each link in turn and recomputing costs O(E · (V + E)), and E here can reach several million. You must achieve **O(V + E)**.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. The links are guaranteed to give every rack exactly one feed. Without that guarantee, how would the answers to Stage 1 and Stage 3 change?
2. In Stage 3, when is the answer for a link 0? Explain it in terms of the DFS tree.
3. When handling parallel links, why must you skip the way you came in by its edge index rather than by the parent vertex?

---

**Official solution:** [Problem Set 5 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/23d4cf5421c77bb55b3735f7e8f70dfe_MIT6_006S20_ps5_solutions.pdf). Read it after you finish.
