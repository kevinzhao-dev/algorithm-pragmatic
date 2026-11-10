#pragma once

#include <cstddef>
#include <cstdint>

#include "practice.hpp"

class CacheSimulator {
public:
    CacheSimulator(std::size_t capacity_bytes, std::size_t line_bytes, std::size_t ways) {
        PRACTICE_TODO();
    }

    // Stage 1
    bool access(std::uint64_t address) { PRACTICE_TODO(); }
    void access_range(std::uint64_t address, std::size_t bytes) { PRACTICE_TODO(); }
    std::uint64_t accesses() const { PRACTICE_TODO(); }
    std::uint64_t misses() const { PRACTICE_TODO(); }
    std::uint64_t hits() const { PRACTICE_TODO(); }

    // Stage 3
    std::uint64_t cold_misses() const { PRACTICE_TODO(); }
    std::uint64_t conflict_misses() const { PRACTICE_TODO(); }
    std::uint64_t capacity_misses() const { PRACTICE_TODO(); }
};

// Stage 4
class CoherentCaches {
public:
    CoherentCaches(std::size_t cores, std::size_t capacity_bytes, std::size_t line_bytes,
                   std::size_t ways) {
        PRACTICE_TODO();
    }

    bool read(std::size_t core, std::uint64_t address) { PRACTICE_TODO(); }
    bool write(std::size_t core, std::uint64_t address) { PRACTICE_TODO(); }
    std::uint64_t coherence_misses() const { PRACTICE_TODO(); }
};
