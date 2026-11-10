#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename T, typename Less = std::less<T>>
std::vector<T> merge_sorted(std::span<const T> a, std::span<const T> b, Less less = {}) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, typename Less = std::less<T>>
void merge_sort(std::span<T> items, Less less = {}) {
    PRACTICE_TODO();
}

// Stage 3
inline std::uint64_t count_inversions(std::span<const int> values) { PRACTICE_TODO(); }

inline std::uint64_t kendall_tau_distance(std::span<const int> rank_a, std::span<const int> rank_b) {
    PRACTICE_TODO();
}
