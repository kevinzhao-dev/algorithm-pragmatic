---
title: Binary Heap and In-Place Heapsort
unit: 04 · Trees & Heaps (PS4)
source: 6.006 Lecture 8, Recitation 8
pdf: materials/6.006/recitations/rec08.pdf
read: materials/6.006/lectures/lec08.pdf
kind: code
minutes: 75
---

# 0405 · Binary Heap and In-Place Heapsort

**Based on:** [Recitation 8 notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/701c2f339890cbf9c0c2a3495d007f26_MIT6_006S20_r08.pdf) · **Read first:** [Lecture 8: Binary Heaps notes](https://ocw.mit.edu/courses/6-006-introduction-to-algorithms-spring-2020/40d4851e550507ca14dc778b9b2266cc_MIT6_006S20_lec8.pdf)

> **Why this comes up in practice:** schedulers, Dijkstra, TCP reassembly buffers, and Top-K are all priority queues. Linux's hrtimer uses a red-black tree, but the Go runtime's timers are a 4-ary heap; the cache behavior of an implicit tree in an array is something pointer-based structures can't match.

## Your task

Implement a binary heap, then an in-place heapsort, and finally a buffer that puts slightly late events back in order.

Treat an array as an implicit complete binary tree (`parent(i) = (i-1)/2`, `left(i) = 2i+1`, `right(i) = 2i+2`) with a `std::vector` underneath. Do not use `std::priority_queue`, `std::push_heap`, and the like.

`BinaryHeap<T, Compare>` is a **max-heap** (with respect to `Compare`: `top()` is the element that `Compare` considers largest); pass `std::greater<>` and it becomes a min-heap.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Basic operations

```cpp
template <typename T, typename Compare = std::less<T>>
class BinaryHeap {
public:
    explicit BinaryHeap(Compare compare = Compare{});

    void push(T value);
    const T& top() const;
    T pop();
    std::size_t size() const;
    bool empty() const;
    std::span<const T> items() const;
};
```

- `push(v)`: O(log n).
- `top()` / `pop()`: get/remove the top of the heap (`pop` returns the value). Throw `std::out_of_range` on an empty heap. `pop` is O(log n); one `push` plus one `pop` may make at most `3⌈lg n⌉ + 8` comparisons in total (the tests count).
- `size()` / `empty()`.
- `items()`: the underlying array (as a read-only `std::span`); `items()[0]` is the top. The tests use it to check the heap property.

Elements may be move-only; `pop` returns by move.

## Stage 2 — Building a heap in O(n)

```cpp
// in class BinaryHeap<T, Compare>
explicit BinaryHeap(std::vector<T> items, Compare compare = Compare{});
```

`explicit BinaryHeap(std::vector<T> items, Compare = {})`: build the heap in O(n) by sifting down **from the bottom up**. It must make no more than `3n` comparisons (pushing the items one at a time takes O(n log n), and the tests count).

First think about why building with sift-down is O(n) while sift-up is not: leaves make up half the nodes, and they barely have to move.

## Stage 3 — In-Place Heapsort

```cpp
template <std::random_access_iterator It, typename Compare = std::less<>>
void heapsort(It first, It last, Compare compare = Compare{});
```

`heapsort(first, last, compare = {})`: sort a random access iterator range into ascending order according to `compare`.

- O(1) extra space: you must not copy the elements into another container (the tests use `Tracked` to watch the number of live objects, and allow at most 4 extra at any moment). Stability is not required.
- O(n log n) comparisons (at most `2n⌈lg n⌉ + 3n` in the tests).
- You may share code with the sift functions from Stage 1: split the heap operations into free functions that take iterators, and they can serve both `BinaryHeap` and `heapsort`.

## Stage 4 — Real-world extension: reordering out-of-order events

```cpp
template <typename T, typename Compare = std::less<T>>
class ReorderBuffer {
public:
    explicit ReorderBuffer(std::size_t lateness, Compare compare = Compare{});

    std::optional<T> push(T item);
    std::vector<T> drain();
    std::size_t buffered() const;
};
```

Events in a stream may arrive a little late (network packets, merged log streams). `ReorderBuffer<T, Compare>`:

- `explicit ReorderBuffer(lateness)`: every event is guaranteed to be at most `lateness` positions away from its sorted position (`lateness = 0` means the stream is already sorted).
- `push(item)`: add an event. When the buffer holds more than `lateness` events, return the earliest event it currently holds (the smallest according to `Compare`); otherwise return `std::nullopt`.
- `drain()`: when the stream ends, return the events still in the buffer, in order.
- `buffered()`: the number of events currently buffered, which never exceeds `lateness`.

Concatenating all the outputs must give the complete sorted result. Each `push` makes O(log(lateness + 1)) comparisons (not O(log n); the tests allow `3⌈lg(lateness + 2)⌉ + 6` per event, `drain` included); that is the payoff of keeping the heap at only `lateness + 1` elements.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. How long does it take to find the smallest element of a max-heap? Why can't it be faster?
2. How do heapsort's constant factors and cache behavior differ from mergesort's and quicksort's? Why doesn't std::sort use heapsort as its workhorse, yet uses it as a fallback?
3. To support `decrease-key`, what is this structure still missing? (The next exercise.)
