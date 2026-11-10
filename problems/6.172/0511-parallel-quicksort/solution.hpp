#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <vector>

#include "01_exec.hpp"
#include "practice.hpp"

// Stage 1
template <typename T, typename Compare = std::less<>>
std::size_t partition_around(std::span<T> data, std::size_t pivot_index, Compare cmp = {}) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Exec, typename T, typename Compare = std::less<>>
void quicksort(Exec& exec, std::span<T> data, Compare cmp = {}) {
    PRACTICE_TODO();
}

// Stage 3
template <typename Exec, typename T, typename Compare>
void quicksort(Exec& exec, std::span<T> data, Compare cmp, std::size_t cutoff) {
    PRACTICE_TODO();
}
