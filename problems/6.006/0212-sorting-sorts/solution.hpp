#pragma once

#include <cstddef>
#include <functional>
#include <span>

#include "practice.hpp"

// Stage 1
template <typename Seq>
void sort_expensive_writes(Seq& seq) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, typename Less>
void sort_pointers(std::span<const T*> pointers, Less less) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T, typename Less = std::less<T>>
void repair_nearly_sorted(std::span<T> items, Less less = {}) {
    PRACTICE_TODO();
}
