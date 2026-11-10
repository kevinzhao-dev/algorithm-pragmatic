---
title: Two-Sum Within a Distance Limit (close pair)
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 PS3 Problem 3-4
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec04.pdf, materials/6.006/lectures/lec05.pdf
kind: code
minutes: 60
---

# 0314 · Two-Sum Within a Distance Limit

**Original problem:** [Problem Set 3 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/03a34fcf3012a242ffcae5534e9fa511_MIT6_006S20_ps3-questions.pdf) · **Read first:** [Lecture 4: Hashing notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/ce9e94705b914598ce78a00a70a1f734_MIT6_006S20_lec4.pdf), [Lecture 5: Linear Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/78a3c3444de1ff837f81e52991c24a86_MIT6_006S20_lec5.pdf)

> **Why this comes up in practice:** compaction has to find neighboring segments whose total size exactly matches a target block, and a matching engine has to find two orders among the last few that offset each other. Two-sum plus a position limit is the classic dividing line between hashing and sorting.

## Original problem

6.006 PS3 Problem 3-4 takes place in a paper warehouse. A wall n feet long is lined with n boxes of paper, each one foot wide. The box i feet from the left end holds b_i reams, where each b_i is a positive integer and all the b_i are distinct. Given B = (b_0, …, b_{n−1}) and an order of r reams, a pair of boxes (b_i, b_j) is close when |i − j| < n/10, and it fulfills the order when b_i + b_j = r.

- (a) Give an algorithm that decides in expected O(n) time whether some close pair fulfills the order.
- (b) Assuming r < n², give an algorithm that decides the same thing in worst-case O(n) time.

## Your task

You are given an array `b` of length `n` (its elements are **distinct positive integers**) and a target `r`. A pair of indices `(i, j)` with `i < j` is a **close pair** if `b[i] + b[j] == r` and `|i − j| < n/10` (this is real division, `10·(j−i) < n`, not the integer division `n/10`).

Both functions return any close pair (as `(i, j)` with `i < j`), or `std::nullopt` if there is none. Stages 1 and 2 are parts (a) and (b), except that they return a pair instead of yes or no; Stage 3 adds a streaming version that the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Expected O(n)

```cpp
using IndexPair = std::pair<std::size_t, std::size_t>;

inline std::optional<IndexPair> close_pair_expected(std::span<const std::uint64_t> b, std::uint64_t r);
```

`close_pair_expected(b, r)`: expected O(n). You may use a hash table.

Traps:

- `r - b[i]` can underflow on `uint64_t`, and `b[i] + b[j]` can overflow too (values in `b` may be close to `2^64`). Think it through before you subtract.
- When `b[i] == r/2`, it must not pair with itself.
- With `n = 10`, even the minimum distance of 1 is not allowed (`1 < 1` is false); with `n = 11`, distance 1 is allowed (`1 < 1.1`).

## Stage 2 — Worst-case O(n)

```cpp
inline std::optional<IndexPair> close_pair_worst_case(std::span<const std::uint64_t> b, std::uint64_t r);
```

`close_pair_worst_case(b, r)`: `r < n²` is guaranteed. It must run in **worst-case** O(n), and it **must not use a hash table**.

Think first: values in `b` can be far larger than `n²` (for example `2^60`), so why doesn't that slow radix sort down? After sorting, make one pass with two pointers (the two-finger technique); remember that each value has at most one partner. You may `#include "../0303-counting-radix-sort/solution.hpp"` and reuse your `radix_sort`.

## Stage 3 — Real-world extension: streaming window matching

```cpp
class WindowPairMatcher {
public:
    WindowPairMatcher(std::size_t window, std::uint64_t target);

    std::optional<std::size_t> offer(std::uint64_t value);
    std::size_t tracked() const;
};
```

`WindowPairMatcher(window, target)`, with `window ≥ 1` (otherwise throw `std::invalid_argument`):

- `offer(value)`: call number `i` (counting from 0) represents data position `i`. Return the **nearest** position `j` in `[i−window, i−1]` such that `value_j + value_i == target`, or `std::nullopt` if there is none. Here values **may repeat**, and may be 0.
- `tracked()`: the number of distinct values still remembered internally. Old data that has left the window must be forgotten, so this is at most `window`.

Each `offer` must take expected O(1).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why is throwing away the values `≥ r` the key to making Stage 2's radix sort linear? What if `r < n^100`?
2. In Stage 2's two-pointer scan, when `sum == r` but the distance is too large, why can you simply move the left pointer right?
3. In Stage 3, if the keys don't fit in memory (say, terabytes of keys), is a hash map still the right tool?

---

**Official solution:** [Problem Set 3 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9aede1539e086a709a75d604a89f08b6_MIT6_006S20_ps3-solutions.pdf). Read it after you finish.
