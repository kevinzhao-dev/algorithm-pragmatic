#pragma once

#include <cstddef>

// The n × n tableau stores one value per diagonal: cell (i, j) lives at index n + i - j - 1,
// so filling row by row overwrites each cell's left and upper-left predecessor in place.
inline std::size_t diagonal_index(std::size_t i, std::size_t j, std::size_t n) {
    return n + i - j - 1;
}

// Arr exposes size(), get(k), set(k, v) and has exactly 2n - 1 cells.
// f(up_left, left, up) computes one cell.
template <typename Arr, typename F>
double fill_tableau(Arr& a, std::size_t n, double init, F f) {
    for (std::size_t i = 0; i < n; ++i) {
        a.set(diagonal_index(i, 0, n), init);
        a.set(diagonal_index(0, i, n), init);
    }
    for (std::size_t i = 1; i < n; ++i) {
        for (std::size_t j = 1; j < n; ++j) {
            const double value = f(a.get(diagonal_index(i - 1, j - 1, n)),
                                   a.get(diagonal_index(i, j - 1, n)),
                                   a.get(diagonal_index(i - 1, j, n)));
            a.set(diagonal_index(i, j, n), value);
        }
    }
    return a.get(diagonal_index(n - 1, n - 1, n));
}

struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
};

// Θ-estimate of the iterative filler's misses.
inline double predict_iterative_misses(double n, const CacheGeometry& cache) {
    if (n < cache.capacity_elements) {
        return 2 * n / cache.line_elements;
    }
    return n * n / cache.line_elements;
}
