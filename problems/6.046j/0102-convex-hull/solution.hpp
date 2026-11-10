#pragma once

#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

struct Point {
    std::int64_t x;
    std::int64_t y;
    friend auto operator<=>(const Point&, const Point&) = default;
};

struct Tangent {
    std::size_t left;
    std::size_t right;
};

// Stage 1
inline int orient(Point a, Point b, Point c) { PRACTICE_TODO(); }
inline Tangent upper_tangent(std::span<const Point> left, std::span<const Point> right) {
    PRACTICE_TODO();
}
inline Tangent lower_tangent(std::span<const Point> left, std::span<const Point> right) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<Point> merge_hulls(std::span<const Point> left, std::span<const Point> right) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<Point> convex_hull(std::span<const Point> points) { PRACTICE_TODO(); }

// Stage 4
class ConvexRegion {
public:
    explicit ConvexRegion(std::span<const Point> points) { PRACTICE_TODO(); }

    const std::vector<Point>& hull() const { PRACTICE_TODO(); }
    bool contains(Point p) const { PRACTICE_TODO(); }
};
