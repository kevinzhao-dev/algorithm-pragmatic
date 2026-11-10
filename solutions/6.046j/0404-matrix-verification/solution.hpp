#pragma once

#include <bit>
#include <cstddef>
#include <cstdint>
#include <random>
#include <stdexcept>
#include <vector>

template <typename Ring>
class Matrix {
public:
    explicit Matrix(std::size_t n) : n_(n), cells_(n * n, Ring{}) {}

    std::size_t size() const { return n_; }
    Ring& operator()(std::size_t i, std::size_t j) { return cells_[i * n_ + j]; }
    const Ring& operator()(std::size_t i, std::size_t j) const { return cells_[i * n_ + j]; }

    friend bool operator==(const Matrix&, const Matrix&) = default;

private:
    std::size_t n_;
    std::vector<Ring> cells_;
};

namespace matrix_detail {

template <typename Ring>
void require_same_size(const Matrix<Ring>& a, const Matrix<Ring>& b) {
    if (a.size() != b.size()) {
        throw std::invalid_argument("matrix sizes differ");
    }
}

template <typename Ring>
Matrix<Ring> add(const Matrix<Ring>& a, const Matrix<Ring>& b) {
    Matrix<Ring> c(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < a.size(); ++j) {
            c(i, j) = a(i, j) + b(i, j);
        }
    }
    return c;
}

template <typename Ring>
Matrix<Ring> subtract(const Matrix<Ring>& a, const Matrix<Ring>& b) {
    Matrix<Ring> c(a.size());
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < a.size(); ++j) {
            c(i, j) = a(i, j) - b(i, j);
        }
    }
    return c;
}

template <typename Ring>
Matrix<Ring> quadrant(const Matrix<Ring>& a, std::size_t row, std::size_t col, std::size_t size) {
    Matrix<Ring> part(size);
    for (std::size_t i = 0; i < size; ++i) {
        for (std::size_t j = 0; j < size; ++j) {
            part(i, j) = a(row + i, col + j);
        }
    }
    return part;
}

template <typename Ring>
void paste(Matrix<Ring>& target, const Matrix<Ring>& part, std::size_t row, std::size_t col) {
    for (std::size_t i = 0; i < part.size(); ++i) {
        for (std::size_t j = 0; j < part.size(); ++j) {
            target(row + i, col + j) = part(i, j);
        }
    }
}

template <typename Ring>
Matrix<Ring> strassen(const Matrix<Ring>& a, const Matrix<Ring>& b, std::size_t cutoff);

}  // namespace matrix_detail

template <typename Ring>
Matrix<Ring> multiply_naive(const Matrix<Ring>& a, const Matrix<Ring>& b) {
    matrix_detail::require_same_size(a, b);
    const std::size_t n = a.size();
    Matrix<Ring> c(n);
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < n; ++k) {
            for (std::size_t j = 0; j < n; ++j) {
                c(i, j) = c(i, j) + a(i, k) * b(k, j);
            }
        }
    }
    return c;
}

template <typename Ring>
Matrix<Ring> multiply_strassen(const Matrix<Ring>& a, const Matrix<Ring>& b, std::size_t cutoff = 64) {
    matrix_detail::require_same_size(a, b);
    if (cutoff == 0) {
        throw std::invalid_argument("cutoff must be positive");
    }
    const std::size_t n = a.size();
    if (n == 0) {
        return Matrix<Ring>(0);
    }
    const std::size_t padded = std::bit_ceil(n);
    if (padded == n) {
        return matrix_detail::strassen(a, b, cutoff);
    }
    Matrix<Ring> grown_a(padded);
    Matrix<Ring> grown_b(padded);
    matrix_detail::paste(grown_a, a, 0, 0);
    matrix_detail::paste(grown_b, b, 0, 0);
    const Matrix<Ring> product = matrix_detail::strassen(grown_a, grown_b, cutoff);
    return matrix_detail::quadrant(product, 0, 0, n);
}

namespace matrix_detail {

template <typename Ring>
Matrix<Ring> strassen(const Matrix<Ring>& a, const Matrix<Ring>& b, std::size_t cutoff) {
    const std::size_t n = a.size();
    if (n <= cutoff) {
        return multiply_naive(a, b);
    }
    const std::size_t half = n / 2;
    const auto a11 = quadrant(a, 0, 0, half);
    const auto a12 = quadrant(a, 0, half, half);
    const auto a21 = quadrant(a, half, 0, half);
    const auto a22 = quadrant(a, half, half, half);
    const auto b11 = quadrant(b, 0, 0, half);
    const auto b12 = quadrant(b, 0, half, half);
    const auto b21 = quadrant(b, half, 0, half);
    const auto b22 = quadrant(b, half, half, half);

    const auto m1 = strassen(add(a11, a22), add(b11, b22), cutoff);
    const auto m2 = strassen(add(a21, a22), b11, cutoff);
    const auto m3 = strassen(a11, subtract(b12, b22), cutoff);
    const auto m4 = strassen(a22, subtract(b21, b11), cutoff);
    const auto m5 = strassen(add(a11, a12), b22, cutoff);
    const auto m6 = strassen(subtract(a21, a11), add(b11, b12), cutoff);
    const auto m7 = strassen(subtract(a12, a22), add(b21, b22), cutoff);

    Matrix<Ring> c(n);
    paste(c, add(subtract(add(m1, m4), m5), m7), 0, 0);
    paste(c, add(m3, m5), 0, half);
    paste(c, add(m2, m4), half, 0);
    paste(c, add(subtract(add(m1, m3), m2), m6), half, half);
    return c;
}

}  // namespace matrix_detail

template <typename Ring, std::uniform_random_bit_generator Rng>
bool verify_product(const Matrix<Ring>& a, const Matrix<Ring>& b, const Matrix<Ring>& c, Rng& rng,
                    int rounds = 32) {
    matrix_detail::require_same_size(a, b);
    matrix_detail::require_same_size(a, c);
    const std::size_t n = a.size();
    std::bernoulli_distribution coin(0.5);
    const auto apply = [n](const Matrix<Ring>& m, const std::vector<Ring>& v) {
        std::vector<Ring> out(n, Ring{});
        for (std::size_t i = 0; i < n; ++i) {
            for (std::size_t j = 0; j < n; ++j) {
                out[i] = out[i] + m(i, j) * v[j];
            }
        }
        return out;
    };

    for (int round = 0; round < rounds; ++round) {
        std::vector<Ring> r;
        r.reserve(n);
        for (std::size_t i = 0; i < n; ++i) {
            r.push_back(coin(rng) ? Ring(1) : Ring{});
        }
        if (apply(a, apply(b, r)) != apply(c, r)) {
            return false;
        }
    }
    return true;
}
