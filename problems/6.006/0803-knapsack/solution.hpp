#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

struct Item {
    std::int64_t size;
    std::int64_t value;
};

// Stage 1, tightened in Stage 3
inline std::int64_t knapsack_value(std::span<const Item> items, std::int64_t capacity) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<std::size_t> knapsack_items(std::span<const Item> items, std::int64_t capacity) {
    PRACTICE_TODO();
}

// Stage 4
inline std::int64_t knapsack_by_value(std::span<const Item> items, std::int64_t capacity) {
    PRACTICE_TODO();
}
