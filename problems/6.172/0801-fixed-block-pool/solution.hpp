#pragma once

#include <cstddef>
#include <memory>
#include <new>
#include <utility>

#include "practice.hpp"

// Where raw chunks come from. The default forwards to operator new.
struct NewDeleteUpstream {
    void* allocate(std::size_t bytes) { return ::operator new(bytes); }
    void deallocate(void* p, std::size_t /*bytes*/) noexcept { ::operator delete(p); }
};

template <typename Upstream = NewDeleteUpstream>
class FixedBlockPool {
public:
    FixedBlockPool(std::size_t block_size, std::size_t blocks_per_chunk,
                   Upstream upstream = Upstream{}) {
        PRACTICE_TODO();
    }

    // Stage 2: return every chunk to the upstream.
    ~FixedBlockPool() {}

    FixedBlockPool(const FixedBlockPool&) = delete;
    FixedBlockPool& operator=(const FixedBlockPool&) = delete;

    // Stage 3. Once implemented, these should be noexcept.
    FixedBlockPool(FixedBlockPool&& other) { PRACTICE_TODO(); }
    FixedBlockPool& operator=(FixedBlockPool&& other) { PRACTICE_TODO(); }

    std::size_t block_size() const { PRACTICE_TODO(); }
    std::size_t allocated_blocks() const { PRACTICE_TODO(); }
    std::size_t chunk_count() const { PRACTICE_TODO(); }

    void* allocate() { PRACTICE_TODO(); }
    void deallocate(void* block) { PRACTICE_TODO(); }
};

// Stage 4: a standard allocator whose single-object allocations come from fixed-size pools.
template <typename T>
class PoolAllocator {
public:
    using value_type = T;

    PoolAllocator() { PRACTICE_TODO(); }
    PoolAllocator(const PoolAllocator& other) {}
    template <typename U>
    PoolAllocator(const PoolAllocator<U>& other) {}

    T* allocate(std::size_t n) { PRACTICE_TODO(); }
    void deallocate(T* p, std::size_t n) { PRACTICE_TODO(); }

    friend bool operator==(const PoolAllocator& a, const PoolAllocator& b) { PRACTICE_TODO(); }

    template <typename U>
    friend class PoolAllocator;
};
