---
title: LCS and Minimal Diff (Hirschberg)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 Lecture 16
pdf: materials/6.006/lectures/lec16.pdf
read: materials/6.006/lectures/lec16.pdf
kind: code
minutes: 110
---

# 0702 · LCS and Minimal Diff

**Based on:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf) · **Read first:** [Lecture 16: Dynamic Programming, Part 2: LCS, LIS, Coins notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/28461a74f81101874a13d9679a40584d_MIT6_006S20_lec16.pdf)

> **Why this comes up in practice:** `git diff`, three-way merge, and DNA sequence alignment are all relatives of the longest common subsequence: take two sequences, keep what they share, and everything else is a change. Once the files get large, the memory for the 2D table stops you before the running time does.

## Your task

The longest common subsequence is the longest of the subsequences that `a` and `b` have in common (its elements need not be contiguous, but they must appear in the same order).

Write every function as a function template that accepts any `std::ranges::random_access_range` whose elements need only `operator==`.

Lecture 16 solves LCS with a 2D table and parent pointers. Here you go further: you cut the memory down to linear, first for the length and then for the alignment itself (Hirschberg's method), and finish with a line-level diff.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Length

```cpp
template <std::ranges::random_access_range A, std::ranges::random_access_range B>
std::size_t lcs_length(const A& a, const B& b);
```

`std::size_t lcs_length(const A& a, const B& b)`, in O(|a||b|) time, comparing each pair of elements at most a constant number of times (the tests count element comparisons and allow at most 2·|a|·|b|).

## Stage 2 — Recovering one solution

```cpp
using Alignment = std::vector<std::pair<std::size_t, std::size_t>>;

template <std::ranges::random_access_range A, std::ranges::random_access_range B>
Alignment lcs_alignment(const A& a, const B& b);
```

`Alignment lcs_alignment(const A& a, const B& b)`: `Alignment` is `std::vector<std::pair<size_t, size_t>>`, and each pair `(i, j)` means that `a[i]` is matched with `b[j]`. Both `i` and `j` must be strictly increasing, and the length must equal the LCS length. Many solutions have the same length; any one of them is fine.

## Stage 3 — Cut the memory of `lcs_length` to O(min(|a|, |b|))

When you only need the length, the relation looks only at the previous row, and you can pick the shorter sequence for the columns. Extra allocation must stay within `O(min(|a|, |b|))` (the tests measure the heap high-water mark in both argument orders: at most 64 bytes per element of the shorter sequence, and at most 8 KiB when it has only 16 elements). `lcs_alignment` from Stage 2 doesn't need to change.

## Stage 4 — Recovering a solution in linear memory (Hirschberg)

```cpp
template <std::ranges::random_access_range A, std::ranges::random_access_range B>
Alignment lcs_alignment_linear(const A& a, const B& b);
```

At 10⁵ × 10⁵, a 2D table of parent pointers doesn't fit in memory. Implement `Alignment lcs_alignment_linear(const A& a, const B& b)`: the time is still O(|a||b|), and the heap high-water mark is O(|a| + |b|) (the tests allow at most 72·(|a| + |b|) bytes).

The key idea: using only the rolling rows from Stage 3, run forward over the first half of `a` and backward over the second half of `a`, and you can work out where an optimal solution crosses the midline in `b`. Once that splits the problem into two pieces, recurse on each; together the two pieces take only half the original work.

## Stage 5 — Real-world extension: line-level diff

```cpp
enum class DiffKind { Keep, Remove, Add };
struct DiffLine { DiffKind kind; std::string_view text; };

std::vector<DiffLine> diff_lines(std::span<const std::string> before,
                                 std::span<const std::string> after);
```

- Taking the `Keep` and `Remove` lines in order must give `before`; taking the `Keep` and `Add` lines in order must give `after`.
- The number of `Keep` lines must equal the LCS length, which makes the diff minimal.
- Between two `Keep` lines, every `Remove` of that change comes before its `Add` lines (the usual diff convention).
- `text` points into the strings passed in, and the caller must keep those strings alive longer than the result.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why does Stage 4 cost only a constant factor more time? Add up the work at each level of the recursion for the interviewer.
2. Why does the rolling in Stage 3 make the recovery from Stage 2 impossible? Which other DPs can use the same "midline" trick?
3. The Myers algorithm that `git diff` actually uses runs in O((n+m)D). On what inputs is it much faster than the LCS table? When does it make no difference?
