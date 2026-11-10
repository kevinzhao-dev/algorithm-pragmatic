#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>

#include "practice.hpp"

struct Sample {
    std::int64_t value;
    std::int64_t weight;
};

// Stage 1
template <typename T>
std::pair<std::size_t, std::size_t> partition3(std::span<T> values, T pivot) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T>
T select_kth(std::span<T> values, std::size_t k) {
    PRACTICE_TODO();
}

// Stage 3
inline std::int64_t weighted_median(std::span<const Sample> samples) { PRACTICE_TODO(); }
