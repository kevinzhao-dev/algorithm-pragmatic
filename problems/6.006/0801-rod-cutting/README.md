---
title: Rod Cutting, Unbounded Knapsack, and Coin Change
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 Lecture 18, Recitation 18
pdf: materials/6.006/lectures/lec18.pdf
read: materials/6.006/lectures/lec18.pdf, materials/6.006/recitations/rec18.pdf
kind: code
minutes: 80
---

# 0801 · Rod Cutting, Unbounded Knapsack, and Coin Change

**Based on:** [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf) · **Read first:** [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf), [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf)

> **Why this comes up in practice:** buying reserved capacity, piecing together a configuration that meets a requirement, and making change are all integer decompositions in which "the same kind of thing can be used again". A subproblem is just an integer smaller than the current one, so the running time is pseudopolynomial: how big the data is depends not on how many numbers there are but on **how large the numbers themselves are**.

## Your task

Lecture 18 solves rod cutting, and Recitation 18 solves the unbounded knapsack. Here you also recover the cuts, solve coin change three ways (fewest coins, and two kinds of counts), and buy capacity that must cover a demand rather than fit within a limit.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — The best Rod Cutting value

```cpp
std::int64_t rod_cut_value(std::span<const std::int64_t> prices);
```

`std::int64_t rod_cut_value(std::span<const std::int64_t> prices)`: `prices[l]` is the price that a piece of length `l` sells for (`prices[l] ≥ 0`, and `prices[0]` is unused). The whole rod has length `L = prices.size() − 1`; cut it into pieces of integer length and return the highest total price. Return 0 for empty input or when `L == 0`. O(L²) time.

Cutting into the pieces with the highest price per unit length is **wrong**.

## Stage 2 — The cuts

```cpp
std::vector<std::size_t> rod_cut_pieces(std::span<const std::int64_t> prices);
```

`std::vector<std::size_t> rod_cut_pieces(std::span<const std::int64_t> prices)`: return the length of each piece (in any order). The lengths must add up to `L`, and the total price must be the maximum. Return an empty vector for empty input or when `L == 0`.

## Stage 3 — Only some lengths can be sold (Unbounded Knapsack)

```cpp
struct Item { std::size_t size; std::int64_t value; };   // size >= 1, value >= 0
std::int64_t unbounded_knapsack(std::span<const Item> items, std::size_t capacity);
```

Each kind of item can be used any number of times, and the total size must not exceed `capacity`. Return the highest total value. O(n·capacity) time, O(capacity) memory.

## Stage 4 — Coin Change, with two kinds of counting

```cpp
std::optional<std::int64_t> min_coins(std::span<const int> coins, int amount);  // nullopt when the amount can't be made
std::uint64_t count_change_ways(std::span<const int> coins, int amount);        // order doesn't matter
std::uint64_t count_ordered_ways(std::span<const int> coins, int amount);       // order matters
```

Every denomination is positive, each denomination can be used repeatedly, and `amount ≥ 0` (when `amount == 0`, the minimum is 0 coins and both counts are 1). The two counting functions differ only in the order of their loops; think through why.

## Stage 5 — Real-world extension: buying enough capacity

```cpp
struct Offer { std::int64_t units; std::int64_t price; };   // units >= 1, price >= 0
struct Purchase { std::int64_t cost; std::vector<std::int64_t> counts; };

std::optional<Purchase> cheapest_capacity(std::span<const Offer> offers, std::int64_t demand);
```

You can buy any number of copies of each offer, and the total `units` must reach **at least** `demand`. Return the lowest cost and how many copies of each offer to buy (`counts[i]` corresponds to `offers[i]`, and buying more than the demand is fine). When `demand` is 0, the cost is 0 and every count is 0; when you need to buy something but there are no offers, return `std::nullopt`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Is Rod Cutting's O(L²) polynomial? What about O(n·capacity) in Stage 3? Where's the difference?
2. For which coin systems (such as the coins in circulation today) does greedy happen to be optimal? How can you tell?
3. `count_change_ways` and `count_ordered_ways` swap the inner and outer loops. What subproblem definition does each one correspond to?
