---
title: Warehouse Robot Errand (access-controlled doors, badges, shortest round trip)
unit: 05 · Graph Search (PS5)
source: 6.006 PS5 Problem 5-5
pdf: materials/6.006/psets/ps5/questions.pdf
answer: materials/6.006/psets/ps5/solutions.pdf
read: materials/6.006/lectures/lec09.pdf, materials/6.006/recitations/rec09.pdf
kind: code
minutes: 110
---

# 0515 · Warehouse Robot Errand

**Original problem:** [Problem Set 5 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/8c155b28c87cc1fe3f506de2036bdf1b_MIT6_006S20_ps5_questions.pdf) · **Read first:** [Lecture 9: Breadth-First Search notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/196a95604877d326c6586e60477b59d4_MIT6_006S20_lec9.pdf), [Recitation 9 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6afd7e9a85d7d36e204533569f9fccf6_MIT6_006S20_r09.pdf)

> **Why this comes up in practice:** robot path planning, keys and switches in games, and permission chains between services (you need a certain token before you can call the next API) are all search problems where, besides where you are, you also have to remember what you're holding.

## Original problem

6.006 PS5 Problem 5-5 follows a student hunting for free pizza in a huge, maze-like building. The map gives a set L of locations, where a whole elevator shaft is treated as a single location that has a door on each floor it serves, and a set D of doors, each connecting two locations. The path has to begin and end at the entrance/exit e ∈ L, and every location in a subset Π ⊂ L has a free pizza. A door d = (ℓ₁, ℓ₂) is either one-way, usable only from ℓ₁ to ℓ₂, or two-way. Some doors are also locked to one of four lab key cards: card t lies at a known location ℓₜ, and a door locked to t opens only once she holds card t.

Give an O(|L| + |D|)-time algorithm for a round trip from e that grabs at least one pizza, picking up whatever cards it needs, with the fewest door crossings.

## Your task

A warehouse has `locations` locations (`0 … locations-1`) connected by doors. The robot starts at `dock`, must reach **at least one** location that holds an item, and must then return to `dock`, passing through as few doors as possible.

```cpp
inline constexpr int kNoBadge = -1;

struct Door {
    int a;
    int b;
    bool two_way;  // false means only a -> b
    int badge;     // kNoBadge, or an index into Site::badge_locations
};

struct Site {
    int locations;
    int dock;
    std::vector<Door> doors;
    std::vector<int> item_locations;   // may be empty, may repeat
    std::vector<int> badge_locations;  // badge i lies at badge_locations[i]; at most 8 badges
};
```

Rules:

- **As soon as** the robot arrives at a location (including being at `dock` at the start), it automatically picks up every badge and item there and keeps them from then on.
- To pass through a door with `badge != kNoBadge`, the robot must already carry that badge (in both directions for a two-way door).
- The robot may pass the same location and the same door any number of times.

`L = locations`, `D = doors.size()`, and `k = badge_locations.size()`.

This is the original problem with pizzas as items and key cards as badges, except that there can be up to 8 badges instead of four. Stage 1 has no badges, Stage 2 adds them, and Stage 3 asks for the route itself.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — No access control

```cpp
std::optional<int> fewest_doors(const Site& site);
```

`fewest_doors(site)`: return the fewest door passages, or `std::nullopt` if the errand can't be done. In this stage every door's `badge` is `kNoBadge`, and `badge_locations` is empty.

If `dock` itself holds an item, the answer is 0.

Treating `k` as a constant, you must achieve O(L + D).

## Stage 2 — Access control

From this stage on, doors can require badges. `fewest_doors` must compute the minimum number by the rules, still in O(L + D) (treating `k` as a constant).

## Stage 3 — The route itself

```cpp
std::optional<std::vector<int>> errand_route(const Site& site);
```

`errand_route(site)`: return the sequence of locations the robot actually walks through (the first and the last are both `dock`; if `dock` holds an item, the sequence is just `dock` on its own), or `std::nullopt` if the errand can't be done. When there is more than one shortest route, returning any of them is correct.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What graph does your search actually traverse? How many vertices and edges does it have? What happens when `k` grows to 30, and what would you do about it?
2. When does a Stage 3 route have to pass the same location more than once? Can you give an example where every shortest route has to double back?
3. If every door took a different amount of time to pass through, which part of BFS would you replace? Would the whole model have to change?

---

**Official solution:** [Problem Set 5 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/23d4cf5421c77bb55b3735f7e8f70dfe_MIT6_006S20_ps5_solutions.pdf). Read it after you finish.
