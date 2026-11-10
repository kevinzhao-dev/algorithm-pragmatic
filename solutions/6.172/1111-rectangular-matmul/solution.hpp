#pragma once

#include <cmath>
#include <cstddef>

// A is m×n, B is n×r, C is m×r (row-major); the result accumulates into C.

template <typename A, typename B, typename C>
void multiply_rect_naive(const A& a, const B& b, C& c) {
    const std::size_t m = a.rows(), n = a.cols(), r = b.cols();
    for (std::size_t i = 0; i < m; ++i) {
        for (std::size_t j = 0; j < r; ++j) {
            for (std::size_t k = 0; k < n; ++k) {
                c.set(i, j, c.get(i, j) + a.get(i, k) * b.get(k, j));
            }
        }
    }
}

namespace detail {

template <typename A, typename B, typename C>
void multiply_rect(const A& a, std::size_t ar, std::size_t ac, const B& b, std::size_t br,
                   std::size_t bc, C& c, std::size_t cr, std::size_t cc, std::size_t m,
                   std::size_t n, std::size_t r) {
    if (m == 1 && n == 1 && r == 1) {
        c.set(cr, cc, c.get(cr, cc) + a.get(ar, ac) * b.get(br, bc));
        return;
    }
    if (m >= n && m >= r) {
        const std::size_t h = m / 2;
        multiply_rect(a, ar, ac, b, br, bc, c, cr, cc, h, n, r);
        multiply_rect(a, ar + h, ac, b, br, bc, c, cr + h, cc, m - h, n, r);
    } else if (n >= r) {
        const std::size_t h = n / 2;
        multiply_rect(a, ar, ac, b, br, bc, c, cr, cc, m, h, r);
        multiply_rect(a, ar, ac + h, b, br + h, bc, c, cr, cc, m, n - h, r);
    } else {
        const std::size_t h = r / 2;
        multiply_rect(a, ar, ac, b, br, bc, c, cr, cc, m, n, h);
        multiply_rect(a, ar, ac, b, br, bc + h, c, cr, cc + h, m, n, r - h);
    }
}

}  // namespace detail

template <typename A, typename B, typename C>
void multiply_rect_recursive(const A& a, const B& b, C& c) {
    detail::multiply_rect(a, 0, 0, b, 0, 0, c, 0, 0, a.rows(), a.cols(), b.cols());
}

struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
    double lines() const { return capacity_elements / line_elements; }
};

// Θ-estimates, without the hidden constants, of the two naive regimes from the lecture.
inline double predict_naive_misses(double m, double n, double r, const CacheGeometry& cache) {
    if (n > cache.lines()) {
        return m * n * r;  // every element of B's column is a miss
    }
    return m * n * r / cache.line_elements;
}

inline double predict_recursive_misses(double m, double n, double r, const CacheGeometry& cache) {
    return m * n * r / (cache.line_elements * std::sqrt(cache.capacity_elements)) +
           (m * n + n * r + m * r) / cache.line_elements + m + n + r;
}
