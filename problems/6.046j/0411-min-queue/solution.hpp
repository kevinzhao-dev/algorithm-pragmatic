#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>

#include "practice.hpp"

// Stages 1 and 2
template <typename T, typename Compare = std::less<T>>
class MinQueue {
public:
    explicit MinQueue(Compare compare = Compare{}) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    void enqueue(T value) { PRACTICE_TODO(); }
    T dequeue() { PRACTICE_TODO(); }
    const T& front() const { PRACTICE_TODO(); }
    const T& find_min() const { PRACTICE_TODO(); }
};

// Stage 3
class PeakTracker {
public:
    explicit PeakTracker(std::int64_t window) { PRACTICE_TODO(); }

    void record(std::int64_t timestamp, std::int64_t value) { PRACTICE_TODO(); }
    std::optional<std::int64_t> peak(std::int64_t now) { PRACTICE_TODO(); }
};
