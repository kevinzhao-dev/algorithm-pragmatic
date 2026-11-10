#pragma once

#include <cstddef>
#include <cstdint>

#include "forkjoin.hpp"

inline std::size_t diagonal_index(std::size_t i, std::size_t j, std::size_t n) {
    return n + i - j - 1;
}

namespace detail {

constexpr std::size_t kParallelCutoff = 32;

template <typename Arr, typename F>
void compute_cell(Arr& a, std::size_t n, F& f, std::size_t i, std::size_t j) {
    a.set(diagonal_index(i, j, n),
          f(a.get(diagonal_index(i - 1, j - 1, n)), a.get(diagonal_index(i, j - 1, n)),
            a.get(diagonal_index(i - 1, j, n))));
}

template <typename Arr, typename F>
void fill_serial(Arr& a, std::size_t n, F& f, std::size_t r0, std::size_t r1, std::size_t c0,
                 std::size_t c1) {
    const std::size_t rows = r1 - r0, cols = c1 - c0;
    if (rows == 1 && cols == 1) {
        compute_cell(a, n, f, r0, c0);
        return;
    }
    const std::size_t rm = rows > 1 ? r0 + rows / 2 : r1;
    const std::size_t cm = cols > 1 ? c0 + cols / 2 : c1;
    fill_serial(a, n, f, r0, rm, c0, cm);
    if (cm < c1) {
        fill_serial(a, n, f, r0, rm, cm, c1);
    }
    if (rm < r1) {
        fill_serial(a, n, f, rm, r1, c0, cm);
    }
    if (rm < r1 && cm < c1) {
        fill_serial(a, n, f, rm, r1, cm, c1);
    }
}

template <typename Arr, typename F>
void fill_parallel(Arr& a, std::size_t n, F& f, std::size_t r0, std::size_t r1, std::size_t c0,
                   std::size_t c1) {
    const std::size_t rows = r1 - r0, cols = c1 - c0;
    if (rows <= kParallelCutoff && cols <= kParallelCutoff) {
        fill_serial(a, n, f, r0, r1, c0, c1);
        return;
    }
    const std::size_t rm = rows > 1 ? r0 + rows / 2 : r1;
    const std::size_t cm = cols > 1 ? c0 + cols / 2 : c1;
    fill_parallel(a, n, f, r0, rm, c0, cm);
    if (cm < c1 && rm < r1) {
        practice::fork2([&] { fill_parallel(a, n, f, r0, rm, cm, c1); },
                        [&] { fill_parallel(a, n, f, rm, r1, c0, cm); });
    } else if (cm < c1) {
        fill_parallel(a, n, f, r0, rm, cm, c1);
    } else if (rm < r1) {
        fill_parallel(a, n, f, rm, r1, c0, cm);
    }
    if (rm < r1 && cm < c1) {
        fill_parallel(a, n, f, rm, r1, cm, c1);
    }
}

template <typename Arr>
void initialize(Arr& a, std::size_t n, double init) {
    for (std::size_t i = 0; i < n; ++i) {
        a.set(diagonal_index(i, 0, n), init);
        a.set(diagonal_index(0, i, n), init);
    }
}

}  // namespace detail

// Arr has exactly 2n - 1 cells, exposes size(), get(k), set(k, v).
template <typename Arr, typename F>
double recursive_tableau(Arr& a, std::size_t n, double init, F f) {
    detail::initialize(a, n, init);
    if (n > 1) {
        detail::fill_serial(a, n, f, 1, n, 1, n);
    }
    return a.get(diagonal_index(n - 1, n - 1, n));
}

template <typename Arr, typename F>
double parallel_tableau(Arr& a, std::size_t n, double init, F f) {
    detail::initialize(a, n, init);
    if (n > 1) {
        detail::fill_parallel(a, n, f, 1, n, 1, n);
    }
    return a.get(diagonal_index(n - 1, n - 1, n));
}

struct WorkSpan {
    std::uint64_t work = 0;
    std::uint64_t span = 0;
};

// k²-way construction on an n × n tableau with n = k^h and unit-cost cells.
inline WorkSpan tableau_work_span(std::size_t n, std::size_t k) {
    WorkSpan result;
    result.work = static_cast<std::uint64_t>(n) * n;
    result.span = 1;
    for (std::size_t side = n; side > 1; side /= k) {
        result.span *= 2 * k - 1;
    }
    return result;
}
