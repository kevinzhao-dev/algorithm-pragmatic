#pragma once

#include <cstddef>
#include <optional>

#include "practice.hpp"

class BuddyAllocator {
public:
    explicit BuddyAllocator(int max_order) { PRACTICE_TODO(); }

    // Stage 1
    std::optional<std::size_t> allocate(std::size_t units) { PRACTICE_TODO(); }
    std::size_t free_blocks(int order) const { PRACTICE_TODO(); }
    std::size_t free_units() const { PRACTICE_TODO(); }

    // Stage 2
    void free(std::size_t offset) { PRACTICE_TODO(); }

    // Stage 3
    std::size_t largest_free_block() const { PRACTICE_TODO(); }
    double external_fragmentation() const { PRACTICE_TODO(); }
};
