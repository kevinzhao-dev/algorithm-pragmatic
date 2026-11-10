---
title: Cache Line Alignment vs Tight Packing (an allocation-strategy experiment)
unit: 08 · Storage Allocation (HW6)
source: 6.172 HW6 §5–§6 Write-up 1–3 (cache-friendly allocation)
pdf: materials/6.172/homework/hw06/handout.pdf
read: materials/6.172/lectures/lec11.pdf
kind: code
minutes: 80
---

# 0812 · Cache Line Alignment vs Tight Packing

**Original problem:** [HW6: Simple Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e8e29570ad0673ec3460af46536a1e04_MIT6_172F18hw6.pdf) · **Read first:** [Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/68624442da6047731a33b3590cefd030_MIT6_172F18_lec11.pdf)

> **Why this comes up in practice:** should small, frequently read objects be aligned to cache lines? The intuitive answer is "yes", but if an object is only 24 bytes, aligning it wastes most of every line, and the overall cache footprint actually grows. This exercise uses a deterministic cache model to verify that trade-off by hand.

## Original problem

6.172 HW6 §5 (cache-friendly allocation) starts from the observation that aligning an object to a cache-line boundary keeps down the number of lines an access must touch, which matters most when objects are accessed at random. In this section objects are never freed, and the allocators are evaluated on `traces/trace_c0_nofree`.

- Write-up 1: inside `wrapped_malloc()`, wrap a call to `unaligned_malloc()` so that every object starts on a cache-line boundary, without assuming anything about the inner allocator's state. Then argue, from your code and from the utilization the trace driver reports, how much memory aligned allocation wastes.

The handout then points out that needing fewer lines per object can still mean more cache loads overall when the cache is used inefficiently, and turns to a packed allocator for a single-threaded workload: several objects may share a line, but each object must still be 8-byte aligned and span as few lines as possible, and every block keeps its size header next to it.

- Write-up 2: implement `packed_malloc()`. Where did you put the block header? Report the utilization and performance scores, and how much memory the allocator wastes overall.

§6 (allocator overheads):

- Write-up 3: is the header still needed if, instead of the usual `free(p)`, programmers had to pass the original size, as in `free(p, size)`?

## Your task

The constant is `kCacheLine = 64`. This exercise uses only the interface in `solution.hpp`, and the cache model comes from `_shared/08_cache_sim.hpp` (`#include "08_cache_sim.hpp"`; read the interface comments of its `Cache` directly).

⚠️ **Spoiler warning:** read only those interface comments. The implementation in `_shared/08_cache_sim.hpp` is the solution to 1101 and much of 1103, so skip it until you've done those.

Instead of modifying C allocators and running them through the trace driver, you build the aligned wrapper and the packed allocator in C++ and then compare the two layouts on a simulated cache.

Each stage lists the declarations it adds, copied from `solution.hpp`.

## Stage 1 — Wrapping any allocator to align to cache lines

```cpp
inline constexpr std::size_t kCacheLine = 64;

// Upstream provides void* allocate(std::size_t) (8-byte aligned) and void deallocate(void*).
template <typename Upstream>
class CacheAlignedWrapper {
public:
    explicit CacheAlignedWrapper(Upstream& upstream);

    void* allocate(std::size_t bytes);
    std::size_t overhead_bytes() const;
};
```

`CacheAlignedWrapper<Upstream>`: `Upstream` provides only `void* allocate(std::size_t)` (its return value is aligned to 8, and beyond that you **must not assume** anything about its state).

- `allocate(bytes)`: the returned pointer is aligned to 64, and `[p, p + bytes)` lies entirely within the range the upstream gave you. `bytes == 0` returns `nullptr` without calling the upstream.
- Each call makes exactly one request to the upstream, and the requested size must not exceed `bytes + 63 + sizeof(void*)`.
- `overhead_bytes()`: the total number of extra bytes requested from the upstream so far (beyond what the users asked for).

## Stage 2 — Supporting deallocation

```cpp
// in class CacheAlignedWrapper<Upstream>
void deallocate(void* p);
```

`deallocate(p)`: hand the **original pointer** that the upstream gave you back to `Upstream::deallocate(void*)`. `nullptr` does nothing. Where is the original pointer stored? Given only `p`, how do you find it again?

## Stage 3 — Tight packing: `PackedAllocator`

```cpp
class PackedAllocator {
public:
    explicit PackedAllocator(std::span<std::byte> heap);

    void* allocate(std::size_t bytes);
    std::size_t size_of(const void* p) const;
    std::size_t used_bytes() const;
};
```

Relax the cache-line alignment requirement: objects may share cache lines, but each object must span as few lines as possible.

`PackedAllocator(std::span<std::byte> heap)`: bump-allocate within `heap`, with an 8-byte **size header** in front of each object:

- `allocate(bytes)`: objects are aligned to 8, and the header sits immediately before the object's start. Return `nullptr` if `bytes == 0` or the heap doesn't have enough room.
- The number of cache lines an object spans must equal `⌈bytes / 64⌉`. If that's not possible where it is, move the object forward to the start of the next line (the header may land at the end of the previous line).
- `size_of(p)`: return the originally requested size. `used_bytes()`: the current usage (the high-water mark).

Compute line boundaries from **real addresses** (the start of `heap` is not necessarily aligned to 64).

## Stage 4 — The experiment

```cpp
struct PlacementStats {
    double lines_per_access = 0.0;
    std::uint64_t misses = 0;
};

inline PlacementStats measure_placement(std::span<const std::uint64_t> addresses,
                                        std::span<const std::size_t> sizes,
                                        std::span<const std::size_t> order,
                                        practice::cachesim::Cache& cache);
```

Implement `measure_placement(addresses, sizes, order, cache)`: in the sequence given by `order`, access each object once over its whole range (`Cache::access_range`), and return `PlacementStats`:

- `lines_per_access`: the average number of cache lines one access spans.
- `misses`: the number of misses this run **adds** to `cache`.

The tests lay out the objects with your Stage 1 and Stage 3 allocators and compare how the "aligned" and "packed" placements perform on identical caches.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. On average, how many bytes does the Stage 1 approach waste? The smaller the object, the worse this ratio gets; why should you be careful with calls like `aligned_alloc(64, 8)`? (Homework Write-up 1)
2. In the scenario the Stage 4 tests use, the aligned version actually misses more. Explain why: when does alignment win, and when does tight packing win?
3. If `PackedAllocator`'s header doesn't go in front of the object, where else could it go and still work? (Homework Write-up 2) If the allocator switched to a `free(p, size)` interface, would the header still be needed? (Homework Write-up 3)
