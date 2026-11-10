#pragma once

#include <cstddef>

#include "practice.hpp"

// Stage 1
inline std::size_t diagonal_index(std::size_t i, std::size_t j, std::size_t n) { PRACTICE_TODO(); }

// Arr exposes size(), get(k), set(k, v) and has exactly 2n - 1 cells.
// f(up_left, left, up) computes one cell.
template <typename Arr, typename F>
double fill_tableau(Arr& a, std::size_t n, double init, F f) {
    PRACTICE_TODO();
}

// Stage 2
struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
};

inline double predict_iterative_misses(double n, const CacheGeometry& cache) { PRACTICE_TODO(); }
