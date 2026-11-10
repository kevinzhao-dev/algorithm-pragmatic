#pragma once

#include <algorithm>
#include <cstddef>

// A, B, C expose rows(), cols(), get(i, j), set(i, j, v); results accumulate into C.

template <typename A, typename B, typename C>
void multiply_ijk(const A& a, const B& b, C& c) {
    const std::size_t n = a.rows();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            for (std::size_t k = 0; k < n; ++k) {
                c.set(i, j, c.get(i, j) + a.get(i, k) * b.get(k, j));
            }
        }
    }
}

template <typename A, typename B, typename C>
void multiply_ikj(const A& a, const B& b, C& c) {
    const std::size_t n = a.rows();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < n; ++k) {
            for (std::size_t j = 0; j < n; ++j) {
                c.set(i, j, c.get(i, j) + a.get(i, k) * b.get(k, j));
            }
        }
    }
}

template <typename A, typename B, typename C>
void multiply_tiled(const A& a, const B& b, C& c, std::size_t s) {
    const std::size_t n = a.rows();
    for (std::size_t i1 = 0; i1 < n; i1 += s) {
        for (std::size_t k1 = 0; k1 < n; k1 += s) {
            for (std::size_t j1 = 0; j1 < n; j1 += s) {
                for (std::size_t i = i1; i < std::min(i1 + s, n); ++i) {
                    for (std::size_t k = k1; k < std::min(k1 + s, n); ++k) {
                        for (std::size_t j = j1; j < std::min(j1 + s, n); ++j) {
                            c.set(i, j, c.get(i, j) + a.get(i, k) * b.get(k, j));
                        }
                    }
                }
            }
        }
    }
}

namespace detail {

template <typename A, typename B, typename C>
void multiply_recursive(const A& a, std::size_t ar, std::size_t ac, const B& b, std::size_t br,
                        std::size_t bc, C& c, std::size_t cr, std::size_t cc, std::size_t n) {
    if (n == 1) {
        c.set(cr, cc, c.get(cr, cc) + a.get(ar, ac) * b.get(br, bc));
        return;
    }
    const std::size_t h = n / 2;
    multiply_recursive(a, ar, ac, b, br, bc, c, cr, cc, h);
    multiply_recursive(a, ar, ac + h, b, br + h, bc, c, cr, cc, h);
    multiply_recursive(a, ar, ac, b, br, bc + h, c, cr, cc + h, h);
    multiply_recursive(a, ar, ac + h, b, br + h, bc + h, c, cr, cc + h, h);
    multiply_recursive(a, ar + h, ac, b, br, bc, c, cr + h, cc, h);
    multiply_recursive(a, ar + h, ac + h, b, br + h, bc, c, cr + h, cc, h);
    multiply_recursive(a, ar + h, ac, b, br, bc + h, c, cr + h, cc + h, h);
    multiply_recursive(a, ar + h, ac + h, b, br + h, bc + h, c, cr + h, cc + h, h);
}

}  // namespace detail

// n must be a power of two.
template <typename A, typename B, typename C>
void multiply_recursive(const A& a, const B& b, C& c) {
    detail::multiply_recursive(a, 0, 0, b, 0, 0, c, 0, 0, a.rows());
}
