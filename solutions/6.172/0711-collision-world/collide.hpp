// collide.hpp — exact narrow phase: do two moving bars touch during one tick?
#pragma once

#include <algorithm>
#include <cstdint>

#include "world_types.hpp"

namespace collide_detail {

constexpr std::int64_t cross(Vec a, Vec b) { return a.x * b.y - a.y * b.x; }
constexpr int sign(std::int64_t x) { return (x > 0) - (x < 0); }
constexpr int orientation(Vec a, Vec b, Vec c) { return sign(cross(b - a, c - a)); }

// For a point already known to be collinear with the segment ab.
constexpr bool on_segment(Vec a, Vec b, Vec point) {
    return std::min(a.x, b.x) <= point.x && point.x <= std::max(a.x, b.x) &&
           std::min(a.y, b.y) <= point.y && point.y <= std::max(a.y, b.y);
}

constexpr bool segments_touch(Vec a, Vec b, Vec c, Vec d) {
    const int d1 = orientation(c, d, a);
    const int d2 = orientation(c, d, b);
    const int d3 = orientation(a, b, c);
    const int d4 = orientation(a, b, d);
    if (d1 * d2 < 0 && d3 * d4 < 0) {
        return true;
    }
    return (d1 == 0 && on_segment(c, d, a)) || (d2 == 0 && on_segment(c, d, b)) ||
           (d3 == 0 && on_segment(a, b, c)) || (d4 == 0 && on_segment(a, b, d));
}

// Is `point` inside the parallelogram origin + s*e1 + t*e2, s and t in [0, 1]? Solved by
// Cramer's rule with the denominator kept on the integer side. A degenerate parallelogram has
// no interior; its boundary edges already cover all of it.
constexpr bool inside_parallelogram(Vec point, Vec origin, Vec e1, Vec e2) {
    std::int64_t denominator = cross(e1, e2);
    if (denominator == 0) {
        return false;
    }
    const Vec r = point - origin;
    std::int64_t s = cross(r, e2);
    std::int64_t t = cross(e1, r);
    if (denominator < 0) {
        denominator = -denominator;
        s = -s;
        t = -t;
    }
    return 0 <= s && s <= denominator && 0 <= t && t <= denominator;
}

}  // namespace collide_detail

// Works in b's rest frame: there a sweeps a parallelogram and b does not move, so the bars
// touch during the tick exactly when b meets that closed parallelogram.
inline bool collides(const Segment& a, const Segment& b) {
    using namespace collide_detail;
    const Vec w = a.v - b.v;

    const std::int64_t min_x = std::min(a.p.x, a.q.x) + std::min<std::int64_t>(0, w.x);
    const std::int64_t max_x = std::max(a.p.x, a.q.x) + std::max<std::int64_t>(0, w.x);
    const std::int64_t min_y = std::min(a.p.y, a.q.y) + std::min<std::int64_t>(0, w.y);
    const std::int64_t max_y = std::max(a.p.y, a.q.y) + std::max<std::int64_t>(0, w.y);
    if (max_x < std::min(b.p.x, b.q.x) || std::max(b.p.x, b.q.x) < min_x ||
        max_y < std::min(b.p.y, b.q.y) || std::max(b.p.y, b.q.y) < min_y) {
        return false;
    }

    const Vec p_end = a.p + w;
    const Vec q_end = a.q + w;
    return segments_touch(b.p, b.q, a.p, a.q) || segments_touch(b.p, b.q, a.q, q_end) ||
           segments_touch(b.p, b.q, q_end, p_end) || segments_touch(b.p, b.q, p_end, a.p) ||
           inside_parallelogram(b.p, a.p, a.q - a.p, w);
}

// The default narrow phase; detectors take it as a parameter so tests can count calls.
struct ExactPredicate {
    bool operator()(const Segment& a, const Segment& b) const { return collides(a, b); }
};
