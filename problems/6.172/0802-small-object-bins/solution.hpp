#pragma once

#include <cstddef>
#include <new>
#include <utility>

#include "practice.hpp"

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

    explicit BinnedAllocator(std::size_t chunk_bytes = 1 << 16, Upstream upstream = Upstream{}) {
        PRACTICE_TODO();
    }

    // Stage 3: return every chunk to the upstream.
    ~BinnedAllocator() {}

    BinnedAllocator(const BinnedAllocator&) = delete;
    BinnedAllocator& operator=(const BinnedAllocator&) = delete;

    // Stage 1
    static std::size_t bin_index(std::size_t size) { PRACTICE_TODO(); }
    static std::size_t bin_size(std::size_t index) { PRACTICE_TODO(); }

    // Stage 2
    void* allocate(std::size_t size) { PRACTICE_TODO(); }
    void deallocate(void* p, std::size_t size) { PRACTICE_TODO(); }

    // Stage 3
    std::size_t large_allocations() const { PRACTICE_TODO(); }
    std::size_t live_bytes() const { PRACTICE_TODO(); }
};
