#pragma once

#include <cstdint>
#include <optional>
#include <stdexcept>

#include "practice.hpp"

// Stage 1
inline std::int64_t floor_mod(std::int64_t a, std::int64_t m) { PRACTICE_TODO(); }
inline std::int64_t floor_div(std::int64_t a, std::int64_t m) { PRACTICE_TODO(); }
inline bool congruent(std::int64_t a, std::int64_t b, std::int64_t m) { PRACTICE_TODO(); }

// Stage 2
inline std::uint64_t mul_mod(std::uint64_t a, std::uint64_t b, std::uint64_t m) {
    PRACTICE_TODO();
}
inline std::uint64_t pow_mod(std::uint64_t base, std::uint64_t exponent, std::uint64_t m) {
    PRACTICE_TODO();
}

// Stage 3
struct PeriodicJob {
    std::int64_t phase;
    std::int64_t period;
};

inline std::optional<std::int64_t> first_coincidence(PeriodicJob a, PeriodicJob b) {
    PRACTICE_TODO();
}
