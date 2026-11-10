---
title: Choosing a Sort by Key Type
unit: 03 · Hashing & Linear Sorting (PS3)
source: 6.006 PS3 Problem 3-3
pdf: materials/6.006/psets/ps3/questions.pdf
answer: materials/6.006/psets/ps3/solutions.pdf
read: materials/6.006/lectures/lec05.pdf, materials/6.006/recitations/rec05.pdf
kind: code
minutes: 50
---

# 0313 · Choosing a Sort by Key Type

**Original problem:** [Problem Set 3 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/03a34fcf3012a242ffcae5534e9fa511_MIT6_006S20_ps3-questions.pdf) · **Read first:** [Lecture 5: Linear Sorting notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/78a3c3444de1ff837f81e52991c24a86_MIT6_006S20_lec5.pdf), [Recitation 5 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/cda4cc0c0e626bfbd30bc15fb78c3994_MIT6_006S20_r05.pdf)

> **Why this comes up in practice:** sort the same batch of measurements by an ID, by an integer field, by a high-precision fraction, or by data that can only be compared, and the best method is completely different each time. A database's query planner makes this choice every day.

## Original problem

6.006 PS3 Problem 3-3 is set in glaciology. Ice cores drilled out of glaciers are cut into slices, one per year of deposits. For each scenario below, describe an efficient algorithm (the asymptotically faster, the better) that sorts n slices taken from several ice cores, and justify it.

- (a) Each ice core has a unique core identifier: a string of exactly 16⌈log₄(√n)⌉ ASCII characters, where each character is one byte holding an integer from 0 to 127. Sort the slices by core identifier.
- (b) The oldest cores are up to 800,000 years old. Sort the slices by age, meaning the whole number of years since each slice formed.
- (c) Because snowfall varies from year to year, slices differ in thickness. Each thickness is m/n³ centimeters for some integer m and lies between 0 and 4; sort the slices by thickness.
- (d) A slice's memory can't be measured directly, but for any two slices an O(1)-time test tells which one has more. Sort the slices by memory.

## Your task

A database's batch import pipeline receives `n` `Slice` records and has to sort them by different keys. `Slice` is **move-only**: just move it, don't copy it. You should `#include "../0303-counting-radix-sort/solution.hpp"` directly and reuse the `counting_sort`, `tuple_sort`, and `radix_sort` you already wrote. Stages 1–3 should all be linear (for length `L`, Stage 1 is O(n·L)) and must not use a comparison sort; only Stage 4 needs one. Each stage implements one part of the original problem, (a) through (d), as a function that sorts a span of `Slice` records in place.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — By fixed-length string ID

```cpp
// Move-only on purpose: rearrange slices by moving them, never by copying.
struct Slice {
    Slice(std::string id, std::uint32_t age_years, std::uint64_t thickness, std::uint64_t memory_value,
          int tag_value);  // provided
    Slice(Slice&&) = default;
    Slice& operator=(Slice&&) = default;

    std::string core_id;
    std::uint32_t age;
    std::uint64_t thickness_numerator;
    std::uint64_t memory;
    int tag;
};

inline void sort_by_core_id(std::span<Slice> slices);
```

`sort_by_core_id(slices)`: every `core_id` has the same length and contains only ASCII (0–127). The length `L` can be large (the original problem sets `L = 16⌈log₄√n⌉ = Θ(log n)`). Sort lexicographically by byte. O(n · L).

## Stage 2 — By age

```cpp
inline void sort_by_age(std::span<Slice> slices);
```

`sort_by_age(slices)`: `age` is an integer in `[0, 800000]`. The original problem treats 800000 as a constant, so counting sort is O(n).

But this function is **called frequently, and a batch may hold only a few records**. If every call allocates an 800001-entry array, the Θ(u) constant will sink you on small batches. It must be fast on small batches too.

## Stage 3 — By rational thickness

```cpp
inline void sort_by_thickness(std::span<Slice> slices);
```

`sort_by_thickness(slices)`: `thickness_numerator` is the integer `m`, and the thickness is `m / n³` (`n` is the number of records in the batch), with `0 ≤ m ≤ 4n³`. Sort by increasing thickness.

- `m` can reach `4n³`, which is beyond `2^53`. Never convert to `double` before comparing, or neighboring values of `m` will be treated as equal.
- `n` is at most `10^6`, and `m` fits in `uint64_t`. O(n).

## Stage 4 — Keys that can only be compared

```cpp
template <typename Less>
void sort_opaque(std::span<Slice> slices, Less less);
```

`sort_opaque(slices, less)`: this key (for example, `memory`) can only be compared through `less(a, b)`; you shouldn't even look at the `memory` field. Use O(n log n) comparisons (the tests allow at most `3n⌈lg n⌉ + 1000`); here a comparison sort is optimal.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. What is the real complexity of Stage 1? When `L = Θ(log n)`, is an LSD sort that handles one byte at a time still linear? What would it take to make it linear?
2. How does your base-n radix sort avoid the small-batch trap of Stage 2? If the interviewer insisted on counting sort, how would you change it?
3. Why can't Stage 3 use floating point? If `n` were ten times larger, would `uint64_t` still be enough?

---

**Official solution:** [Problem Set 3 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/9aede1539e086a709a75d604a89f08b6_MIT6_006S20_ps3-solutions.pdf). Read it after you finish.
