#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>

#include "practice.hpp"

// Stage 1
template <typename T>
std::size_t count_longest_runs(std::span<const T> values) {
    PRACTICE_TODO();
}

// Stage 2
template <typename T, typename Less = std::less<T>>
class RunTracker {
public:
    void push(const T& value) { PRACTICE_TODO(); }

    std::size_t longest() const { PRACTICE_TODO(); }
    std::size_t count() const { PRACTICE_TODO(); }
    std::size_t current() const { PRACTICE_TODO(); }
};

// Stage 3
template <typename T, typename Less = std::less<T>>
class StreakAlarm {
public:
    explicit StreakAlarm(std::size_t threshold) { PRACTICE_TODO(); }

    bool observe(const T& value) { PRACTICE_TODO(); }
};
