#pragma once

#include <cstddef>

inline std::size_t diagonal_index(std::size_t i, std::size_t j, std::size_t n) {
    return n + i - j - 1;
}

namespace detail {

template <typename Arr, typename F>
void compute_cell(Arr& a, std::size_t n, F& f, std::size_t i, std::size_t j) {
    a.set(diagonal_index(i, j, n),
          f(a.get(diagonal_index(i - 1, j - 1, n)), a.get(diagonal_index(i, j - 1, n)),
            a.get(diagonal_index(i - 1, j, n))));
}

template <typename Arr, typename F>
void fill_piece(Arr& a, std::size_t n, F& f, std::size_t r0, std::size_t r1, std::size_t c0,
                std::size_t c1) {
    if (r1 - r0 <= 1 && c1 - c0 <= 1) {
        if (r1 > r0 && c1 > c0) {
            compute_cell(a, n, f, r0, c0);
        }
        return;
    }
    const std::size_t rm = r1 - r0 > 1 ? r0 + (r1 - r0) / 2 : r1;
    const std::size_t cm = c1 - c0 > 1 ? c0 + (c1 - c0) / 2 : c1;
    fill_piece(a, n, f, r0, rm, c0, cm);
    if (cm < c1) {
        fill_piece(a, n, f, r0, rm, cm, c1);
    }
    if (rm < r1) {
        fill_piece(a, n, f, rm, r1, c0, cm);
    }
    if (rm < r1 && cm < c1) {
        fill_piece(a, n, f, rm, r1, cm, c1);
    }
}

template <typename Arr, typename F>
void fill_piece_k(Arr& a, std::size_t n, std::size_t k, F& f, std::size_t r0, std::size_t r1,
                  std::size_t c0, std::size_t c1) {
    const std::size_t rows = r1 - r0, cols = c1 - c0;
    if (rows == 0 || cols == 0) {
        return;
    }
    if (rows == 1 && cols == 1) {
        compute_cell(a, n, f, r0, c0);
        return;
    }
    // Split each dimension into k nearly equal pieces.
    const auto row_cut = [&](std::size_t p) { return r0 + rows * p / k; };
    const auto col_cut = [&](std::size_t q) { return c0 + cols * q / k; };
    for (std::size_t p = 0; p < k; ++p) {
        for (std::size_t q = 0; q < k; ++q) {
            fill_piece_k(a, n, k, f, row_cut(p), row_cut(p + 1), col_cut(q), col_cut(q + 1));
        }
    }
}

}  // namespace detail

// Arr has exactly 2n - 1 cells; the array is initialized along the first row and column.
template <typename Arr, typename F>
double recursive_tableau(Arr& a, std::size_t n, double init, F f) {
    for (std::size_t i = 0; i < n; ++i) {
        a.set(diagonal_index(i, 0, n), init);
        a.set(diagonal_index(0, i, n), init);
    }
    detail::fill_piece(a, n, f, 1, n, 1, n);
    return a.get(diagonal_index(n - 1, n - 1, n));
}

template <typename Arr, typename F>
double recursive_tableau_k(Arr& a, std::size_t n, std::size_t k, double init, F f) {
    for (std::size_t i = 0; i < n; ++i) {
        a.set(diagonal_index(i, 0, n), init);
        a.set(diagonal_index(0, i, n), init);
    }
    detail::fill_piece_k(a, n, k, f, 1, n, 1, n);
    return a.get(diagonal_index(n - 1, n - 1, n));
}

struct CacheGeometry {
    double capacity_elements;  // M
    double line_elements;      // B
};

inline double predict_recursive_misses(double n, const CacheGeometry& cache) {
    return n * n / (cache.capacity_elements * cache.line_elements);
}
