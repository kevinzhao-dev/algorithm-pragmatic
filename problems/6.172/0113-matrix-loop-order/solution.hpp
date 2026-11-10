#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

enum class LoopOrder { IJK, IKJ, JIK, JKI, KIJ, KJI };

class Matrix {
public:
    // Stage 1
    Matrix(std::size_t rows, std::size_t cols) { PRACTICE_TODO(); }

    std::size_t rows() const { PRACTICE_TODO(); }
    std::size_t cols() const { PRACTICE_TODO(); }

    double& operator()(std::size_t r, std::size_t c) { PRACTICE_TODO(); }
    double operator()(std::size_t r, std::size_t c) const { PRACTICE_TODO(); }

    std::span<double> row(std::size_t r) { PRACTICE_TODO(); }
    std::span<const double> row(std::size_t r) const { PRACTICE_TODO(); }

    friend bool operator==(const Matrix& a, const Matrix& b) { PRACTICE_TODO(); }
};

// Stage 1
inline Matrix multiply(const Matrix& a, const Matrix& b) { PRACTICE_TODO(); }

// Stage 2
template <typename Body>
void for_each_index_triple(std::size_t rows, std::size_t cols, std::size_t depth, LoopOrder order,
                           Body&& body) {
    PRACTICE_TODO();
}

inline Matrix multiply_ordered(const Matrix& a, const Matrix& b, LoopOrder order) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<LoopOrder> rank_by_cache_misses(std::size_t n, std::size_t line_bytes,
                                                   std::size_t sets, std::size_t ways) {
    PRACTICE_TODO();
}
