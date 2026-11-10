---
title: EV Route Planner (battery charge is part of the coordinates)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 PS6 Problem 6-5
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec13.pdf, materials/6.006/recitations/rec13.pdf
kind: code
minutes: 110
---

# 0615 · EV Route Planner

**Original problem:** [Problem Set 6 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0cb5c705752261af9da744c22925a136_MIT6_006S20_ps6_questions.pdf) · **Read first:** [Lecture 13: Dijkstra’s Algorithm notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/d819e7f4568aced8d5b59e03db6c7b67_MIT6_006S20_lec13.pdf), [Recitation 13 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ec187276db0faaa047d655979a44de7f_MIT6_006S20_r13.pdf)

> **Why this comes up in practice:** EV navigation, drone flight paths, and API call chains under a quota are all shortest-path problems where the resources you hold affect where you can go next. What you're really practicing is deciding what belongs in the search state.

## Original problem

6.006 PS6 Problem 6-5 has a race car heading across the country to a road race, using a map of:

- n intersections xᵢ, each with a positive integer elevation eᵢ, some of which have a gas station; and
- r roads between pairs of intersections, where road rⱼ takes positive integer time tⱼ to drive in either direction.

Intersections average fewer than 5 roads each, though some have many. The car must travel from intersection s to the track, which is at intersection t, under two rules:

- The tank's capacity is g units, where g < n is a positive integer, and it begins full. At a gas station it can be topped up by any whole number of units, and each unit takes exactly t_G time.
- Only climbs burn gas: a road from elevation eᵢ up to eⱼ > eᵢ burns eⱼ − eᵢ units, and level or downhill roads burn none.

Give an O(n² log n)-time algorithm for the quickest trip from s to t during which the amount of gas stays strictly above zero, or report that no such trip exists.

## Your task

An electric car must drive from `source` to `target`, as fast as possible.

```cpp
struct Road {
    int a;
    int b;
    std::int64_t minutes;  // >= 1, drivable in both directions
};

struct RoadMap {
    std::vector<int> elevation;     // one per intersection, positive
    std::vector<char> has_charger;  // 0 or 1, one per intersection
    std::vector<Road> roads;
    int capacity;                   // battery capacity, >= 1 unit
    std::int64_t minutes_per_unit;  // >= 1
};
```

Rules:

- The car leaves `source` with a full battery (`capacity` units).
- Driving a road from intersection `x` to `y` takes `minutes`. If `y` is higher than `x`, it uses `elevation[y] - elevation[x]` units of charge; driving downhill or on the flat uses none.
- The charge **must be greater than 0 at all times** (including on arrival at an intersection).
- At an intersection with a charger (including `source` and `target`), the car can charge any whole number of units. Each unit takes `minutes_per_unit` minutes, and the total charge can't exceed `capacity`.

Throw `std::out_of_range` if `source` or `target` doesn't exist. Let `g = capacity`, let `n` be the number of intersections, and let `r` be the number of roads.

This is the original problem with gas as battery charge and gas stations as chargers. Stage 1 has no chargers, Stage 2 adds them, and Stage 3 asks for the trip itself, including how much to charge at each stop.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — No chargers

```cpp
std::optional<std::int64_t> fastest_time(const RoadMap& map, int source, int target);
```

`fastest_time(map, source, target)`: in this stage every `has_charger` is 0. Return the shortest time, or `std::nullopt` if `target` can't be reached; the time is 0 when `source == target`.

Must run in O((n + r) · g · log(n · g)). Note that the fastest route doesn't necessarily keep enough charge.

## Stage 2 — Charging

Chargers are now in use. `fastest_time` must take charging into account, still in O((n + r) · g · log(n · g)). `g` can be far larger than `n`, so handling charging must not multiply the complexity by another factor of `g`.

## Stage 3 — Producing the trip

```cpp
std::optional<RoutePlan> fastest_plan(const RoadMap& map, int source, int target);
```

`fastest_plan(map, source, target)` returns a `RoutePlan`:

```cpp
struct Stop {
    int location;
    int charge_units;  // charged here before leaving, >= 0, only at chargers
};
struct RoutePlan {
    std::int64_t total_minutes;
    std::vector<Stop> stops;  // first is source, last is target, consecutive stops share a road
};
```

Return `std::nullopt` if `target` can't be reached. The trip must actually be drivable; when there is more than one fastest trip, returning any of them is correct.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. How many vertices and edges does this graph have? How does that compare with the original road network?
2. Why is it enough to charge one unit at a time? How much more time would it take to decide directly how much to charge at each intersection?
3. If the battery could regenerate (gaining charge downhill), what would you have to change? Would it affect the assumptions Dijkstra relies on?

---

**Official solution:** [Problem Set 6 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9073af3b8151002c5f63d06dc7ac961e_MIT6_006S20_ps6_solutions.pdf). Read it after you finish.
