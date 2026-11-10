#pragma once

#include <cstddef>
#include <cstdint>
#include <ranges>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1
struct Subarray {
    std::int64_t sum;
    std::size_t first;
    std::size_t last;  // inclusive
};

inline Subarray max_subarray(std::span<const std::int64_t> values) { PRACTICE_TODO(); }

// Stage 2, tightened in Stage 4
template <std::ranges::random_access_range R>
std::size_t lis_length(const R& values) {
    PRACTICE_TODO();
}

// Stage 3, tightened in Stage 4
template <std::ranges::random_access_range R>
std::vector<std::size_t> lis_indices(const R& values) {
    PRACTICE_TODO();
}

// Stage 5
template <std::ranges::random_access_range R>
std::vector<std::size_t> keep_in_place(const R& before, const R& after) {
    PRACTICE_TODO();
}
