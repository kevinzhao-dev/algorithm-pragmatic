#pragma once

#include <cstddef>
#include <random>
#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename T, std::uniform_random_bit_generator Rng>
void randomized_quicksort(std::span<T> values, Rng& rng) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, std::uniform_random_bit_generator Rng>
int paranoid_quicksort(std::span<T> values, Rng& rng) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T, std::uniform_random_bit_generator Rng>
T randomized_select(std::span<T> values, std::size_t k, Rng& rng) {
    PRACTICE_TODO();
}

// Stage 4
template <typename T, std::uniform_random_bit_generator Rng>
std::vector<T> top_k(std::span<T> values, std::size_t k, Rng& rng) {
    PRACTICE_TODO();
}
