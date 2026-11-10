#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

template <typename T, typename Compare = std::less<T>>
class BinaryHeap {
public:
    // Stage 1
    explicit BinaryHeap(Compare compare = Compare{}) { PRACTICE_TODO(); }

    void push(T value) { PRACTICE_TODO(); }
    const T& top() const { PRACTICE_TODO(); }
    T pop() { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::span<const T> items() const { PRACTICE_TODO(); }

    // Stage 2
    explicit BinaryHeap(std::vector<T> items, Compare compare = Compare{}) { PRACTICE_TODO(); }
};

// Stage 3
template <std::random_access_iterator It, typename Compare = std::less<>>
void heapsort(It first, It last, Compare compare = Compare{}) {
    PRACTICE_TODO();
}

// Stage 4
template <typename T, typename Compare = std::less<T>>
class ReorderBuffer {
public:
    explicit ReorderBuffer(std::size_t lateness, Compare compare = Compare{}) { PRACTICE_TODO(); }

    std::optional<T> push(T item) { PRACTICE_TODO(); }
    std::vector<T> drain() { PRACTICE_TODO(); }
    std::size_t buffered() const { PRACTICE_TODO(); }
};
