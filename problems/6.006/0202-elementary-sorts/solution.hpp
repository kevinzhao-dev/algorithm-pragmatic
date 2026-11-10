#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <utility>

#include "practice.hpp"

// Stage 1
template <typename T, typename Less = std::less<T>>
void selection_sort(std::span<T> items, Less less = {}) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, typename Less = std::less<T>>
void insertion_sort(std::span<T> items, Less less = {}) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T, typename Less = std::less<T>>
class TopK {
public:
    explicit TopK(std::size_t k, Less less = {}) { PRACTICE_TODO(); }

    void push(T value) { PRACTICE_TODO(); }

    std::span<const T> items() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
