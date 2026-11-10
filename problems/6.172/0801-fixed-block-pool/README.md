---
title: Fixed-Size Block Pool (free list and object pool)
unit: 08 · Storage Allocation (HW6)
source: 6.172 Lecture 11 Fixed-Size Allocation; Recitation 6 Problem 2
pdf: materials/6.172/lectures/lec11.pdf
answer: materials/6.172/recitations/r6/solutions.pdf
read: materials/6.172/recitations/r6/problems.pdf
kind: perf
minutes: 100
---

# 0801 · Fixed-Size Block Pool

**Based on:** [Storage Allocation](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/68624442da6047731a33b3590cefd030_MIT6_172F18_lec11.pdf) · **Read first:** [R6: Simple, Straightforward Memory Allocation Problems](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/7f2c864d6f81e8f98da02ee657202518_MIT6_172F18_rec6.pdf)

> **Why this comes up in practice:** connection objects, request contexts, and tree nodes are numerous, all the same size, and short-lived. Sending them through a general-purpose allocator has costs (lookups across size classes, per-block metadata) that are clearly visible on a hot path, so almost every latency-sensitive system has an object pool.

## Original problem

6.172 Recitation 6 Problem 2 is a true-or-false question about free lists for serial allocation of fixed-size blocks. Two professors each build a linked list with two O(1) operations and use it as the free list of their allocator:

- Professor de Lancie's list adds at the tail and removes from the head, so freed blocks come back out in first-in, first-out order.
- Professor Barker's list adds at the head and removes from the head, so the most recently freed block comes out first.

Decide whether each claim is true or false:

- (a) Barker's allocator will likely run faster, thanks to better temporal locality.
- (b) de Lancie's allocator will likely use less space, thanks to less external fragmentation.

## Your task

Turn Lecture 11's fixed-size allocation scheme into a real component: the free list's next pointers are **stored directly in the idle blocks**, so there is no extra metadata.

In `solution.hpp`, `Upstream` is where chunks come from; the tests pass in a version that keeps count.

Where the recitation only compares two free-list orders, here you build the whole pool, with LIFO reuse, growth by chunks, and ownership transfer, and finally plug it into STL containers through an allocator.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — A free list over a single chunk

```cpp
// Where raw chunks come from. The default forwards to operator new.
struct NewDeleteUpstream {
    void* allocate(std::size_t bytes) { return ::operator new(bytes); }
    void deallocate(void* p, std::size_t /*bytes*/) noexcept { ::operator delete(p); }
};

template <typename Upstream = NewDeleteUpstream>
class FixedBlockPool {
public:
    FixedBlockPool(std::size_t block_size, std::size_t blocks_per_chunk,
                   Upstream upstream = Upstream{});

    FixedBlockPool(const FixedBlockPool&) = delete;
    FixedBlockPool& operator=(const FixedBlockPool&) = delete;

    std::size_t block_size() const;
    std::size_t allocated_blocks() const;

    void* allocate();
    void deallocate(void* block);
};
```

`FixedBlockPool(block_size, blocks_per_chunk, upstream)`:

- `block_size()` is the actual size of each block: `block_size` rounded up to a multiple of 16 (a block must hold the free list's pointer and be aligned to 16, so that it can safely hold any fundamental type).
- `allocate()`: take a block from the free list **first**; otherwise carve off the next unused block of the chunk. Request a chunk from the upstream the first time memory is needed. O(1) on average.
- `deallocate(block)`: put the block back on the free list in O(1). `nullptr` does nothing.
- The free list must be **LIFO**: the last block put back is the first one taken out. This is deliberate; think about why (interview follow-up 1).
- `allocated_blocks()`: the number of blocks handed out and not yet returned.

This stage never uses more than one chunk.

## Stage 2 — Growing and giving back

```cpp
// in class FixedBlockPool<Upstream>
~FixedBlockPool();  // returns every chunk to the upstream
std::size_t chunk_count() const;
```

- When the chunk runs out, request another one of the same size from the upstream, and `chunk_count()` goes up by one. Do not request a new chunk until the returned blocks have been used up.
- The destructor returns **every** chunk to the upstream (`Upstream::deallocate` gets back the original pointer and size), whether or not any blocks are still out.

Each chunk may carry only a constant amount of extra metadata: the chunk size the upstream receives must not exceed `blocks_per_chunk * block_size() + 64`.

A `PERF` test (Release builds only) checks that allocating and freeing through the pool is more than 3 times as fast as `::operator new` and `::operator delete`.

## Stage 3 — Transferring ownership

```cpp
// in class FixedBlockPool<Upstream>
FixedBlockPool(FixedBlockPool&& other);             // should be noexcept once implemented
FixedBlockPool& operator=(FixedBlockPool&& other);  // should be noexcept once implemented
```

Add a move constructor and move assignment (`noexcept`). Blocks handed out before the move stay valid afterwards and can be returned through the new owner. A moved-from pool holds no chunks (`chunk_count()` and `allocated_blocks()` both return 0) and can be destroyed. Move assignment first returns the target's own chunks to its upstream. Self-move must not break anything.

## Stage 4 — Plugging into the STL: `PoolAllocator<T>`

```cpp
// a standard allocator whose single-object allocations come from fixed-size pools
template <typename T>
class PoolAllocator {
public:
    using value_type = T;

    PoolAllocator();
    PoolAllocator(const PoolAllocator& other);
    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other);

    T* allocate(std::size_t n);
    void deallocate(T* p, std::size_t n);

    friend bool operator==(const PoolAllocator& a, const PoolAllocator& b);

    template <typename U>
    friend class PoolAllocator;
};
```

Build a minimal standard allocator so that the nodes of node-based containers (`std::list`, `std::map`) all come from a pool:

- Default construction creates one shared state (`std::shared_ptr`); copies and rebinds all point to that same state. A container internally rebinds `PoolAllocator<T>` to its node type, so the shared state must be able to keep separate pools by `sizeof`.
- `allocate(1)` goes through a pool; `allocate(n)` (n ≠ 1) falls back to `::operator new`, and the matching `deallocate` must take the same path.
- Two allocators compare equal when they point to the same state, so two default-constructed allocators compare unequal.

A `PERF` test checks that a `std::list` on `PoolAllocator` takes less than 1.3 times as long as one on the default allocator.

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why should the free list be LIFO? What would be worse about handing out the block returned longest ago first (FIFO)? Does it make a difference to space usage? (Recitation 6 Problem 2)
2. Why is this pool's external fragmentation almost zero? When is it actually more wasteful than malloc?
3. To make it thread-safe for multithreaded use, how would you split it up? How do Lecture 12's local heaps differ from local ownership?
4. For allocating objects that are always 1040 bytes, what does a fixed-size pool do better than power-of-two binned free lists? (Recitation 6 Problem 3)

---

**Official solution:** [R6 Solutions](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/3f0e281a9df611606311d6508ca76f7d_MIT6_172F18_rec6sol.pdf). Read it after you finish.
