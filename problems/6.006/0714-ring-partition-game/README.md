---
title: Ring Partition Game (circular interval DP and minimax)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 PS7 Problem 7-4
pdf: materials/6.006/psets/ps7/questions.pdf
answer: materials/6.006/psets/ps7/solutions.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 90
---

# 0714 · Ring Partition Game

**Original problem:** [Problem Set 7 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/3dff7a1ecbc241e9b91b48e69a1588d4_MIT6_006S20_ps7_questions.pdf) · **Read first:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf)

> **Why this comes up in practice:** the standard technique for circular data (a consistent hashing ring, range planning in a circular buffer) is to cut the ring open into a line, and two agents competing for the same resources (or a worst case treated as an opponent) call for minimax rather than each side's own best solution. This exercise puts the two together.

## Original problem

6.006 PS7 Problem 7-4 has an older sister and her little brother sharing a round pizza. Cuts run outward from its center at the angles α_i = iπ/n for i ∈ {0, 1, …, 2n} (with α_0 = α_2n), dividing it into 2n equal slices. Slice i, between α_i and α_{i+1}, has an integer tastiness t_i that may be negative. The siblings alternate in the role of chooser, starting with the sister:

- When a single slice is left, the chooser eats it and the meal is over.
- Otherwise, an angle α_i counts as proper when the line through the center at that angle has uneaten slices on both of its sides. The chooser selects a proper α_i, with i ∈ {1, …, 2n}, and eats every slice still left in the half-turn counter-clockwise from α_i to α_i + π.
- Then the other sibling becomes the chooser.

The sister wants as much total tastiness as possible. The problem asks for an O(n³)-time algorithm that computes the largest total she can guarantee herself under these rules. A figure shows the labeled angles, one proper and one improper angle, and the position where eating stops. As with every PS7 problem, the solution must be a dynamic program presented through the SRT BOT steps.

## Your task

Stage 1 solves the game that remains after the opening move, played on a line, and Stage 2 adds the opening move on the ring, which answers the original problem with `slices[i]` as t_i. Stage 3 then turns your table into a bot that plays the game, which the original doesn't ask for.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — The game on a line

```cpp
std::int64_t line_game_value(std::span<const int> values);
```

A line holds `n` slices, slice `i` is worth `values[i]` (possibly negative), and two players take turns. The player to move:

- If only one slice is left: eats it, and the game ends.
- Otherwise: cuts between any two adjacent slices (both pieces non-empty), eats one of the two pieces, and leaves the other to the next player.

Each player wants to maximize the total they eat.

`std::int64_t line_game_value(std::span<const int> values)`: return the first player's total when both play optimally. Return 0 for empty input. O(n³) time (range sums must be available in O(1)).

## Stage 2 — The game on a ring

```cpp
std::int64_t ring_game_value(std::span<const int> slices);
```

`2n` slices form a ring, and `slices` lists them in ring order (the size must be even and ≥ 2; otherwise throw `std::invalid_argument`). The first move: pick any starting point and eat **the `n` consecutive slices** that begin there (one cut along a diameter, taking one half). The remaining `n` slices form one contiguous piece, and play continues on it under the Stage 1 rules, with the other player moving first.

`std::int64_t ring_game_value(std::span<const int> slices)`: the first player's best total, in O(n³) time.

## Stage 3 — Playing for real

```cpp
struct Arc { std::size_t start; std::size_t length; };   // start is a position on the ring

class RingGame {
public:
    explicit RingGame(std::span<const int> slices);
    std::int64_t value() const;      // same as ring_game_value
    Arc best_claim(Arc remaining) const;
};
```

The constructor checks `slices` the way `ring_game_value` does: it throws `std::invalid_argument` unless the size is even and ≥ 2.

`best_claim` returns the piece that the player to move should eat (an `Arc` whose start lies on the ring, in `[0, 2n)`):

- `remaining.length == 2n`: the whole ring, where `remaining.start` may be `0`; the returned arc must be exactly `n` slices.
- `remaining.length == 1`: return `remaining` itself.
- Otherwise: return a prefix or a suffix of `remaining` whose length is in `[1, remaining.length − 1]`; this is one move of Stage 1.

The tests pit your bot against every line of opponent play: whatever the opponent does, your bot must get at least `value()`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why doesn't this problem need to record whose turn it is? If the two players scored with different functions (so the game isn't zero-sum), what would the table look like?
2. Why is it enough to append the array to itself just once to handle the ring? Where else can this trick be used with a circular buffer?
3. After the first move there are only `O(n²)` positions, so why does the ring input add only a constant factor of extra cost? If the first move could eat a piece of any length, how many states would there be?

---

**Official solution:** [Problem Set 7 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ed996e1569867432dc1fbdeae53e309f_MIT6_006S20_ps7_solutions.pdf). Read it after you finish.
