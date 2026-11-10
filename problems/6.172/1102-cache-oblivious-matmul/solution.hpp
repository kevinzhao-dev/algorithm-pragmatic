#pragma once

#include <cstddef>

#include "practice.hpp"

// A, B, C expose rows(), cols(), get(i, j), set(i, j, v); results accumulate into C.

// Stage 1
template <typename A, typename B, typename C>
void multiply_ijk(const A& a, const B& b, C& c) {
    PRACTICE_TODO();
}

template <typename A, typename B, typename C>
void multiply_ikj(const A& a, const B& b, C& c) {
    PRACTICE_TODO();
}

// Stage 2
template <typename A, typename B, typename C>
void multiply_tiled(const A& a, const B& b, C& c, std::size_t s) {
    PRACTICE_TODO();
}

// Stage 3: n must be a power of two.
template <typename A, typename B, typename C>
void multiply_recursive(const A& a, const B& b, C& c) {
    PRACTICE_TODO();
}
