#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

#include "practice.hpp"

// Stage 1
template <typename T, typename KeyFn>
void counting_sort(std::span<T> items, std::size_t u, KeyFn key) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, typename DigitFn>
void tuple_sort(std::span<T> items, std::size_t digits, std::size_t base, DigitFn digit) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T, typename KeyFn>
void radix_sort(std::span<T> items, KeyFn key) {
    PRACTICE_TODO();
}

// Stage 4
inline void sort_signed(std::span<std::int64_t> values) { PRACTICE_TODO(); }
inline void sort_equal_length(std::span<std::string> strings) { PRACTICE_TODO(); }
