---
title: Counting, Tuple, and Radix Sort
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 Lecture 5, Recitation 5
pdf: materials/6.006/recitations/rec05.pdf
read: materials/6.006/lectures/lec05.pdf
kind: code
minutes: 75
---

# 0303 · Counting, Tuple, and Radix Sort

**Based on:** [Recitation 5 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cda4cc0c0e626bfbd30bc15fb78c3994_MIT6_006S20_r05.pdf) · **Read first:** [Lecture 5: Linear Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/78a3c3444de1ff837f81e52991c24a86_MIT6_006S20_lec5.pdf)

> **Why this comes up in practice:** databases sorting integer columns, suffix array construction, and sorting pipelines on GPUs all use radix sort, because when the keys come from a bounded range it isn't subject to the `Ω(n log n)` lower bound on comparison sorting. A stable underlying sort is the key to this whole toolkit.

## Your task

Implement the linear-time sorts from Lecture 5 and Recitation 5 (counting sort, tuple sort, and radix sort), and then use them to sort signed integers and equal-length strings.

Every sort has an **in-place interface**: after the call, `items` holds the sorted result (internally you may use an extra buffer). Do not use a comparison sort (`std::sort` and the like).

**Elements may be move-only and may have no default constructor**, so no part of your implementation may rely on copying or default construction.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Counting Sort

```cpp
template <typename T, typename KeyFn>
void counting_sort(std::span<T> items, std::size_t u, KeyFn key);
```

`counting_sort(items, u, key)`: `key(item)` returns an integer in `[0, u)`.

- **Stable:** elements with equal keys keep their original order.
- Time O(n + u). The number of calls to `key` must be O(n) (the tests allow at most `4n`).

## Stage 2 — Tuple Sort

```cpp
template <typename T, typename DigitFn>
void tuple_sort(std::span<T> items, std::size_t digits, std::size_t base, DigitFn digit);
```

`tuple_sort(items, digits, base, digit)`: each element has `digits` fields, and `digit(item, i)` returns field `i` (in `[0, base)`). **`i = 0` is the least significant field.** Sort lexicographically on that basis (the most significant field is `digits - 1`), and stably.

Make sure you understand why you must start sorting from the least significant field rather than the other way around.

## Stage 3 — Radix Sort

```cpp
template <typename T, typename KeyFn>
void radix_sort(std::span<T> items, KeyFn key);
```

`radix_sort(items, key)`: `key(item)` returns any `uint64_t` (**possibly close to `2^64 - 1`**). Sort by key in increasing order, and stably: the tests check that elements with equal keys keep their input order.

- Split the keys into base-n digits (`n` is the number of elements), for time O(n · ⌈log_n(max + 1)⌉). When the keys are bounded by a polynomial in n, that is O(n).
- Do not call a comparison sort. The number of calls to key must be O(n × digits), not O(n log n) (the tests allow at most `n · (5 · digits + 2)`, where digits is the number of base-n digits in the largest key).
- Edge cases: `n ≤ 1`, all keys equal, and a largest key of `2^64 - 1` (watch out: `max + 1` overflows, and so can computing powers).

## Stage 4 — Signed integers and strings

```cpp
inline void sort_signed(std::span<std::int64_t> values);
inline void sort_equal_length(std::span<std::string> strings);
```

- `sort_signed(values)`: the full `int64_t` range, in linear time (you can't handle `INT64_MIN` by negating it).
- `sort_equal_length(strings)`: all strings have the same length. Sort them lexicographically by byte value (`unsigned char`) in O(n × L). Move the strings; don't copy them.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why must radix sort use a stable subroutine? What breaks if you swap in an unstable one?
2. In practice people choose base-256 or base-65536 rather than base-n. Why? What does it cost?
3. What assumptions does the claim "radix sort is O(n)" rest on? What if the keys are big integers of Θ(n) bits?
