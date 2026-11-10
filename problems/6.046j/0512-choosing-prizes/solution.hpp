#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

struct Prize {
    std::int64_t value;
    char type;
};

struct TreePrize {
    std::int64_t value;
    int left;
    int right;
};

// Stage 1
inline std::vector<int> pick_top(std::span<const std::int64_t> values, std::size_t m) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<int> pick_a_then_b(std::span<const Prize> prizes, std::size_t m) {
    PRACTICE_TODO();
}

// Stages 3 and 5
inline std::vector<int> pick_non_decreasing(std::span<const std::int64_t> values, std::size_t m) {
    PRACTICE_TODO();
}

// Stages 4 and 5
inline std::vector<int> pick_rooted_subtree(std::span<const TreePrize> tree, int root, std::size_t m) {
    PRACTICE_TODO();
}
