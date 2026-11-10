---
title: Text Justification (optimal line breaking)
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 Recitation 15
pdf: materials/6.006/recitations/rec15.pdf
read: materials/6.006/recitations/rec15.pdf
kind: code
minutes: 60
---

# 0706 · Text Justification

**Based on:** [Recitation 15 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6022dc36557c5e5df2ad0a77e9463855_MIT6_006S20_r15.pdf) · **Read first:** [Recitation 15 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/6022dc36557c5e5df2ad0a77e9463855_MIT6_006S20_r15.pdf)

> **Why this comes up in practice:** TeX lays out paragraphs with this DP (Knuth–Plass), while a browser's default greedy line breaking often leaves ugly short lines. Any problem that cuts a sequence into pieces, each with its own cost (splitting into chunks, sending in batches), is the same kind of suffix DP.

## Your task

There are `n` words with widths `widths[i]`, and each line has a usable width of `line_width`. A line holds several consecutive words separated by single spaces, so it uses `used = Σ widths + (words − 1)`, and `used ≤ line_width` must hold. Every line except **the last line** has badness `(line_width − used)³`; the last line costs nothing. The goal is to minimize the total badness.

Recitation 15 poses a slightly different version that doesn't count the spaces and charges the last line like any other. Here you compute the minimum badness, recover the line breaks, and then wrap real text.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Minimum badness

```cpp
std::optional<std::int64_t> min_badness(std::span<const int> widths, int line_width);
```

`std::optional<std::int64_t> min_badness(std::span<const int> widths, int line_width)`

- If any word is wider than `line_width`, return `std::nullopt`; return 0 for empty input.
- O(n²) time (use prefix sums for the widths on a line instead of recomputing them each time, and stop trying as soon as a line no longer fits).
- Limits: `line_width ≤ 10⁴`, `n ≤ 10⁵`, `widths[i] ≥ 1`.

## Stage 2 — Recovering the line breaks

```cpp
std::optional<std::vector<std::size_t>> line_starts(std::span<const int> widths,
                                                    int line_width);
```

`std::optional<std::vector<std::size_t>> line_starts(std::span<const int> widths, int line_width)`: return the index of the first word on each line, in increasing order, starting with `0` (an empty vector for empty input). The total badness must equal the answer from Stage 1; when several layouts tie, any of them is fine.

## Stage 3 — Real-world extension: laying out a paragraph

```cpp
std::optional<std::vector<std::string>> wrap_text(std::string_view text, int line_width);
```

`std::optional<std::vector<std::string>> wrap_text(std::string_view text, int line_width)`: split the text into words at runs of whitespace (`' '`, `'\t'`, `'\n'`), lay them out with the result of Stage 2, and join the words on each line with single spaces. Return an empty vector when there are no words, and `std::nullopt` when some word doesn't fit.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is a suffix (or prefix) enough as the subproblem? If badness depended on the length of the previous line (for example, a penalty when adjacent lines differ too much), how would you extend the state?
2. For which badness function does greedy happen to be optimal? Why does the cube make it fail?
3. When `n` reaches a million, O(n²) can't keep up. How does Knuth–Plass actually prune the candidate breakpoints?
