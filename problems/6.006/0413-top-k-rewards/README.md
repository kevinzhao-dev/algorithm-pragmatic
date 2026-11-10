---
title: Top-K (three ways to use a heap)
unit: 04 · Trees & Heaps (PS4)
source: 6.006 PS4 Problem 4-3
pdf: materials/6.006/psets/ps4/questions.pdf
answer: materials/6.006/psets/ps4/solutions.pdf
read: materials/6.006/lectures/lec08.pdf
kind: code
minutes: 50
---

# 0413 · Top-K

**Original problem:** [Problem Set 4 Questions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/bff0aaeeb571a77083f10fd8e31f71d4_MIT6_006S20_ps4-questions.pdf) · **Read first:** [Lecture 8: Binary Heaps notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/40d4851e550507ca14dc778b9b2266cc_MIT6_006S20_lec8.pdf)

> **Why this comes up in practice:** a leaderboard's top 100, re-ranking search results, and a monitoring system's top-N slow queries never need the whole data set sorted. The only difference is whether the data is handed to you all at once or keeps flowing in.

## Original problem

6.006 PS4 Problem 4-3 is about a nationwide gardening contest. Each garden is a pair (s_i, r_i): s_i is its score, a positive integer, and r_i is its registration number, a positive integer that no other garden shares.

- (a) The top k gardens each win an identical trophy. From an unsorted array A of garden pairs and an integer k with 1 ≤ k ≤ |A|, find in O(|A| + k log |A|) time the registration numbers of k gardens with the highest scores (ties may be broken either way).
- (b) Instead, every garden that scores strictly above a reference score x wins. With the garden pairs stored as a max-heap A, report the registration numbers of all those gardens in O(n_x) time, where n_x is the size of the answer.

## Your task

An online content platform wants to hand out rewards across a batch of content. Each `Garden<Score>` (a piece of content) has a `score` and an `id`. `Score` is only guaranteed to have `operator<`.

You should `#include "../0405-binary-heap/solution.hpp"` and reuse the `BinaryHeap` you wrote. Stages 1 and 2 are parts (a) and (b), and Stage 3 adds a streaming tracker that the original doesn't have.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — One batch: take the top k

```cpp
template <typename Score>
struct Garden {
    Score score;
    int id;
};

template <typename Score>
std::vector<int> top_k_ids(std::span<const Garden<Score>> gardens, std::size_t k);
```

`top_k_ids(gardens, k)`: with `0 ≤ k ≤ gardens.size()`, return the `id`s of the `k` entries with the highest scores, in any order (break ties any way you like).

**O(n + k log n)**: cheaper than a full sort or than calling `push` on the items one at a time, both of which cost `n log n`. The tests count comparisons against a limit of `3n + 4k⌈lg n⌉`, and a full sort exceeds it.

## Stage 2 — Everything above a threshold

```cpp
template <typename Score>
std::vector<int> ids_above(std::span<const Garden<Score>> heap, const Score& threshold);
```

`ids_above(heap, x)`: `heap` is already a **max-heap** ordered by `score` (in array form). Return the `id` of every entry with `score > x`, in any order. **O(k)**, where `k` is the number of results (the tests allow at most `3k + 8` comparisons); you must not scan the whole heap.

Think about the heap property: if a node's score is `≤ x`, you don't need to look at any of its subtree. The number of nodes you actually visit is at most about twice the size of the answer.

## Stage 3 — Real-world extension: streaming Top-K Tracker

```cpp
template <typename T, typename Compare = std::less<T>>
class TopKTracker {
public:
    explicit TopKTracker(std::size_t k, Compare compare = Compare{});

    void offer(T value);
    std::size_t size() const;
    const T* threshold() const;
    std::vector<T> sorted_largest() const;
};
```

Data arrives one item at a time and may never end, so you can't store all of it. `TopKTracker<T, Compare>`:

- `explicit TopKTracker(k, compare = {})`: keep only the k largest items (according to `compare`). `k` may be 0.
- `offer(v)`: O(log k), and when `v` can't possibly make the top k, it must give up in O(1) (with a single comparison). The tests allow `3⌈lg(k + 1)⌉ + 6` comparisons for a value that gets in.
- `size()`: how many items are currently kept, at most `k`.
- `threshold()`: the smallest of the k items kept (as a pointer; return `nullptr` until the tracker is full, and always when `k` is 0). A new element must be larger than it to have a chance of getting in.
- `sorted_largest()`: the items currently kept, from largest to smallest, in O(k log k).

You may use only O(k) memory: just stuffing everything into a big vector won't pass the `Tracked` check, which allows at most `k + 2` elements alive at once.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Stage 1: why not maintain a min-heap of size k from the start? How does its running time differ from the build-heap approach?
2. Why does Stage 2 save a log factor compared with repeated `pop`s? If `x` is very small and `k` is close to `n`, which approach is better?
3. Stage 3: `offer` goes from O(log k) to O(1). Why isn't that the case that speeds things up the most? (Think about which input is the worst case.)

---

**Official solution:** [Problem Set 4 Solutions](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/0462839ce975fea99f6a7b8adc8e1477_MIT6_006S20_ps4-solutions.pdf). Read it after you finish.
