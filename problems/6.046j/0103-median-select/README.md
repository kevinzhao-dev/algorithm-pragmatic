---
title: Median of Medians (deterministic select)
unit: 01 · Scheduling and Divide & Conquer (PS1)
source: 6.046J Lecture 2
pdf: materials/6.046j/lectures/lec02.pdf
read: materials/6.046j/lectures/lec02.pdf
kind: code
minutes: 90
---

# 0103 · Median of Medians

**Based on:** [Divide & Conquer: Convex Hull, Median Finding](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7463c413c944ed72b46a3c3d02b49448_MIT6_046JS15_lec02.pdf) · **Read first:** [Divide & Conquer: Convex Hull, Median Finding](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/7463c413c944ed72b46a3c3d02b49448_MIT6_046JS15_lec02.pdf)

> **Why this comes up in practice:** to compute p50 or p99, or to pick the top k from a batch of candidates, you don't need to sort all the data. `std::nth_element` is O(n) on average but can degrade on specific inputs; when you need a worst-case guarantee, this is the algorithm.

## Your task

`T` is only guaranteed to support copying, moving, and `operator<`. Do not use `==`.

You build the deterministic median-of-medians select from Lecture 2 on top of a three-way partition, with the tests counting its comparisons, and then adapt the idea to a weighted median.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Three-way partition

```cpp
template <typename T>
std::pair<std::size_t, std::size_t> partition3(std::span<T> values, T pivot);
```

`partition3(values, pivot)`: rearrange `values` into three parts, "less than pivot", "equal to pivot", and "greater than pivot", and return `{less_end, greater_begin}`, the start positions of the second and third parts.

Compare each element at most 2 times. `pivot` is passed by value because callers often take it from `values`, and a reference would change under you during the rearrangement.

## Stage 2 — Deterministic select

```cpp
template <typename T>
T select_kth(std::span<T> values, std::size_t k);
```

`select_kth(values, k)`: return the `k`-th smallest element (0-based). `values` gets rearranged, but no element may be lost. Throw `std::out_of_range` if `k` is out of range.

It must run in O(n) in the worst case. The tests count comparisons with `Counted`:

- With the median of the group-of-five medians as the pivot, make at most `60n` comparisons.
- As `n` grows from `2^10` to `2^20`, the average number of comparisons per element must not grow with it. This rule rejects `sort`, whose ratio grows with `lg n`.
- The tests include an adaptive adversary: it decides the elements' values while it watches your comparisons, so unless your pivot rule truly has a guarantee, it forces you into Θ(n²). A random pivot doesn't pass either.

The input may be all equal, or may contain only a few distinct values.

## Stage 3 — Real-world extension: weighted median

```cpp
struct Sample {
    std::int64_t value;
    std::int64_t weight;
};

std::int64_t weighted_median(std::span<const Sample> samples);
```

Monitoring systems often receive histograms that are already aggregated, where each record says "this value occurred this many times".

`weighted_median(samples)`: let `W` be the sum of all the `weight`s. Return the **smallest** `v` for which the total weight `S` of the samples with `value ≤ v` satisfies `2S ≥ W`. The total weight is guaranteed to fit in `int64`, but `2S` may not, so be careful how you compare.

Throw `std::invalid_argument` if there are no samples or if any sample has `weight ≤ 0`. **O(n)**; you must not sort first.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why groups of 5? Why doesn't the recurrence for groups of 3 solve to linear time? What about groups of 7?
2. This algorithm has a large constant factor, and in measurements it can even be slower than sorting. So why does it exist? How does `nth_element` get both average-case speed and a worst-case guarantee in practice? (Hint: introselect.)
3. Why can the weighted median discard half of the samples in each round? If the pivot is not the true median, what does the total complexity become?
