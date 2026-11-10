---
title: Small-Object Bins (size-class allocator)
unit: 08 · Storage Allocation (HW6)
source: 6.172 Lecture 11 Variable-Size Allocation (binned free lists)
pdf: materials/6.172/lectures/lec11.pdf
kind: perf
minutes: 90
---

# 0802 · Small-Object Bins

**Based on:** [Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/68624442da6047731a33b3590cefd030_MIT6_172F18_lec11.pdf)

> **Why this comes up in practice:** the fast paths of tcmalloc, jemalloc, and mimalloc all have this structure: round the request size up to a size class and keep one free list per class. Once you understand it, you can explain why "allocating 33 bytes actually gets you 48" and why sized delete can save a header.

## Your task

Build a small-object allocator with power-of-two bins: 9 bins of 16, 32, …, 4096 bytes, one free list per bin, and each chunk is carved up for a single bin only. Requests larger than 4096 bytes go straight to `::operator new`.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — Size classes

```cpp
// Where raw chunks come from. The default forwards to operator new.
struct NewDeleteUpstream {
    void* allocate(std::size_t bytes) { return ::operator new(bytes); }
    void deallocate(void* p, std::size_t /*bytes*/) noexcept { ::operator delete(p); }
};

template <typename Upstream = NewDeleteUpstream>
class BinnedAllocator {
public:
    static constexpr std::size_t kBinCount = 9;
    static constexpr std::size_t kMaxBinSize = 4096;

    static std::size_t bin_index(std::size_t size);
    static std::size_t bin_size(std::size_t index);
};
```

`bin_index(size)`: return the smallest bin that can hold `size` (the 16-byte bin is 0 and the 4096-byte bin is 8). `size` ranges from 1 to 4096. `bin_size(index)` returns that bin's block size. Both must be O(1).

## Stage 2 — Allocating and freeing

```cpp
// in class BinnedAllocator<Upstream>
explicit BinnedAllocator(std::size_t chunk_bytes = 1 << 16, Upstream upstream = Upstream{});

BinnedAllocator(const BinnedAllocator&) = delete;
BinnedAllocator& operator=(const BinnedAllocator&) = delete;

void* allocate(std::size_t size);
void deallocate(void* p, std::size_t size);
```

- `allocate(size)` (1 ≤ size ≤ 4096): take a block from the matching bin's free list; if it is empty, carve one from this bin's current chunk; if that chunk is used up too, request a new one from the upstream, of the size `chunk_bytes` given to the constructor.
- `deallocate(p, size)`: `size` must equal the argument originally passed to `allocate`, so blocks **need no header**. Put the block back in its bin in O(1).
- Returned pointers are aligned to 16. Every bin's free list is LIFO.

Blocks of different bins must never overlap, and a block returned to one bin must not be used to serve another bin.

Keep any per-chunk header small: with 1 KiB chunks, 100 allocations of 100 bytes may request at most 16 chunks from the upstream.

## Stage 3 — Large requests and edge cases

```cpp
// in class BinnedAllocator<Upstream>
~BinnedAllocator();  // returns every chunk to the upstream
std::size_t large_allocations() const;
std::size_t live_bytes() const;
```

- `size > 4096`: allocate goes through `::operator new` and deallocate (deciding by size) through `::operator delete`; `large_allocations()` returns how many times this path has been taken. Large requests never touch the upstream.
- `size == 0`: treat it as 1 byte. The returned pointer can be deallocated normally, and the matching `deallocate(p, 0)` must not fail. The block comes from the 16-byte bin, but the request adds 0 to `live_bytes()`.
- The destructor returns every chunk to the upstream.
- `live_bytes()`: the total, over all requests not yet returned, of **the bytes the callers asked for** (not the block sizes).

A `PERF` test allocates a million 32-byte blocks and then frees them all; this must take less than one second, so allocation must not slow down as the number of live blocks grows.

## Stage 4 — Measurement

No new interface. Within the same executable, a `PERF` test runs a workload of small objects of mixed sizes through your allocator and through `::operator new/delete`; yours must be more than 2 times faster (Release only).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. How large a gap between Stage 3's `live_bytes()` and the bytes the upstream handed out counts as "waste"? What is the upper bound on internal fragmentation? Are power-of-two bins the best choice?
2. Lecture 11's analysis says that binned free lists may use O(M log M) virtual memory. Why? How does that relate to external fragmentation?
3. Why is C++14's sized deallocation a plus for this kind of allocator? If a caller passes the wrong size, when and in what form does it go wrong?
