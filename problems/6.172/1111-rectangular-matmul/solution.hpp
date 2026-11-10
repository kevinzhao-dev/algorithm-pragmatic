#pragma once

#include <cstddef>

#include "practice.hpp"

// A is m×n, B is n×r, C is m×r (row-major); the result accumulates into C.

// Stage 1
template <typename A, typename B, typename C>
void multiply_rect_naive(const A& a, const B& b, C& c) {
    PRACTICE_TODO();
}

// Stage 2
template <typename A, typename B, typename C>
void multiply_rect_recursive(const A& a, const B& b, C& c) {
    PRACTICE_TODO();
}

// Stage 3
struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
    double lines() const { return capacity_elements / line_elements; }
};

inline double predict_naive_misses(double m, double n, double r, const CacheGeometry& cache) {
    PRACTICE_TODO();
}
inline double predict_recursive_misses(double m, double n, double r, const CacheGeometry& cache) {
    PRACTICE_TODO();
}
