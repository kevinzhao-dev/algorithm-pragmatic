---
title: Alternating Coin Game (two-player zero-sum interval DP)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 Lecture 16
pdf: materials/6.006/lectures/lec16.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 60
---

# 0705 · Alternating Coin Game

**Based on:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf) · **Read first:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf)

> **Why this comes up in practice:** minimax is the starting point of self-play systems like AlphaZero, and sharing positions that repeat is what a transposition table does. Here a position is described entirely by the remaining range, which makes this a good place to practice DP for two-player games: your opponent is also making the best decisions, so your answer has to hold up against every reply.

## Your task

A row of `n` coins has values `v[0..n-1]` (possibly negative). Two players take turns removing one coin from **the front or the back of the current row** until none are left (on your turn, you must take a coin). Each player wants to maximize their own total.

Lecture 16 computes the first player's best total. Here you also turn the table into a bot that can play the game, and then cut the memory down to O(n).

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — How much the first player can get

```cpp
std::int64_t coin_game_value(std::span<const int> coins);
```

`std::int64_t coin_game_value(std::span<const int> coins)`: the first player's total when both players play optimally. O(n²) time. Return 0 for empty input.

Greedy (always taking the larger end) is wrong.

## Stage 2 — A strategy that can actually play

```cpp
enum class Side { Front, Back };

class CoinGame {
public:
    explicit CoinGame(std::span<const int> coins);       // O(n^2)
    std::int64_t value() const;                          // same as Stage 1
    Side best_move(std::size_t first, std::size_t last) const;  // O(1)
};
```

`best_move(first, last)`: when `coins[first..last]` remain (both ends included, `first <= last`), the end the player to move should take to reach the best outcome. The tests pit your bot against every line of opponent play, enumerated exhaustively: whatever the opponent does, your bot must get at least `value()`.

## Stage 3 — Cut `coin_game_value` to O(n) memory

When you compute by increasing range length, you only need the layer whose length is one less. The heap high-water mark must stay within O(n) (the tests allow at most 40 bytes per coin). Using the difference between the two players' totals as the subproblem removes the need for prefix sums and is also simpler.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. This game is zero-sum, so one table is enough. What kind of scoring rule would force you to also record whose turn it is, or even both players' scores?
2. When the length `n` is even, the first player actually has a simple strategy that never loses (judging only by who has the larger total). Why can't it tell you the most the first player can take?
3. Once the relation is rewritten in terms of the difference, how do you turn the answer from a difference back into a total? Can integer division cause trouble?
