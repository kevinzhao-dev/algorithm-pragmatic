#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <span>

#include "08_cache_sim.hpp"
#include "practice.hpp"

inline constexpr std::size_t kCacheLine = 64;

// Upstream provides void* allocate(std::size_t) (8-byte aligned) and void deallocate(void*).
template <typename Upstream>
class CacheAlignedWrapper {
public:
    explicit CacheAlignedWrapper(Upstream& upstream) { PRACTICE_TODO(); }

    // Stage 1
    void* allocate(std::size_t bytes) { PRACTICE_TODO(); }
    std::size_t overhead_bytes() const { PRACTICE_TODO(); }

    // Stage 2
    void deallocate(void* p) { PRACTICE_TODO(); }
};

// Stage 3
class PackedAllocator {
public:
    explicit PackedAllocator(std::span<std::byte> heap) { PRACTICE_TODO(); }

    void* allocate(std::size_t bytes) { PRACTICE_TODO(); }
    std::size_t size_of(const void* p) const { PRACTICE_TODO(); }
    std::size_t used_bytes() const { PRACTICE_TODO(); }
};

// Stage 4
struct PlacementStats {
    double lines_per_access = 0.0;
    std::uint64_t misses = 0;
};

inline PlacementStats measure_placement(std::span<const std::uint64_t> addresses,
                                        std::span<const std::size_t> sizes,
                                        std::span<const std::size_t> order,
                                        practice::cachesim::Cache& cache) {
    PRACTICE_TODO();
}
