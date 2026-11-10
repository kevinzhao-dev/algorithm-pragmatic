---
title: Binned Free Lists and Buddy Coalescing
unit: 08 · Storage Allocation (HW6)
source: 6.172 Lecture 11 Allocation for Binned Free Lists, Coalescing
pdf: materials/6.172/lectures/lec11.pdf
kind: code
minutes: 90
---

# 0803 · Binned Free Lists and Buddy Coalescing

**Based on:** [Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/68624442da6047731a33b3590cefd030_MIT6_172F18_lec11.pdf)

> **Why this comes up in practice:** the Linux kernel's page allocator is exactly this buddy system. The core problem is a classic one: when a bin runs out, you must split a larger block, and later you must merge adjacent blocks back together (coalesce) without adding much cost, or external fragmentation will eat up all the memory.

## Your task

Build an allocator that works only with offsets and never touches real memory: the whole arena has 2^`max_order` units. A block of order k is 2^k units long, and its start is always a multiple of 2^k. Implement it with Lecture 11's binned allocation scheme.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Allocating and splitting

```cpp
class BuddyAllocator {
public:
    explicit BuddyAllocator(int max_order);

    std::optional<std::size_t> allocate(std::size_t units);
    std::size_t free_blocks(int order) const;
    std::size_t free_units() const;
};
```

`allocate(units)`: first find the smallest order k that can hold `units`.

- If bin k has a free block, use it directly.
- Otherwise, find the smallest nonempty bin k′ > k and keep halving its block: after each split, put the **upper half** into the next smaller bin and keep splitting the **lower half** until you reach order k, then return that lower block of order k.
- Return `std::nullopt` when the arena has no block large enough.

`units == 0` throws `std::invalid_argument`. `free_blocks(order)` returns the number of blocks in that bin, and `free_units()` returns the total number of units currently free.

Each bin must use a structure that can insert and remove a given offset in O(log n).

## Stage 2 — Freeing and coalescing

```cpp
// in class BuddyAllocator
void free(std::size_t offset);
```

`free(offset)`: `offset` is a value that `allocate` returned and that has not been freed yet. When it is freed, if its **buddy** (the block of the same order next to it that, together with it, forms a larger block) is also free, merge the two immediately and keep checking upward until no more merging is possible.

Coalescing must be O(log n); no linear scans. Once everything has been freed, in any order, the arena must be back to a single whole max-order block.

A `PERF` test runs 1.2 million random allocations and frees on an arena of 2^40 units, where a linear scan of the arena or of the free blocks would take minutes.

## Stage 3 — Fragmentation metrics

```cpp
// in class BuddyAllocator
std::size_t largest_free_block() const;
double external_fragmentation() const;
```

- `largest_free_block()`: the size in units of the largest free block right now (return 0 if nothing is free).
- `external_fragmentation()`: `1 - largest / free_units`, defined as 0 when everything is allocated.

This stage's tests build several allocation patterns and check that the metrics are computed correctly. Think about why the buddy system can't help with a checkerboard pattern either.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why can a buddy's offset be computed with a single bit operation? How does that relate to "a block's start must be a multiple of its own length"?
2. From Lecture 11's analysis: the external fragmentation of binned free lists is bounded by what multiple of the maximum memory in use? Why is coalescing useful in practice even though it doesn't change the worst-case bound?
3. Linux stacks a slab layer on top of its buddy allocator. What problem does each layer solve?
