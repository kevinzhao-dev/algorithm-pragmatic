#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "mini_cache.hpp"

enum class LoopOrder { IJK, IKJ, JIK, JKI, KIJ, KJI };

class Matrix {
public:
    Matrix(std::size_t rows, std::size_t cols)
        : rows_(rows), cols_(cols), values_(rows * cols, 0.0) {}

    std::size_t rows() const { return rows_; }
    std::size_t cols() const { return cols_; }

    double& operator()(std::size_t r, std::size_t c) { return values_[r * cols_ + c]; }
    double operator()(std::size_t r, std::size_t c) const { return values_[r * cols_ + c]; }

    std::span<double> row(std::size_t r) { return {values_.data() + r * cols_, cols_}; }
    std::span<const double> row(std::size_t r) const { return {values_.data() + r * cols_, cols_}; }

    friend bool operator==(const Matrix&, const Matrix&) = default;

private:
    std::size_t rows_;
    std::size_t cols_;
    std::vector<double> values_;
};

namespace detail_0113 {
inline void check_dimensions(const Matrix& a, const Matrix& b) {
    if (a.cols() != b.rows()) {
        throw std::invalid_argument("A.cols() must equal B.rows()");
    }
}
}  // namespace detail_0113

inline Matrix multiply(const Matrix& a, const Matrix& b) {
    detail_0113::check_dimensions(a, b);
    Matrix c(a.rows(), b.cols());
    for (std::size_t i = 0; i < a.rows(); ++i) {
        for (std::size_t k = 0; k < a.cols(); ++k) {
            for (std::size_t j = 0; j < b.cols(); ++j) {
                c(i, j) += a(i, k) * b(k, j);
            }
        }
    }
    return c;
}

template <typename Body>
void for_each_index_triple(std::size_t rows, std::size_t cols, std::size_t depth, LoopOrder order,
                           Body&& body) {
    // Each nest visits (i, j, k) with i in [0, rows), j in [0, cols), k in [0, depth).
    switch (order) {
        case LoopOrder::IJK:
            for (std::size_t i = 0; i < rows; ++i) {
                for (std::size_t j = 0; j < cols; ++j) {
                    for (std::size_t k = 0; k < depth; ++k) {
                        body(i, j, k);
                    }
                }
            }
            break;
        case LoopOrder::IKJ:
            for (std::size_t i = 0; i < rows; ++i) {
                for (std::size_t k = 0; k < depth; ++k) {
                    for (std::size_t j = 0; j < cols; ++j) {
                        body(i, j, k);
                    }
                }
            }
            break;
        case LoopOrder::JIK:
            for (std::size_t j = 0; j < cols; ++j) {
                for (std::size_t i = 0; i < rows; ++i) {
                    for (std::size_t k = 0; k < depth; ++k) {
                        body(i, j, k);
                    }
                }
            }
            break;
        case LoopOrder::JKI:
            for (std::size_t j = 0; j < cols; ++j) {
                for (std::size_t k = 0; k < depth; ++k) {
                    for (std::size_t i = 0; i < rows; ++i) {
                        body(i, j, k);
                    }
                }
            }
            break;
        case LoopOrder::KIJ:
            for (std::size_t k = 0; k < depth; ++k) {
                for (std::size_t i = 0; i < rows; ++i) {
                    for (std::size_t j = 0; j < cols; ++j) {
                        body(i, j, k);
                    }
                }
            }
            break;
        case LoopOrder::KJI:
            for (std::size_t k = 0; k < depth; ++k) {
                for (std::size_t j = 0; j < cols; ++j) {
                    for (std::size_t i = 0; i < rows; ++i) {
                        body(i, j, k);
                    }
                }
            }
            break;
    }
}

inline Matrix multiply_ordered(const Matrix& a, const Matrix& b, LoopOrder order) {
    detail_0113::check_dimensions(a, b);
    Matrix c(a.rows(), b.cols());
    for_each_index_triple(a.rows(), b.cols(), a.cols(), order,
                          [&](std::size_t i, std::size_t j, std::size_t k) {
                              c(i, j) += a(i, k) * b(k, j);
                          });
    return c;
}

inline std::vector<LoopOrder> rank_by_cache_misses(std::size_t n, std::size_t line_bytes,
                                                   std::size_t sets, std::size_t ways) {
    const std::vector<LoopOrder> orders = {LoopOrder::IJK, LoopOrder::IKJ, LoopOrder::JIK,
                                           LoopOrder::JKI, LoopOrder::KIJ, LoopOrder::KJI};
    std::vector<std::pair<std::uint64_t, LoopOrder>> scored;
    for (const LoopOrder order : orders) {
        MiniCache cache(line_bytes, sets, ways);
        const std::uint64_t base_a = 0;
        const std::uint64_t base_b = std::uint64_t{1} << 30;
        const std::uint64_t base_c = std::uint64_t{2} << 30;
        for_each_index_triple(n, n, n, order, [&](std::size_t i, std::size_t j, std::size_t k) {
            cache.access(base_c + 8 * (i * n + j));
            cache.access(base_a + 8 * (i * n + k));
            cache.access(base_b + 8 * (k * n + j));
            cache.access(base_c + 8 * (i * n + j));
        });
        scored.emplace_back(cache.misses(), order);
    }
    std::stable_sort(scored.begin(), scored.end(),
                     [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<LoopOrder> ranking;
    for (const auto& [misses, order] : scored) {
        ranking.push_back(order);
    }
    return ranking;
}
