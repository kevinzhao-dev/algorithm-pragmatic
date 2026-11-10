#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include "08_cache_sim.hpp"
#include "practice.hpp"

struct LevelConfig {
    std::size_t capacity_bytes;
    std::size_t line_bytes;
    std::size_t ways;
};

// Stage 1
class CacheHierarchy {
public:
    explicit CacheHierarchy(std::vector<LevelConfig> levels) { PRACTICE_TODO(); }

    std::size_t access(std::uint64_t address) { PRACTICE_TODO(); }
    std::size_t access_range(std::uint64_t address, std::size_t bytes) { PRACTICE_TODO(); }
    std::uint64_t accesses(std::size_t level) const { PRACTICE_TODO(); }
    std::uint64_t misses(std::size_t level) const { PRACTICE_TODO(); }
};

// Stage 2
inline std::vector<std::uint64_t> miss_counts(std::span<const std::uint64_t> addresses,
                                              std::size_t line_bytes,
                                              std::span<const std::size_t> capacities) {
    PRACTICE_TODO();
}
