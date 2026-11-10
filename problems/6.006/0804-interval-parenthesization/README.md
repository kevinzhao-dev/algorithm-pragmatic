---
title: Parenthesization and Matrix Chain (interval DP)
unit: 08 · Dynamic Programming II (PS8)
source: 6.006 Lecture 17
pdf: materials/6.006/lectures/lec17.pdf
read: materials/6.006/lectures/lec17.pdf
kind: code
minutes: 85
---

# 0804 · Parenthesization and Matrix Chain

**Based on:** [Lecture 17: Dynamic Programming, Part 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/665523227a175e9e9ce26ea8d3e5b51c_MIT6_006S20_lec17.pdf) · **Read first:** [Lecture 17: Dynamic Programming, Part 3 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/665523227a175e9e9ce26ea8d3e5b51c_MIT6_006S20_lec17.pdf)

> **Why this comes up in practice:** `numpy.linalg.multi_dot`, `opt_einsum`, and the join ordering in a SQL optimizer all decide "the order of operations". A subproblem is a contiguous range, and the answer comes from trying every position for the last cut; once signs come into play, each subproblem must also record its minimum, because the product of two negative numbers can turn into the maximum.

## Your task

The input is a sequence of operands with an operator between each pair: `operands.size() == ops.size() + 1`, and each character of `ops` is `'+'` or `'*'`. You may add parentheses however you like (changing the order of evaluation), but the left-to-right order of the operands and operators must not change. Every possible parenthesization is guaranteed to evaluate to a value that fits in `std::int64_t`.

Throw `std::invalid_argument` when `operands` is empty, or when `ops` has the wrong length or contains any other character.

Lecture 17 solves arithmetic parenthesization with negative numbers allowed. Here you work up to it from non-negative operands, print the best parenthesization, and then apply the same interval DP to matrix-chain multiplication.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — The maximum with non-negative operands

```cpp
std::int64_t max_value(std::span<const std::int64_t> operands, std::string_view ops);
```

`std::int64_t max_value(std::span<const std::int64_t> operands, std::string_view ops)`: the `operands` are all ≥ 0; return the largest value that any parenthesization can produce. O(n³) time.

## Stage 2 — Allowing negative numbers

`operands` may now be negative (the function stays the same). Recording only the maximum is no longer enough: think about when multiplying the maxima of two ranges doesn't give the overall maximum.

## Stage 3 — Recovering the parentheses

```cpp
std::string best_parenthesization(std::span<const std::int64_t> operands,
                                  std::string_view ops);
```

`std::string best_parenthesization(std::span<const std::int64_t> operands, std::string_view ops)`: return one parenthesization that reaches the maximum, in exactly this format:

- A single operand: written in decimal, with a `-` for a negative number, for example `-3`.
- Every operation gets its own pair of parentheses: `(left+right)` or `(left*right)`, with no spaces.

For example, `(1+(2*-3))`.

## Stage 4 — Real-world extension: Matrix Chain

`dims` describes a product of `n = dims.size() − 1` matrices, where matrix `i` has size `dims[i] × dims[i+1]` (every entry of `dims` is positive and `dims.size() ≥ 2`; otherwise throw `std::invalid_argument`). Multiplying an `a×b` matrix by a `b×c` matrix costs `a·b·c`.

```cpp
std::int64_t min_multiplications(std::span<const std::int64_t> dims);
std::string best_multiplication_order(std::span<const std::int64_t> dims);
```

Return the minimum total cost, and a multiplication order that achieves it, in this format: a single matrix is written `M{i}` (where `i` is its decimal index), and a product is written `(left*right)`, for example `((M0*M1)*M2)`. O(n³) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does Stage 2 have O(n²) subproblems and not more? How many combinations does the relation have to look at?
2. Which is more natural here, building the subproblem around the last cut or around the first cut? What does the tree look like when you recover the solution?
3. With many tensors, `opt_einsum` doesn't run the full DP. What does it use instead, and what does it give up?
