#pragma once

#include <cstddef>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename T = int>
class RangeTree1D {
public:
    explicit RangeTree1D(std::vector<T> keys) { PRACTICE_TODO(); }

    std::size_t count(const T& a, const T& b) const { PRACTICE_TODO(); }
    std::vector<std::size_t> report(const T& a, const T& b) const { PRACTICE_TODO(); }
    // Half-open ranges [first, second) of positions in sorted order.
    std::vector<std::pair<std::size_t, std::size_t>> canonical(const T& a, const T& b) const {
        PRACTICE_TODO();
    }
};

// Stage 2
template <typename T = int>
struct Point {
    T x{};
    T y{};
};

template <typename T = int>
class RangeTree2D {
public:
    explicit RangeTree2D(std::vector<Point<T>> pts) { PRACTICE_TODO(); }

    std::size_t count(const T& x1, const T& x2, const T& y1, const T& y2) const { PRACTICE_TODO(); }
    std::vector<std::size_t> report(const T& x1, const T& x2, const T& y1, const T& y2) const {
        PRACTICE_TODO();
    }
};

// Stage 3
template <typename T, typename V>
struct WeightedPoint {
    T x{};
    T y{};
    V weight{};
};

template <typename T, typename V>
class WeightedRangeTree {
public:
    explicit WeightedRangeTree(std::vector<WeightedPoint<T, V>> pts) { PRACTICE_TODO(); }

    V sum(const T& x1, const T& x2, const T& y1, const T& y2) const { PRACTICE_TODO(); }
    std::size_t count(const T& x1, const T& x2, const T& y1, const T& y2) const { PRACTICE_TODO(); }
};
