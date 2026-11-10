#pragma once

#include <cstddef>
#include <random>

#include "practice.hpp"

// Stage 1
template <typename Ring>
class Matrix {
public:
    explicit Matrix(std::size_t n) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    Ring& operator()(std::size_t i, std::size_t j) { PRACTICE_TODO(); }
    const Ring& operator()(std::size_t i, std::size_t j) const { PRACTICE_TODO(); }

    friend bool operator==(const Matrix&, const Matrix&) { PRACTICE_TODO(); }
};

template <typename Ring>
Matrix<Ring> multiply_naive(const Matrix<Ring>& a, const Matrix<Ring>& b) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Ring>
Matrix<Ring> multiply_strassen(const Matrix<Ring>& a, const Matrix<Ring>& b, std::size_t cutoff = 64) {
    PRACTICE_TODO();
}

// Stage 3
template <typename Ring, std::uniform_random_bit_generator Rng>
bool verify_product(const Matrix<Ring>& a, const Matrix<Ring>& b, const Matrix<Ring>& c, Rng& rng,
                    int rounds = 32) {
    PRACTICE_TODO();
}
