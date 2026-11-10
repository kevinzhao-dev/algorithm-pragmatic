---
title: One-Time Valve in a Gravity Pipeline (state that changes the terrain)
unit: 06 · Weighted Shortest Paths (PS6)
source: 6.006 PS6 Problem 6-3
pdf: materials/6.006/psets/ps6/questions.pdf
answer: materials/6.006/psets/ps6/solutions.pdf
read: materials/6.006/lectures/lec11.pdf, materials/6.006/recitations/rec11.pdf
kind: code
minutes: 80
---

# 0613 · One-Time Valve in a Gravity Pipeline

**Original problem:** [Problem Set 6 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0cb5c705752261af9da744c22925a136_MIT6_006S20_ps6_questions.pdf) · **Read first:** [Lecture 11: Weighted Shortest Paths notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/aa57a9785adf925bc85c1920f53755a0_MIT6_006S20_lec11.pdf), [Recitation 11 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/77ff36ad54779575d9ba8d8687f91b5b_MIT6_006S20_r11.pdf)

> **Why this comes up in practice:** an action that permanently changes which paths are open afterward is common in scheduling and configuration changes; for example, flipping a feature flag changes the whole call graph. The key is to think it through: before and after the action, are you really walking the same graph?

## Original problem

6.006 PS6 Problem 6-3 follows a spy fleeing a mountain hideout on skis. Her map shows clearings and the slopes between them, n items altogether. She starts at clearing L beside the hideout and needs to reach the snowmobile parked at clearing S.

- Clearing cᵢ ∈ C sits at integer elevation eᵢ.
- A slope (cᵢ, cⱼ, ℓᵢⱼ) is a trail of positive integer length ℓᵢⱼ between two clearings, along which the elevation changes strictly monotonically. She never skis uphill, so she uses a slope only in its downhill direction.
- Some known clearings are rigged with dynamite. Setting it off makes each rigged clearing sink at once by a fixed, known amount, so its elevation eᵢ becomes some eᵢ′ < eᵢ. The trigger is at clearing D ∈ C, which is not rigged; when she is at D she may set it off before moving on, or not.

Give an O(n)-time algorithm for the shortest total skiing distance from L to S, where she may use the trigger on the way.

## Your task

A pipeline network carries water by gravity, and an inspection robot travels with the flow.

```cpp
struct Pipe { int a; int b; std::int64_t length; };       // length > 0
struct HeadDrop { int junction; std::int64_t new_head; }; // new_head < head[junction]

struct PipeNetwork {
    std::vector<std::int64_t> head;  // water head of each junction
    std::vector<Pipe> pipes;
    std::vector<HeadDrop> drops;     // at most one per junction; none at `valve`
    int valve;                       // the junction with the relief valve
};
```

Rules:

- The robot can only move in the direction in which `head` goes **strictly down**: going from `x` through a pipe to `y` requires `head[x] > head[y]` at that moment. A pipe whose two ends are at the same height can't be used at all.
- While the robot is at the `valve` junction, it may choose to fire the valve (at most once, and it takes no time). Once it fires, the `head` of every junction in `drops` immediately and permanently becomes `new_head`, and from then on the new `head` decides which moves are possible. Firing is optional.

`V = head.size()` and `E = pipes.size()`, and both can exceed a million. Throw `std::out_of_range` if the start or the end doesn't exist.

This is the original problem with clearings as junctions, elevation as water head, and the detonator as the valve. Stages 1 and 2 ask for the shortest length without and with the valve, and Stage 3 also asks for the route and where the valve fires.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — A pipeline that never changes

```cpp
std::optional<std::int64_t> shortest_drift(const PipeNetwork& network, int source, int sink);
```

`shortest_drift(network, source, sink)`: in this stage `drops` is always empty (the valve has no effect). Return the shortest total length from `source` to `sink`, or `std::nullopt` if `sink` can't be reached; the length is 0 when `source == sink`.

Must run in O(V + E).

## Stage 2 — The valve

`drops` is no longer empty. Return the shortest total length as before: when the robot passes `valve` along the way, it can decide whether to fire it.

Must run in O(V + E).

## Stage 3 — Producing the route

```cpp
std::optional<DriftPlan> plan_drift(const PipeNetwork& network, int source, int sink);
```

`plan_drift(network, source, sink)` returns a `DriftPlan`:

```cpp
struct DriftPlan {
    std::int64_t length;
    std::vector<int> route;              // junctions in order, source to sink
    std::optional<std::size_t> trigger;  // index in route at which the valve fires, if ever
};
```

`route[*trigger]` must be `valve`. If the shortest length can be reached without firing, leave `trigger` empty. Return `std::nullopt` if there is no route. When there is more than one shortest route, returning any of them is correct.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does this problem need neither Dijkstra nor Bellman-Ford? What property of the graph does your algorithm rely on?
2. If the valve could fire many times, or there were several different valves, how would your model change? What about the complexity?
3. Why can't you put the directions from before and after firing into one graph and tell them apart by weight?

---

**Official solution:** [Problem Set 6 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9073af3b8151002c5f63d06dc7ac961e_MIT6_006S20_ps6_solutions.pdf). Read it after you finish.
