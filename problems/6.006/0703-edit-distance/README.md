---
title: Edit Distance and Spelling Suggestions
unit: 07 · Dynamic Programming I (PS7)
source: 6.006 Recitation 16
pdf: materials/6.006/recitations/rec16.pdf
read: materials/6.006/recitations/rec16.pdf
kind: code
minutes: 100
---

# 0703 · Edit Distance and Spelling Suggestions

**Based on:** [Recitation 16 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/487dc976eabd0f9bcd6a3c385203ea27_MIT6_006S20_r16.pdf) · **Read first:** [Recitation 16 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/487dc976eabd0f9bcd6a3c385203ea27_MIT6_006S20_r16.pdf)

> **Why this comes up in practice:** a search box's did-you-mean, fuzzy matching in data cleaning, and snapshot diffs in CI all ask the same question: how many characters must change before two strings are equal? Real systems only care whether the difference is within k, and the answer is usually known within a few steps, so there's no need to fill in the whole table.

## Your task

Turn the string `from` into `to` using three operations: insert, erase, and replace.

Recitation 16 counts the minimum number of edits, each costing one. Here each operation can have its own cost, and you also produce the edit script, cut the memory, compute a distance bounded by a limit, and build a spelling suggester.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time, and finish it before reading on.

## Stage 1 — Minimum cost

```cpp
struct EditCosts { int insert = 1; int erase = 1; int replace = 1; };  // each >= 1
int edit_distance(std::string_view from, std::string_view to, EditCosts costs = {});
```

O(|from||to|) time. The three operations may cost different amounts, and replace may even cost more than an insert plus an erase, so you must pick the cheapest way.

## Stage 2 — The edit script

```cpp
enum class EditOp { Insert, Erase, Replace };
struct Edit { EditOp op; std::size_t at; char ch; };
std::vector<Edit> edit_script(std::string_view from, std::string_view to, EditCosts costs = {});
```

Applying the script in order must turn `from` into `to`, and its total cost must equal the answer from Stage 1. **`at` is a position in the string as it stands when that step is applied:** `Insert` inserts `ch` before `at`, `Erase` deletes the character at `at`, and `Replace` changes the character at `at` to `ch` (for `Erase`, `ch` may be any value).

## Stage 3 — Cut `edit_distance` to O(min(|from|, |to|)) memory

The heap high-water mark must stay linear in the length of the shorter string, in both argument orders (the tests allow at most 32 bytes per character of the shorter string, and at most 8 KiB when it has only 16 characters).

## Stage 4 — Bounded distance

```cpp
std::optional<int> edit_distance_within(std::string_view from, std::string_view to,
                                        int limit);
```

`std::optional<int> edit_distance_within(from, to, limit)`: with unit costs, return the distance when it is ≤ `limit`, and `std::nullopt` otherwise.

- Time O(limit · min(|from|, |to|)), memory O(limit). Only the cells near the diagonal can lie on the answer.
- When the length difference already exceeds `limit`, you can answer without scanning.
- `limit` may be much larger than the strings (for example 10⁹), so you must not allocate memory based on it (for two short words and a `limit` of 10⁹, the tests allow at most 4 KiB of heap).

## Stage 5 — Real-world extension: spelling suggestions

```cpp
struct Suggestion { std::string_view word; int distance; };
std::vector<Suggestion> suggest(std::string_view query,
                                std::span<const std::string> dictionary, int limit);
```

Return every word in the dictionary whose unit-cost distance is ≤ `limit`, sorted by increasing distance, and by increasing lexicographic order among words at the same distance. The words in `dictionary` are all distinct, and `word` points to a string in `dictionary`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. When Stage 3 swaps the two strings to save memory, why must the insert and erase costs be swapped too?
2. Why is it enough for Stage 4 to look only at `|j - i| ≤ limit`? List the cost settings where this doesn't apply.
3. With hundreds of millions of words in the dictionary, comparing them one by one is too slow. How would you build an index (BK-tree, trigram index)? What does each one cost?
