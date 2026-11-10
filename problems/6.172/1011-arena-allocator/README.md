---
title: Arena Allocator (bump → implicit → binned)
unit: 10 · Memory Allocator (Project 3)
source: 6.172 Project 3
pdf: materials/6.172/projects/p3/handout.pdf
read: materials/6.172/lectures/lec11.pdf, materials/6.172/lectures/lec12.pdf, materials/6.172/homework/hw06/handout.pdf, materials/6.172/recitations/r6/problems.pdf
kind: project
minutes: 780
---

# 1011 · Arena Allocator

**Original problem:** [Project 3: Serial Dynamic Memory Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/d2fb8ee337632ab3e256fa1704375053_MIT6_172F18_project3.pdf) · **Read first:** [Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/68624442da6047731a33b3590cefd030_MIT6_172F18_lec11.pdf), [Parallel Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/24c247c943024dcda40bcc7caa206581_MIT6_172F18_lec12.pdf), [HW6: Simple Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e8e29570ad0673ec3460af46536a1e04_MIT6_172F18hw6.pdf), [R6: Simple, Straightforward Memory Allocation Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/7f2c864d6f81e8f98da02ee657202518_MIT6_172F18_rec6.pdf)

> **Why this comes up in practice:** a service's peak memory is often high not because there really is that much data, but because of fragmentation plus metadata. A 20% difference in an allocator's utilization on the same trace is a 20% difference in the server bill, and on-call incidents where the resident set blows up often end up being traced back here.

## Original problem

6.172 Project 3 (Serial Dynamic Memory Allocation) asks for a single-threaded allocator in C that keeps libc's semantics while being both quick and frugal with memory, tested on traces recorded from real applications.

- Interface (§3): `my_init()` prepares the heap and returns 0 on success or −1 on failure. `my_malloc(size)` returns an 8-byte-aligned block of at least `size` bytes that lies inside the heap and overlaps no other live block, or `NULL` when `size` is 0 or the request can't be met. `my_free(ptr)` releases a block returned earlier, and freeing `NULL` does nothing. `my_realloc(ptr, size)` acts like `my_malloc(size)` when `ptr` is `NULL` and like `my_free(ptr)` when `size` is 0; otherwise it returns a block of at least `size` bytes that holds the old contents, truncated if the new block is smaller. The starting code implements realloc as malloc, copy, and free.
- Heap checker (§4): write `check()`, which scans the heap and returns 0 exactly when it is consistent and −1 otherwise. Possible checks: blocks on the free list are all marked free, no adjacent free blocks escaped coalescing, each free block appears on the free list, free-list pointers lead to valid free blocks, allocated blocks never overlap, and pointers stored in heap blocks stay within the heap.
- Support routines (§5): heap memory comes only from a simulated memory system. `mem_sbrk(incr)` extends the heap like `sbrk` (positive increments only), and `mem_heap_lo()`, `mem_heap_hi()`, `mem_heapsize()`, and `mem_pagesize()` describe it.
- External validator (§6): for cross-testing the other teams' allocators, complete a black-box validator that checks that, on the given traces, malloc and realloc never return `NULL`, every returned range is 8-byte aligned, lies inside the heap, and is disjoint from the others, and realloc keeps the old data up to the new size. You write the add and remove operations of its list of ranges.
- Driver (§7): `mdriver` replays trace files of allocate, reallocate, free, and write commands and reports correctness, utilization, and throughput; its `-c` option runs your checker after every operation.
- Autotuning (§8, optional): OpenTuner can search your allocator's parameters, but tuning on too narrow a set of inputs overfits.
- Rules (§10): change only the allocator, validator, and tuning-parameter files; make no memory-management library or system calls (`malloc`, `calloc`, `free`, `realloc`, `sbrk`, `brk`, or their variants); use no parallelism; keep global and static data to at most 512 bytes in total; and take the memory for every data structure from your own heap, where it counts against utilization.
- Evaluation (§11): space utilization U = max(M, 40KB) / max(H, 40KB), taken at its peak, where M is the memory allocated and not yet freed and H is the heap size; throughput T is the average number of operations per second; and the performance index is P = exp(w log U + (1 − w) log(min(1, T/Tlibc))) with w = 0.5, where Tlibc is libc's throughput on the default traces.

There are no guiding questions this time. The write-up describes your data structures and how each call manipulates them, the options you tried and dropped, the trade-offs you made, and your autotuning parameters along with the performance before and after tuning.

## Your task

Implement three generations of allocator on a fixed arena obtained with `mmap`, all with `malloc` / `free` / `realloc` semantics, and then measure space and speed with traces. `support/` is ready: `trace.hpp` (seeded traces in seven workload shapes) and `replay.hpp` (a black-box validator that writes a pattern into every byte and verifies it, catching overlap, misalignment, and data loss).

Unlike the original, which gives you one C allocator to optimize on top of a simulated `sbrk`, you build three allocators in turn (bump, implicit free list, binned free lists) on a real `mmap` arena, with a block layout that the tests read directly, and pointers must be 16-byte aligned instead of 8. The validator is provided rather than written by you.

Write your code in `arena.hpp`, `bump.hpp`, `implicit.hpp`, and `binned.hpp`; `solution.hpp` only includes them. Each stage lists the declarations it adds, copied from those files.

## The contract all allocators share

- `malloc(n)`: return a 16-byte-aligned pointer; `n == 0` returns `nullptr`; if the request doesn't fit, return `nullptr` and leave no trace. `n` may be close to `SIZE_MAX`, so **watch out for overflow**.
- `free(p)`: `p == nullptr` has no effect.
- `realloc(p, n)`: `p == nullptr` is the same as `malloc(n)`; `n == 0` is the same as `free(p)` and returns `nullptr`; the first `min(old size, n)` bytes must be preserved; on failure, return `nullptr` and leave `p` intact (and still allocated).
- `check() const`: a consistency check. It must be robust enough that **even a corrupted heap never makes it crash or loop forever**; it just returns `false`.
- `heap_bytes()` is the number of arena bytes used so far; `arena()` returns `const Arena&` (the validator uses it to confirm that pointers are inside the heap).

All metadata must live in the arena: the validator fails you if it sees you call `operator new` inside `malloc`/`free`/`realloc`, and the allocator object itself must be no larger than 8 KiB.

## Stage 1 — The arena and a bump allocator

```cpp
// arena.hpp
class Arena {
public:
    explicit Arena(std::size_t capacity);
    ~Arena();

    Arena(const Arena&) = delete;
    Arena& operator=(const Arena&) = delete;

    Arena(Arena&& other);
    Arena& operator=(Arena&& other);

    std::byte* grow(std::size_t bytes);

    std::byte* base() const;
    std::size_t used() const;
    std::size_t capacity() const;
};

// bump.hpp
class BumpAllocator {
public:
    explicit BumpAllocator(std::size_t capacity);  // provided

    void* malloc(std::size_t size);
    void free(void* ptr);
    void* realloc(void* ptr, std::size_t size);
    bool check() const;

    std::size_t heap_bytes() const;  // provided
    const Arena& arena() const;      // provided
};
```

`Arena`: the constructor `mmap`s an anonymous region (capacity rounded up to whole pages, and at least one page even for 0), the destructor `munmap`s it, and it is move-only. A moved-from arena has `base() == nullptr`, and its `used()` and `capacity()` are 0. `grow(n)` is sbrk: it returns the old end, or `nullptr` with `used()` unchanged if the request doesn't fit (including absurd sizes such as `SIZE_MAX`).

`BumpAllocator`: it only ever moves forward, and `free` does nothing. `realloc` of the most recent block must grow it in place (so as not to drag down utilization); in every other case, allocate a new block and copy, so each block must remember its own size.

## Stage 2 — Implicit free list

```cpp
// implicit.hpp
class ImplicitAllocator {
public:
    explicit ImplicitAllocator(std::size_t capacity);  // provided

    void* malloc(std::size_t size);
    void free(void* ptr);
    void* realloc(void* ptr, std::size_t size);
    bool check() const;

    std::size_t heap_bytes() const;  // provided
    const Arena& arena() const;      // provided
};
```

**The block format is a public contract; the tests read and write it directly:**

- Every block (allocated or free) starts with an 8-byte header, located right at `payload − 8`. Bit 0 = allocated; bits 4 and up hold the size of the **whole block** (a multiple of 16, at least 32); bits 1–3 are yours to use.
- Blocks are contiguous in memory: the next block's payload is at `payload + size`. The end of the heap may hold a sentinel of size 0.
- At no time are there two adjacent free blocks.

`ImplicitAllocator`: walk all the blocks as an implicit list, find a fit there, and split it (when the remainder is big enough); `free` immediately coalesces with the neighbors on both sides (boundary tags). `realloc` stays in place whenever it can: shrinking doesn't move; a free block right after it gets merged; a block at the end of the heap simply calls `grow`. When `malloc` has to grow the heap and the last block is free, reuse that block and grow the heap only by the shortfall. A freshly constructed allocator may use at most 4096 bytes of the arena.

`check()` walks the whole heap and must catch: a size cleared to 0, a size that points outside the heap, a size altered so that the walk lands in the middle of a payload, and adjacent free blocks.

The implicit list already has to clear a utilization floor on every workload shape (the implicit column of the table in Stage 4).

## Stage 3 — Binned free lists

```cpp
// binned.hpp
class BinnedAllocator {
public:
    explicit BinnedAllocator(std::size_t capacity);  // provided

    void* malloc(std::size_t size);
    void free(void* ptr);
    void* realloc(void* ptr, std::size_t size);
    bool check() const;

    std::size_t heap_bytes() const;  // provided
    const Arena& arena() const;      // provided
};
```

`BinnedAllocator`: the same layout, but free blocks hang in bins by size, and `malloc` must be close to O(1): **the cost of malloc must not grow with the number of free blocks** (a `PERF` test enforces this: with 80000 free blocks too small to use, a 4096-byte malloc must take less than 3 times as long as with 10000). A free block's `next` / `prev` links must sit in the first 16 bytes of its payload, so a use-after-free that writes to freed memory corrupts them, and `check()` must catch that. Another `PERF` test does two million small `malloc`s and `free`s and must finish within 1.5 seconds.

`check()` additionally verifies that every free block is actually in a list, that everything in the lists is marked free, and that list pointers point inside the heap and agree with each other. A cycle in a list must not make it hang either.

## Stage 4 — Scoring

Compare the allocators by replaying traces with `replay.hpp`; you must clear the following thresholds (the first two don't depend on the clock and are deterministic):

- The **utilization** (`peak_live / heap_bytes`) of every shape must be above a fixed floor (`kFloors` in `tests/test.cpp`, listed below).
- The geometric mean of utilization over the seven shapes must not fall far behind the implicit list (≥ 0.92 times as high), and must far beat bump (more than twice as high).
- **Throughput** against the baselines in the same executable: at least 5 times faster than the implicit list (`long-lived`), and at least 1/3 of libc `malloc`'s on every shape.
- The average combined index `P = √(U × min(1, T/T_libc))` must be higher than the implicit list's, and ≥ 0.6.

| Shape | Implicit floor (Stage 2) | Binned floor |
| --- | --- | --- |
| `small-churn` | 0.55 | 0.65 |
| `mixed-sizes` | 0.60 | 0.65 |
| `sawtooth` | 0.75 | 0.80 |
| `checkerboard` | 0.65 | 0.70 |
| `realloc-growth` | 0.40 | 0.40 |
| `stack-lifo` | 0.80 | 0.85 |
| `long-lived` | 0.80 | 0.80 |

To see the report card for every shape (the `scorecard` `PERF` test), build with `./practice test 1011` and then run `build/release/bin/6.172/1011-arena-allocator/run --perf-only --stage 4 --verbose`.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Allocated blocks have no footer, so how does `free` know whether the previous block is free? Roughly how much utilization are those saved 8 bytes worth on a small-object workload?
2. When `realloc` can't grow in place, the heap holds both the old and the new copy until the old block is freed. Why does this push `heap_bytes` up? How can you make it happen less often?
3. Put this allocator under multithreaded use: where is the bottleneck with one global lock? Why can per-thread caches cause problems with false sharing and with blocks freed by other threads? (lec12)
