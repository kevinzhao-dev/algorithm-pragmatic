#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

#include "practice.hpp"

// Stage 1, generalized in Stage 2
inline std::int64_t max_value(std::span<const std::int64_t> operands, std::string_view ops) {
    PRACTICE_TODO();
}

// Stage 3
inline std::string best_parenthesization(std::span<const std::int64_t> operands,
                                         std::string_view ops) {
    PRACTICE_TODO();
}

// Stage 4
inline std::int64_t min_multiplications(std::span<const std::int64_t> dims) { PRACTICE_TODO(); }

inline std::string best_multiplication_order(std::span<const std::int64_t> dims) {
    PRACTICE_TODO();
}
