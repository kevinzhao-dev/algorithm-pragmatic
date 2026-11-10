#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
[[gnu::noinline]] inline void add_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
    PRACTICE_TODO();
}

// Stage 2
[[gnu::noinline]] inline void max_bytes(std::uint8_t* a, const std::uint8_t* b, std::size_t n) {
    PRACTICE_TODO();
}

// Stage 3
[[gnu::noinline]] inline double sum_doubles(const double* x, std::size_t n) { PRACTICE_TODO(); }
[[gnu::noinline]] inline float dot_floats(const float* a, const float* b, std::size_t n) {
    PRACTICE_TODO();
}

// Stage 4
template <typename T>
[[gnu::noinline]] void add_lanes(T* out, const T* a, const T* b, std::size_t n) {
    PRACTICE_TODO();
}

// Stage 5
class ParticleStore {
public:
    explicit ParticleStore(std::size_t count) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    std::span<float> x() { PRACTICE_TODO(); }
    std::span<float> y() { PRACTICE_TODO(); }
    std::span<float> z() { PRACTICE_TODO(); }
    std::span<float> vx() { PRACTICE_TODO(); }
    std::span<float> vy() { PRACTICE_TODO(); }
    std::span<float> vz() { PRACTICE_TODO(); }

    [[gnu::noinline]] void integrate(float dt) { PRACTICE_TODO(); }
};

// Stage 6
[[gnu::noinline]] inline void row_sums(std::span<const int> matrix, std::size_t rows,
                                       std::size_t cols, std::span<int> out) {
    PRACTICE_TODO();
}

// Stage 7
[[gnu::noinline]] inline std::optional<std::size_t> best_match(std::span<const float> query,
                                                               std::span<const float> matrix,
                                                               std::size_t dim) {
    PRACTICE_TODO();
}
