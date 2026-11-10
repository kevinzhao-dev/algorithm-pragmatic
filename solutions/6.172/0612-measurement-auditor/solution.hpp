#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <span>
#include <stdexcept>
#include <vector>

struct Measurement {
    int processors = 1;
    double seconds = 0;
};

struct Audit {
    bool consistent = false;
    double min_work = 0;
    double max_work = 0;
    double min_span = 0;
    double max_span = 0;
    double min_parallelism = 0;
    double max_parallelism = 0;
};

namespace detail_0612 {

struct Point {
    double x;  // T1
    double y;  // T-infinity
};

// a*x + b*y <= c
struct Constraint {
    double a;
    double b;
    double c;
};

inline std::vector<Constraint> constraints_of(std::span<const Measurement> measurements) {
    std::vector<Constraint> constraints = {
        {0, -1, 0},   // T-infinity >= 0
        {-1, 1, 0},   // work is at least span
    };
    for (const Measurement& m : measurements) {
        if (m.processors < 1 || !(m.seconds > 0)) {
            throw std::invalid_argument("measurements need processors >= 1 and seconds > 0");
        }
        const auto p = static_cast<double>(m.processors);
        constraints.push_back({1, 0, p * m.seconds});                // Work Law
        constraints.push_back({0, 1, m.seconds});                    // Span Law
        constraints.push_back({-1, -(p - 1), -p * m.seconds});       // greedy bound
    }
    return constraints;
}

inline bool feasible(const Point& point, std::span<const Constraint> constraints) {
    for (const Constraint& c : constraints) {
        const double tolerance = 1e-9 * (1.0 + std::abs(c.c) + std::abs(c.a * point.x) +
                                         std::abs(c.b * point.y));
        if (c.a * point.x + c.b * point.y > c.c + tolerance) {
            return false;
        }
    }
    return true;
}

inline bool intersect(const Constraint& a, const Constraint& b, Point& out) {
    const double det = a.a * b.b - a.b * b.a;
    if (std::abs(det) < 1e-12) {
        return false;
    }
    out.x = (a.c * b.b - a.b * b.c) / det;
    out.y = (a.a * b.c - a.c * b.a) / det;
    return true;
}

// Feasible intersections of pairs of constraint lines: the vertices of the polygon.
inline std::vector<Point> vertices(std::span<const Constraint> constraints) {
    std::vector<Point> result;
    for (std::size_t i = 0; i < constraints.size(); ++i) {
        for (std::size_t j = i + 1; j < constraints.size(); ++j) {
            Point point{};
            if (intersect(constraints[i], constraints[j], point) && feasible(point, constraints)) {
                result.push_back(point);
            }
        }
    }
    return result;
}

}  // namespace detail_0612

inline Audit audit_measurements(std::span<const Measurement> measurements) {
    using namespace detail_0612;
    if (measurements.empty()) {
        throw std::invalid_argument("at least one measurement is required");
    }
    const std::vector<Constraint> constraints = constraints_of(measurements);
    const std::vector<Point> points = vertices(constraints);
    Audit audit;
    if (points.empty()) {
        return audit;
    }
    audit.consistent = true;
    audit.min_work = audit.max_work = points[0].x;
    audit.min_span = audit.max_span = points[0].y;
    audit.min_parallelism = std::numeric_limits<double>::infinity();
    audit.max_parallelism = 0;
    for (const Point& point : points) {
        audit.min_work = std::min(audit.min_work, point.x);
        audit.max_work = std::max(audit.max_work, point.x);
        audit.min_span = std::min(audit.min_span, point.y);
        audit.max_span = std::max(audit.max_span, point.y);
        const double ratio =
            point.y > 1e-12 ? point.x / point.y : std::numeric_limits<double>::infinity();
        audit.min_parallelism = std::min(audit.min_parallelism, ratio);
        audit.max_parallelism = std::max(audit.max_parallelism, ratio);
    }
    if (audit.min_span <= 1e-12) {
        audit.min_span = 0;
    }
    return audit;
}

inline std::vector<std::size_t> minimal_conflict(std::span<const Measurement> measurements) {
    const std::size_t n = measurements.size();
    if (n == 0 || audit_measurements(measurements).consistent) {
        return {};
    }
    std::vector<Measurement> subset;
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            subset = {measurements[i], measurements[j]};
            if (!audit_measurements(subset).consistent) {
                return {i, j};
            }
        }
    }
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n; ++j) {
            for (std::size_t k = j + 1; k < n; ++k) {
                subset = {measurements[i], measurements[j], measurements[k]};
                if (!audit_measurements(subset).consistent) {
                    return {i, j, k};
                }
            }
        }
    }
    return {};  // unreachable: a 2D infeasibility always has a witness of at most 3 constraints
}

struct RuntimeBounds {
    double low = 0;
    double high = 0;
};

inline RuntimeBounds predict_runtime(std::span<const Measurement> measurements, int processors) {
    using namespace detail_0612;
    if (measurements.empty() || processors < 1) {
        throw std::invalid_argument("need measurements and processors >= 1");
    }
    const std::vector<Constraint> constraints = constraints_of(measurements);
    std::vector<Point> points = vertices(constraints);
    if (points.empty()) {
        throw std::domain_error("the measurements are inconsistent");
    }
    const auto p = static_cast<double>(processors);
    // The minimum of max(x/p, y) can also lie where the two pieces cross a polygon edge.
    const Constraint crossing{1, -p, 0};  // x = p * y
    for (const Constraint& edge : constraints) {
        Point point{};
        if (intersect(edge, crossing, point) && feasible(point, constraints)) {
            points.push_back(point);
        }
    }
    RuntimeBounds bounds{std::numeric_limits<double>::infinity(), 0.0};
    for (const Point& point : points) {
        bounds.low = std::min(bounds.low, std::max(point.x / p, point.y));
        bounds.high = std::max(bounds.high, (point.x - point.y) / p + point.y);
    }
    return bounds;
}
