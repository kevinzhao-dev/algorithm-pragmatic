#pragma once

#include <complex>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"

using Complex = std::complex<double>;

inline constexpr std::uint32_t kNttModulus = 998'244'353;

// Stage 1
inline std::vector<std::int64_t> multiply_naive(std::span<const std::int64_t> a,
                                                std::span<const std::int64_t> b) {
    PRACTICE_TODO();
}

// Stage 2
inline void fft(std::span<Complex> a, bool inverse = false) { PRACTICE_TODO(); }

// Stage 3
inline std::vector<std::int64_t> multiply(std::span<const std::int64_t> a,
                                          std::span<const std::int64_t> b) {
    PRACTICE_TODO();
}

// Stage 4
inline std::vector<std::uint32_t> multiply_mod(std::span<const std::uint32_t> a,
                                               std::span<const std::uint32_t> b) {
    PRACTICE_TODO();
}

// Stage 5
inline std::string multiply_decimal(std::string_view a, std::string_view b) { PRACTICE_TODO(); }
