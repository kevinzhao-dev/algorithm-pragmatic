#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

#include "practice.hpp"

// Stage 1
template <typename T, typename Compare = std::less<>>
void sort_a(std::span<T> data, Compare cmp = {}) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, typename Compare = std::less<>>
void sort_i(std::span<T> data, Compare cmp = {}) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T, typename Compare = std::less<>>
void sort_p(std::span<T> data, Compare cmp = {}) {
    PRACTICE_TODO();
}

struct ByteView {
    std::uint8_t* data;
    std::size_t size;
};

[[gnu::noinline]] inline void add_to_all(ByteView& view, std::uint8_t delta) { PRACTICE_TODO(); }

// Stage 4
template <typename T, typename Compare = std::less<>>
void sort_c(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16) {
    PRACTICE_TODO();
}

// Stage 5
template <typename T, typename Compare = std::less<>>
void sort_m(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16) {
    PRACTICE_TODO();
}

// Stage 6
template <typename T, typename Compare = std::less<>>
void sort_f(std::span<T> data, Compare cmp = {}, std::size_t cutoff = 16) {
    PRACTICE_TODO();
}
