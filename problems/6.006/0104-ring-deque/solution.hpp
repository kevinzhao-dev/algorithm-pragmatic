#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "practice.hpp"

// Stage 1 and 2
template <typename T>
class RingDeque {
public:
    RingDeque() { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }

    const T& get_at(std::size_t i) const { PRACTICE_TODO(); }
    void set_at(std::size_t i, T value) { PRACTICE_TODO(); }

    void insert_first(T value) { PRACTICE_TODO(); }
    void insert_last(T value) { PRACTICE_TODO(); }
    T delete_first() { PRACTICE_TODO(); }
    T delete_last() { PRACTICE_TODO(); }
};

// Stage 3
class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(std::size_t limit, std::int64_t window) { PRACTICE_TODO(); }

    bool allow(std::int64_t t) { PRACTICE_TODO(); }
    std::size_t active(std::int64_t t) { PRACTICE_TODO(); }
};
