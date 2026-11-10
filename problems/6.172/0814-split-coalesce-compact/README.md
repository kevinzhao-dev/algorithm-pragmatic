---
title: Splitting, Coalescing, and Compaction (a two-size heap)
unit: 08 · Storage Allocation (HW6)
source: 6.172 HW6 §6 Write-up 7–8 (split/coalesce with two sizes)
pdf: materials/6.172/homework/hw06/handout.pdf
read: materials/6.172/lectures/lec11.pdf
kind: code
minutes: 80
---

# 0814 · Splitting, Coalescing, and Compaction

**Original problem:** [HW6: Simple Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e8e29570ad0673ec3460af46536a1e04_MIT6_172F18hw6.pdf) · **Read first:** [Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/68624442da6047731a33b3590cefd030_MIT6_172F18_lec11.pdf)

> **Why this comes up in practice:** whether an allocator's free lists can merge scattered small blocks back into large ones decides whether, after a long run, it "fails to allocate even though memory is still free". Once objects can be moved, compaction becomes possible, which is exactly the basis of copying GC and of the way Go resizes stacks.

## Original problem

6.172 HW6 §6 ends with two questions about the allocator for large (64-byte) and small (32-byte) objects. Neither requires an implementation:

- Write-up 7: what can you do when you need a 32-byte object but the free list holds only 64-byte entries? What happens once you have run out of space and keep breaking up large objects but never coalesce adjacent small ones back into large ones? And what can you do when you are out of space and need a 64-byte object while the free list holds only 32-byte entries?
- Write-up 8: allocations must be contiguous in memory. Can two 32-byte objects that are not adjacent be coalesced into a 64-byte object? If so, how would you implement it?

## Your task

Keep the two sizes from the previous exercise: **large = 64 bytes** and **small = 32 bytes**. `TwoSizeHeap` manages an arena of `lines` 64-byte lines, using only offsets and never touching real memory. Initially every line is on the large free list.

The homework only asks you to reason about these questions; here you implement each answer, and the tests make its effects visible.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Splitting without coalescing (first half of Write-up 7)

```cpp
enum class Coalesce { Never, Adjacent };

// offsets only; all lines start out free
class TwoSizeHeap {
public:
    TwoSizeHeap(std::size_t lines, Coalesce policy);

    std::optional<std::size_t> alloc_small();
    std::optional<std::size_t> alloc_large();
    void free_small(std::size_t offset);
    void free_large(std::size_t offset);
    std::size_t free_bytes() const;
};
```

`TwoSizeHeap heap(lines, Coalesce::Never)`:

- `alloc_large()`: take an idle line and return its offset (a multiple of 64). If there is none, return `std::nullopt`.
- `alloc_small()`: use the small free list first; otherwise take an idle line and **split it in half**, return the lower half, and put the upper half on the small free list. If neither is available, return `std::nullopt`.
- `free_small(offset)` / `free_large(offset)`: put the block back on the matching free list, **without any coalescing**.
- `free_bytes()`: the total number of bytes currently free.

The tests let you see for yourself the effect the homework asks about: when you keep splitting without ever coalescing, `free_bytes` is still large, yet no large block can be allocated.

## Stage 2 — Coalescing adjacent halves (second half of Write-up 7)

`Coalesce::Adjacent` mode: on `free_small`, if the other half of the same line is also idle, merge the two halves right away back into an idle large block, which `alloc_large` can then use again. Everything else behaves the same, and the `Coalesce::Never` tests must keep passing.

## Stage 3 — Non-adjacent halves: moving objects through handles (Write-up 8)

```cpp
// objects live in real memory and can be moved; callers hold handles
class CompactingHeap {
public:
    using Handle = std::size_t;

    explicit CompactingHeap(std::size_t lines);

    std::optional<Handle> alloc_small();
    std::optional<Handle> alloc_large();
    void free(Handle handle);
    std::span<std::byte> deref(Handle handle);
    void compact();
    std::size_t free_bytes() const;
};
```

Two non-adjacent 32-byte blocks can't simply be combined into 64 bytes unless objects can be moved. `CompactingHeap` refers to objects indirectly through handles, so an object's real location can change:

- `alloc_small()` / `alloc_large()` return `std::optional<Handle>`.
- `deref(handle)` returns the object's current `std::span<std::byte>` (32 bytes for small, 64 bytes for large). A span obtained earlier becomes invalid after the next operation that can move objects.
- `free(handle)`.
- `compact()`: pair up the "lines with only one half in use", and move the object on one line of each pair into the empty half of the other, so that as many lines as possible become entirely free. Object contents must not change, and handles must not become invalid. Afterwards at most one line is half full.
- When there is no wholly idle line but enough free space overall, `alloc_large()` must `compact()` automatically and then retry.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. If you only split and never coalesce, how much memory can be wasted in the worst case? Why does a real buddy system (0803) merge only buddies and not any two adjacent blocks?
2. Why can't C/C++'s `malloc` move objects the way `CompactingHeap` does? Which languages or settings can? What does it cost?
3. A handle adds a level of indirection, so every access costs one more memory load. When is that cost worth paying? (Hint: Lecture 11's copying GC)
