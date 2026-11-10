#pragma once

#include <cstddef>

#include "practice.hpp"

// Stage 1. Arr has exactly 2n - 1 cells, exposes size(), get(k), set(k, v).
template <typename Arr, typename F>
double recursive_tableau(Arr& a, std::size_t n, double init, F f) {
    PRACTICE_TODO();
}

// Stage 2
struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
};

inline double predict_recursive_misses(double n, const CacheGeometry& cache) { PRACTICE_TODO(); }

// Stage 3
template <typename Arr, typename F>
double recursive_tableau_k(Arr& a, std::size_t n, std::size_t k, double init, F f) {
    PRACTICE_TODO();
}
