---
title: Interval DP Toolbox (palindromes, optimal BST, coin game)
unit: 06 · Augmentation, APSP, and MST (PS6)
source: 6.046J Lecture 10, Recitation 5
pdf: materials/6.046j/lectures/lec10.pdf
read: materials/6.046j/lectures/lec10.pdf, materials/6.046j/recitations/rec05.pdf
kind: code
minutes: 100
---

# 0602 · Interval DP Toolbox

**Based on:** [Dynamic Programming: Advanced DP](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/6144f2dfbe515abfbeb32f99a6907af7_MIT6_046JS15_lec10.pdf) · **Read first:** [Dynamic Programming: Advanced DP](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/6144f2dfbe515abfbeb32f99a6907af7_MIT6_046JS15_lec10.pdf), [Dynamic Programming: More Examples](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/d187472426117c2ebec700a22c209b3f_MIT6_046JS15_Recitation5.pdf)

> **Why this comes up in practice:** LCS in diff tools, matrix-chain ordering and register allocation in compilers, and the optimal search tree for a static dictionary all use the same trick: split an interval and guess where to cut. An interview question won't tell you to use DP; you have to see for yourself that the subproblem is `(i, j)`.

## Your task

The lecture notes' four steps: define the subproblems → write the recurrence → compute from small to large → backtrack to recover the answer. Every stage must actually carry out step 4.

You implement five DPs from Lecture 10 and Recitation 5 across four stages, and each one has to produce its answer, not just fill a table.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Longest palindromic subsequence

```cpp
std::string longest_palindromic_subsequence(std::string_view s);
```

`std::string longest_palindromic_subsequence(std::string_view s)`: return any longest palindromic subsequence of `s` (same order, characters may be skipped). Return an empty string for an empty string. O(n²) time.

## Stage 2 — Optimal BST

Keys `0..n-1` are sorted in increasing order, and key `i` has weight `weights[i]`. Find the BST that minimizes `Σ weights[i]·(depth(i)+1)` (the root has depth 0).

```cpp
struct BstPlan {
    long long cost = 0;              // 0 when there are no keys
    int root = -1;                   // -1 when there are no keys
    std::vector<int> left, right;    // left[i] / right[i]: child key of i, -1 if none
};
BstPlan optimal_bst(std::span<const long long> weights);
```

O(n³) time. Edge cases: no keys, and a single key.

## Stage 3 — Two-player coin game

```cpp
long long first_player_value(std::span<const long long> values);
```

Coins sit in a row. Two players take turns removing one coin from either the left end or the right end, and both play optimally. `long long first_player_value(std::span<const long long> values)` returns the largest amount the first player can guarantee. O(n²) time. Return 0 for an empty row.

## Stage 4 — Two more ways to cut

```cpp
struct Block {
    long long length = 0;
    long long width = 0;
    long long height = 0;
};
long long tallest_tower(std::span<const Block> blocks);
long long count_true(std::string_view expr);
```

**Tower stacking.** A `Block{length, width, height}` cannot be rotated. `A` can go on top of `B` when `A.length ≤ B.length` and `A.width ≤ B.width`. `long long tallest_tower(std::span<const Block> blocks)` returns the height of the tallest tower, using each block at most once; with no blocks it returns 0.

**Boolean parenthesization.** `expr` alternates between `T`/`F` and the operators `&`, `|`, and `^` (for example `T^F&T`; the input is guaranteed to be well formed). `count_true(expr)` returns the number of ways to parenthesize it so that it evaluates to true, modulo `1'000'000'007`. O(n³) time.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is the greedy approach to optimal BSTs (always make the heaviest key the root) wrong? Give the smallest counterexample. How does Knuth's optimization bring O(n³) down to O(n²)?
2. Coin game: why can the first player never lose when the number of coins is even? What does the O(n²) DP gain over the "take all the odd positions or all the even positions" strategy?
3. Boolean parenthesization: why do you need two tables, one for true and one for false? What goes wrong if you keep only one?
