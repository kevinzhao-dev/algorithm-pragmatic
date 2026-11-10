#pragma once

#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

#include "practice.hpp"

struct QuicksortTrace {
    std::vector<std::uint32_t> pivot_comparisons;
    std::uint64_t total_comparisons = 0;
};

struct BoundReport {
    int trials = 0;
    int element_violations = 0;
    int total_violations = 0;
    double mean_total = 0.0;
};

// Stage 1
inline std::size_t good_pivot_count(std::size_t m, std::size_t rank) { PRACTICE_TODO(); }

// Stage 2
template <typename T, std::uniform_random_bit_generator Rng>
QuicksortTrace traced_quicksort(std::span<T> values, Rng& rng) {
    PRACTICE_TODO();
}

// Stage 3
inline double element_bound(double n, int alpha) { PRACTICE_TODO(); }
inline double total_bound(double n, int alpha) { PRACTICE_TODO(); }

template <std::uniform_random_bit_generator Rng>
BoundReport measure_bound(std::size_t n, int trials, int alpha, Rng& rng) {
    PRACTICE_TODO();
}

// Stage 4
template <typename T, std::uniform_random_bit_generator Rng>
void guarded_sort(std::span<T> values, Rng& rng) {
    PRACTICE_TODO();
}
