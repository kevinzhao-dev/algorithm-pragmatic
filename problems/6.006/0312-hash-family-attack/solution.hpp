#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"

struct Fraction {
    std::uint64_t num = 0;
    std::uint64_t den = 1;
    friend bool operator==(const Fraction&, const Fraction&) = default;
};

using KeyPair = std::pair<std::uint64_t, std::uint64_t>;

// Stage 1
inline std::optional<KeyPair> break_linear_family(std::uint64_t n, std::uint64_t u) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<KeyPair> break_bucket_family(std::uint64_t n, std::uint64_t u) {
    PRACTICE_TODO();
}

// Stage 3
inline Fraction universal_collision_probability(std::uint64_t p, std::uint64_t n) {
    PRACTICE_TODO();
}

// Stage 4
inline std::vector<std::uint64_t> flood_linear(std::uint64_t n, std::uint64_t u, std::size_t count) {
    PRACTICE_TODO();
}
inline std::vector<std::uint64_t> flood_bucket(std::uint64_t n, std::uint64_t u, std::size_t count) {
    PRACTICE_TODO();
}
