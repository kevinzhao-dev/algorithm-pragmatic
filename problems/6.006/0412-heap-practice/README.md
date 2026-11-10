---
title: Heap Practice (classification and swap traces)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 PS4 Problem 4-2
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec08.pdf, materials/6.006/recitations/rec08.pdf
kind: code
minutes: 30
---

# 0412 · Heap Practice

**Original problem:** [Problem Set 4 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bff0aaeeb571a77083f10fd8e31f71d4_MIT6_006S20_ps4-questions.pdf) · **Read first:** [Lecture 8: Binary Heaps notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/40d4851e550507ca14dc778b9b2266cc_MIT6_006S20_lec8.pdf), [Recitation 8 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/701c2f339890cbf9c0c2a3495d007f26_MIT6_006S20_r08.pdf)

> **Why this comes up in practice:** heap bugs usually aren't crashes; the order just quietly goes wrong. This exercise practices turning "is this array a heap?" into a mechanical check, and along the way practices every step of building a heap.

## Original problem

6.006 PS4 Problem 4-2 gives four arrays to read as complete binary trees. A binary tree is complete when every depth i except possibly the deepest holds exactly 2^i nodes, and the nodes at the deepest level are packed as far left as possible.

- (a) [4, 12, 8, 21, 14, 9, 17]
- (b) [701, 253, 24, 229, 17, 22]
- (c) [2, 9, 13, 8, 0, 2]
- (d) [1, 3, 6, 5, 4, 9, 7]

For each array, draw the tree and say whether it is a max-heap, a min-heap, or neither. If it is neither, convert it into a min-heap purely by exchanging parent-child pairs, and record the exchanges as a series of drawings, each highlighting the two items just swapped.

## Your task

Treat an array as a complete binary tree (`left(i) = 2i+1`, `right(i) = 2i+2`). Once you're done, you can check your work on the arrays from the Original problem section. Stage 1 classifies an array, Stage 2 records the swaps that turn it into a min-heap, and Stage 3 goes beyond the original with two more questions about max-heaps.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Classification

```cpp
enum class HeapKind { Max, Min, Both, Neither };

inline HeapKind classify_heap(std::span<const int> values);
```

`classify_heap(values)`: return `HeapKind::Max`, `Min`, `Both` (an empty array, a single element, or all elements equal), or `Neither`. Equal values are allowed; for example, `[5, 5, 3]` is a max-heap.

## Stage 2 — Build a min-heap and record the swaps

```cpp
using SwapStep = std::pair<std::size_t, std::size_t>;

inline std::vector<SwapStep> min_heapify_trace(std::span<int> values);
```

`min_heapify_trace(values)`: turn `values` into a min-heap **in place**, and return the index pair `(i, j)` of every swap.

- The two positions in each swap must be adjacent in the tree (one is the parent of the other), and each swap must be an actual swap on the array as it stands at that moment. In other words, applying **only** these swaps to the original array produces the result.
- If the array is already a min-heap, return an empty list.
- The number of swaps must not exceed `n` (`n = values.size()`).

The tests replay your trace on the original array to verify it.

## Stage 3 — The minimum, and conversion

```cpp
template <typename T>
const T& min_of_max_heap(std::span<const T> heap);

template <typename T>
void max_heap_to_min_heap(std::span<T> heap);
```

- `min_of_max_heap(heap)`: `heap` is a max-heap; return a reference to one of its smallest elements (throw `std::invalid_argument` if it is empty). Make no more than `⌈n/2⌉` comparisons. Think about which positions the minimum can be in.
- `max_heap_to_min_heap(heap)`: turn a max-heap into a min-heap with O(n) comparisons (at most `3n`). (Does knowing that it's a max-heap help you?)

The template parameter `T` only needs to support `<`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. In Stage 2, how much do the swap counts differ between bottom-up sift-down and sifting up one element at a time?
2. Why can't `min_of_max_heap` get by with fewer than about `n/2` comparisons?
3. When you convert a max-heap into a min-heap, why doesn't the known max-heap structure help at all?

---

**Official solution:** [Problem Set 4 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0462839ce975fea99f6a7b8adc8e1477_MIT6_006S20_ps4-solutions.pdf). Read it after you finish.
