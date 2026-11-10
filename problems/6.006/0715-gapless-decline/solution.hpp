#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

// Stage 1, tightened in Stage 4
inline std::size_t gapless_decline_length(std::span<const int> samples, std::size_t per_bucket) {
    PRACTICE_TODO();
}

// Stage 2, tightened in Stage 4
inline std::vector<std::size_t> gapless_decline(std::span<const int> samples,
                                                std::size_t per_bucket) {
    PRACTICE_TODO();
}

// Stage 3, tightened in Stage 4
struct WorstSeries {
    std::size_t series;
    std::vector<std::size_t> indices;
};

inline WorstSeries worst_series(std::span<const std::vector<int>> series, std::size_t per_bucket) {
    PRACTICE_TODO();
}
