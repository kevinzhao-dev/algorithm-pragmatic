#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

#include "practice.hpp"

enum class Direction { Less, Here, Greater };

// Stage 1
template <typename Oracle>
std::int64_t locate_in_range(std::int64_t low, std::int64_t high, Oracle oracle) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Oracle>
std::int64_t locate_from_start(std::int64_t n, Oracle oracle) {
    PRACTICE_TODO();
}

// Stage 3
template <typename Oracle>
std::int64_t locate_two_sided(std::int64_t n, Oracle oracle) {
    PRACTICE_TODO();
}

// Stage 4
template <typename T>
class FingerCursor {
public:
    explicit FingerCursor(std::span<const T> sorted) { PRACTICE_TODO(); }

    std::size_t seek(const T& key) { PRACTICE_TODO(); }
    std::size_t position() const { PRACTICE_TODO(); }
};
