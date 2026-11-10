---
title: Zero-Overhead Two-Size Allocator (pointer tagging)
unit: 08 · Storage Allocation (HW6)
source: 6.172 HW6 §6 Write-up 4–6 (fixed aligned and smart allocator)
pdf: materials/6.172/homework/hw06/handout.pdf
kind: code
minutes: 70
---

# 0813 · Zero-Overhead Two-Size Allocator

**Original problem:** [HW6: Simple Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/e8e29570ad0673ec3460af46536a1e04_MIT6_172F18hw6.pdf)

> **Why this comes up in practice:** `free(p)` needs to know the size, which usually means a header. But if blocks are aligned enough, the lowest few bits of a pointer are always 0 and can hold information, so the metadata takes up no space at all. V8's Smi, CPython's tagged pointers, and the tags in all kinds of lock-free structures use this trick.

## Original problem

6.172 HW6 §6 (allocator overheads) continues with allocators that avoid per-object headers:

- Write-up 4: suppose there is only one object size, 64 bytes. Is a size header needed at all? Implement a cache-aligned fixed-size allocator (`fixed_aligned_init()`, `fixed_aligned_malloc()`, `fixed_aligned_free()`), this time with a real free list, and report its utilization and performance.

The handout then adds a second size: large objects of 64 bytes and small ones of 32 bytes, both cache-aligned, meaning that at least one endpoint of every object lies on a cache-line boundary. Serving every request with a large block is internal fragmentation that can double the memory needed, so the question is whether two sizes can be supported with zero space overhead. The smart allocator may change its interface so that every pointer it returns is accessed through a `SMART_PTR()` macro.

- Write-up 5: where and how can you store each allocation's size? Implement the `SMART_PTR()` and `IS_SMALL()` macros that `smart_free()` relies on.
- Write-up 6: placing a 64-byte object on a cache-line boundary may waste space, a form of external fragmentation; what would you do with that space? Implement `alloc_aligned()` and `smart_malloc()`, and report the utilization.

## Your task

The heap is a 64-aligned `std::span<std::byte>`, and on it you do bump allocation plus free lists, without storing a header anywhere. This exercise handles only two kinds of objects: 64 bytes (large) and 32 bytes (small).

Rather than filling in C functions and macros, you build each allocator as a C++ class, and a `SmartPtr` class takes the place of the `SMART_PTR()` macro.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

## Stage 1 — A fixed-size cache-aligned allocator (Write-up 4)

```cpp
class FixedAligned64 {
public:
    explicit FixedAligned64(std::span<std::byte> heap);

    void* allocate();
    void deallocate(void* p);
    std::size_t used_bytes() const;
};
```

`FixedAligned64(heap)`: only 64-byte objects, all aligned to 64.

- `allocate()`: take from the free list first (LIFO); otherwise carve from the heap; return `nullptr` once the heap is used up.
- `deallocate(p)`: put the block back on the free list; `nullptr` does nothing.
- `used_bytes()`: the position of the bump pointer. No header is needed anywhere, so n allocations (with no frees) use exactly `64 * n` bytes.

## Stage 2 — Tagged pointer (Write-up 5)

```cpp
class SmartPtr {
public:
    SmartPtr() = default;

    static SmartPtr make(void* address, bool is_small);

    void* get() const;
    bool is_small() const;
    explicit operator bool() const;

    friend bool operator==(const SmartPtr& a, const SmartPtr& b);
};
```

`SmartPtr`: the same size as `void*` (identical `sizeof`), storing "is this a small object" in the lowest bit of the address.

- `SmartPtr::make(address, is_small)`, `get()` (recovers the real address), `is_small()`, `explicit operator bool` (whether it is non-empty; `SmartPtr{}` is empty), and `operator==`.
- `address` is aligned to at least 32, so the lowest 5 bits are always free.

## Stage 3 — Two sizes without one extra byte (Write-up 5–6)

```cpp
class SmartAllocator {
public:
    explicit SmartAllocator(std::span<std::byte> heap);

    SmartPtr allocate(std::size_t bytes);
    void deallocate(SmartPtr p);
    std::size_t used_bytes() const;
};
```

`SmartAllocator(heap)`:

- `allocate(bytes)`: `1..32` gets a small object and `33..64` a large one; any other size, or running out of space, returns an empty `SmartPtr`.
- `deallocate(SmartPtr)`: knows the size from the tag on the pointer alone, and puts the block back on the matching free list.
- Small objects must satisfy "one endpoint lies on a cache-line boundary": they start at ≡ 0 or 32 (mod 64). Large objects start aligned to 64.
- Both free lists take priority over the bump pointer, and both are LIFO.

**Don't waste space:** when the bump pointer is in the middle of a line (≡ 32 mod 64) and a large object must be allocated, first push the pointer to the next line, and **put that half line's 32 bytes on the small free list**; later small allocations use it first. If the large object doesn't fit even then, return an empty `SmartPtr` and change nothing: the bump pointer stays where it was, and the half line stays available.

`used_bytes()`: the position of the bump pointer.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Write-up 4: why does an allocator for a fixed 64 bytes not even need a size header? Where is the free list's next pointer stored? (Compare 0812's packed allocator.)
2. Tagged pointers depend on alignment. If you wanted to squeeze a 16-bit generation into a pointer, what options do you have? (Hint: arm64's top-byte ignore, x86-64's canonical addresses)
3. Why can Stage 3's small free list also reuse the half line skipped for a large object? Can small and large blocks be merged afterwards? (The next exercise, 0814)
