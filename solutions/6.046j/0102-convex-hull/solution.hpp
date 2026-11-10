#pragma once

#include <algorithm>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

struct Point {
    std::int64_t x;
    std::int64_t y;
    friend auto operator<=>(const Point&, const Point&) = default;
};

struct Tangent {
    std::size_t left;
    std::size_t right;
};

inline int orient(Point a, Point b, Point c) {
    const std::int64_t cross = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    return (cross > 0) - (cross < 0);
}

namespace hull_detail {

inline std::size_t last_index(std::span<const Point> hull) {
    return static_cast<std::size_t>(std::ranges::max_element(hull) - hull.begin());
}

}  // namespace hull_detail

// Lexicographic order acts as an infinitesimal shear, so ties in x need no special case. Each
// finger only walks its own chain, which also keeps one- and two-point hulls from wrapping.
inline Tangent upper_tangent(std::span<const Point> left, std::span<const Point> right) {
    const std::size_t n = left.size();
    const std::size_t m = right.size();
    const std::size_t left_max = hull_detail::last_index(left);
    const std::size_t right_max = hull_detail::last_index(right);
    const std::size_t left_steps = n == 1 ? 0 : n - left_max;
    const std::size_t right_steps = m == 1 ? 0 : m - right_max;

    const auto a = [&](std::size_t step) -> const Point& { return left[(left_max + step) % n]; };
    const auto b = [&](std::size_t step) -> const Point& { return right[(m - step) % m]; };

    std::size_t i = 0;
    std::size_t j = 0;
    for (bool moved = true; moved;) {
        moved = false;
        while (i < left_steps && orient(a(i), b(j), a(i + 1)) >= 0) {
            ++i;
            moved = true;
        }
        while (j < right_steps && orient(a(i), b(j), b(j + 1)) >= 0) {
            ++j;
            moved = true;
        }
    }
    return {(left_max + i) % n, (m - j) % m};
}

inline Tangent lower_tangent(std::span<const Point> left, std::span<const Point> right) {
    const std::size_t left_steps = hull_detail::last_index(left);
    const std::size_t right_steps = hull_detail::last_index(right);

    const auto a = [&](std::size_t step) -> const Point& { return left[left_steps - step]; };
    const auto b = [&](std::size_t step) -> const Point& { return right[step]; };

    std::size_t i = 0;
    std::size_t j = 0;
    for (bool moved = true; moved;) {
        moved = false;
        while (i < left_steps && orient(a(i), b(j), a(i + 1)) <= 0) {
            ++i;
            moved = true;
        }
        while (j < right_steps && orient(a(i), b(j), b(j + 1)) <= 0) {
            ++j;
            moved = true;
        }
    }
    return {left_steps - i, j};
}

inline std::vector<Point> merge_hulls(std::span<const Point> left, std::span<const Point> right) {
    if (left.empty() || right.empty()) {
        const auto& only = left.empty() ? right : left;
        return {only.begin(), only.end()};
    }
    const Tangent upper = upper_tangent(left, right);
    const Tangent lower = lower_tangent(left, right);

    std::vector<Point> merged;
    merged.reserve(left.size() + right.size());
    for (std::size_t i = 0; i <= lower.left; ++i) {
        merged.push_back(left[i]);
    }
    for (std::size_t j = lower.right;; j = (j + 1) % right.size()) {
        merged.push_back(right[j]);
        if (j == upper.right) {
            break;
        }
    }
    if (upper.left != 0) {
        for (std::size_t i = upper.left; i < left.size(); ++i) {
            merged.push_back(left[i]);
        }
    }
    return merged;
}

namespace hull_detail {

inline std::vector<Point> build(std::span<const Point> sorted) {
    if (sorted.size() <= 2) {
        return {sorted.begin(), sorted.end()};
    }
    const std::size_t middle = sorted.size() / 2;
    const std::vector<Point> left = build(sorted.first(middle));
    const std::vector<Point> right = build(sorted.subspan(middle));
    return merge_hulls(left, right);
}

}  // namespace hull_detail

inline std::vector<Point> convex_hull(std::span<const Point> points) {
    std::vector<Point> sorted(points.begin(), points.end());
    std::ranges::sort(sorted);
    const auto duplicates = std::ranges::unique(sorted);
    sorted.erase(duplicates.begin(), duplicates.end());
    return hull_detail::build(sorted);
}

class ConvexRegion {
public:
    explicit ConvexRegion(std::span<const Point> points) : hull_(convex_hull(points)) {}

    const std::vector<Point>& hull() const { return hull_; }

    bool contains(Point p) const {
        const std::size_t n = hull_.size();
        if (n == 0) {
            return false;
        }
        if (n == 1) {
            return p == hull_[0];
        }
        if (n == 2) {
            return orient(hull_[0], hull_[1], p) == 0 && hull_[0] <= p && p <= hull_[1];
        }
        if (orient(hull_[0], hull_[1], p) < 0 || orient(hull_[0], hull_[n - 1], p) > 0) {
            return false;
        }
        std::size_t low = 1;
        std::size_t high = n - 2;
        while (low < high) {
            const std::size_t middle = (low + high + 1) / 2;
            if (orient(hull_[0], hull_[middle], p) >= 0) {
                low = middle;
            } else {
                high = middle - 1;
            }
        }
        return orient(hull_[low], hull_[low + 1], p) >= 0;
    }

private:
    std::vector<Point> hull_;
};
