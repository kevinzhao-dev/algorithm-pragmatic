---
title: Selling a Fixed Amount of Power (0-1 knapsack with a penalty)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 PS8 Problem 8-1
pdf: materials/6.006/psets/ps8/questions.pdf
answer: materials/6.006/psets/ps8/solutions.pdf
read: materials/6.006/recitations/rec18.pdf, materials/6.006/lectures/lec18.pdf
kind: code
minutes: 65
---

# 0811 · Selling a Fixed Amount of Power

**Original problem:** [Problem Set 8 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/740643e3711574870a83ebfceee9a8ae_MIT6_006S20_ps8_questions.pdf) · **Read first:** [Recitation 18 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/e88f4499b351ab82ebf07c4de2859272_MIT6_006S20_r18.pdf), [Lecture 18: Dynamic Programming, Part 4: Rods, Subsetsums, Pseudopolynomial notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/mit6_006s20_lec18.pdf)

> **Why this comes up in practice:** a renewable power plant generates a fixed amount of electricity, whatever it can't sell incurs curtailment or storage costs, and negative electricity prices really do happen; spot capacity in the cloud has the same structure. Put buyers' bids, a fixed capacity, and an idle cost together, and you get a 0-1 knapsack with a penalty.

## Original problem

6.006 PS8 Problem 8-1 is about an oil tycoon expecting m barrels from his wells this month. He has n orders from prospective buyers: order i asks for a_i barrels at a total price of p_i, which covers the whole order rather than each barrel and may be negative. An order is either filled in full or skipped, and none can be filled twice. Leftover oil is allowed, at a storage cost of s dollars for each unsold barrel. The problem asks for an O(nm)-time algorithm that decides which orders to fill so that his profit, which may be negative, is as large as possible. A footnote explains that negative prices are realistic: oil futures had recently traded below zero, because demand collapsed and storage ran short.

Like every PS8 problem, it must be solved with a dynamic program presented through the SRT BOT steps, and you must also say whether the requested running time is polynomial, pseudopolynomial, or exponential in the size of the input. PS8 has no coding part.

## Your task

This week you can sell `capacity` units (`capacity ≥ 0`). There are `n` orders: order `i` wants to buy `amount_i` units for a **total price** of `price_i` (note that it is the total, not a unit price, and that it may be negative: you pay someone to take the power off your hands).

```cpp
struct Order { std::int64_t amount; std::int64_t price; };   // amount >= 1
```

Each order is either filled completely or not at all; you must not oversell, and each order can be filled at most once. Every unsold unit costs `storage_cost` dollars (`storage_cost ≥ 0`).

Profit = the total price of the filled orders − `storage_cost` × (the number of unsold units). The profit may be negative.

This is the original problem with power in place of oil: `capacity` is m, an order's `amount` and `price` are a_i and p_i, and `storage_cost` is s. Besides the best profit and the orders that reach it, you also compute the best profit for every smaller capacity.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Maximum profit

```cpp
std::int64_t max_profit(std::span<const Order> orders, std::int64_t capacity,
                        std::int64_t storage_cost);
```

`std::int64_t max_profit(std::span<const Order> orders, std::int64_t capacity, std::int64_t storage_cost)`: O(n·capacity) time.

The base case is no longer 0; work out what the value is when there are no orders.

## Stage 2 — Which orders to accept

```cpp
std::vector<std::size_t> chosen_orders(std::span<const Order> orders, std::int64_t capacity,
                                       std::int64_t storage_cost);
```

`std::vector<std::size_t> chosen_orders(orders, capacity, storage_cost)`: return the indices of a set of orders that reaches the maximum profit, in increasing order; their total amount must not exceed `capacity`.

## Stage 3 — Real-world extension: the whole capacity curve

```cpp
std::vector<std::int64_t> profit_curve(std::span<const Order> orders, std::int64_t capacity,
                                       std::int64_t storage_cost);
```

The solver's DP table has in fact already answered "how much could I make if I had only `j` units?". Implement

`std::vector<std::int64_t> profit_curve(orders, capacity, storage_cost)`: return `capacity + 1` values, where `result[j]` is the maximum profit when the capacity is exactly `j` units. O(n·capacity) time, O(capacity) memory. Also cut `max_profit` down to O(capacity) memory (the heap high-water mark must stay linear in `capacity`: the tests allow at most 24 bytes per unit of `capacity`, for both functions).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Compare this problem with an ordinary 0-1 knapsack: what is the same, and which base case is different? Why can an order with a negative price be worth accepting?
2. Is this algorithm polynomial, pseudopolynomial, or exponential in the input size? Why?
3. What do the differences between adjacent values of `profit_curve` mean? How could the business use them (for example, to decide whether to buy one more unit of capacity)?

---

**Official solution:** [Problem Set 8 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/39b3ead18e480e4322a8a7308daf8fa1_MIT6_006S20_ps8_solutions.pdf). Read it after you finish.
