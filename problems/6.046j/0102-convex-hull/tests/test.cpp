#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Hull = std::vector<Point>;

// Found through ADL when a failed check prints a point.
std::ostream& operator<<(std::ostream& os, const Point& p) {
    return os << '(' << p.x << ", " << p.y << ')';
}

namespace {

std::int64_t cross(Point a, Point b, Point c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}

// Monotone chain: an independent oracle in the canonical form the README specifies.
Hull oracle_hull(std::vector<Point> points) {
    std::ranges::sort(points);
    const auto repeats = std::ranges::unique(points);
    points.erase(repeats.begin(), repeats.end());
    if (points.size() <= 2) {
        return points;
    }
    Hull lower;
    for (const Point& p : points) {
        while (lower.size() >= 2 && cross(lower[lower.size() - 2], lower.back(), p) <= 0) {
            lower.pop_back();
        }
        lower.push_back(p);
    }
    Hull upper;
    for (auto it = points.rbegin(); it != points.rend(); ++it) {
        while (upper.size() >= 2 && cross(upper[upper.size() - 2], upper.back(), *it) <= 0) {
            upper.pop_back();
        }
        upper.push_back(*it);
    }
    lower.pop_back();
    upper.pop_back();
    lower.insert(lower.end(), upper.begin(), upper.end());
    return lower;
}

std::vector<Point> random_points(Rng& rng, std::size_t n, std::int64_t low, std::int64_t high) {
    std::vector<Point> points(n);
    for (Point& p : points) {
        p = {rng.uniform(low, high), rng.uniform(low, high)};
    }
    return points;
}

// Splits a point set into two lexicographically separated halves and hulls each one.
std::pair<Hull, Hull> two_hulls(std::vector<Point> points) {
    std::ranges::sort(points);
    const auto repeats = std::ranges::unique(points);
    points.erase(repeats.begin(), repeats.end());
    const std::size_t middle = points.size() / 2;
    return {oracle_hull({points.begin(), points.begin() + static_cast<std::ptrdiff_t>(middle)}),
            oracle_hull({points.begin() + static_cast<std::ptrdiff_t>(middle), points.end()})};
}

Tangent brute_tangent(const Hull& left, const Hull& right, int side) {
    bool found = false;
    Tangent best{};
    for (std::size_t i = 0; i < left.size(); ++i) {
        for (std::size_t j = 0; j < right.size(); ++j) {
            bool supports = true;
            for (const Hull* hull : {&left, &right}) {
                for (const Point& p : *hull) {
                    if (side * orient(left[i], right[j], p) > 0) {
                        supports = false;
                    }
                }
            }
            if (!supports) {
                continue;
            }
            // Every supporting pair lies on one line; keep the outermost vertices.
            if (!found || left[i] < left[best.left]) {
                best.left = i;
            }
            if (!found || right[j] > right[best.right]) {
                best.right = j;
            }
            found = true;
        }
    }
    REQUIRE(found);
    return best;
}

// A strictly convex polygon with 4 * half_width vertices, listed in canonical order.
Hull lens(std::int64_t half_width, std::int64_t shift) {
    Hull hull;
    for (std::int64_t x = -half_width; x <= half_width; ++x) {
        hull.push_back({x + shift, x * x});
    }
    for (std::int64_t x = half_width - 1; x > -half_width; --x) {
        hull.push_back({x + shift, 2 * half_width * half_width - x * x});
    }
    return hull;
}

constexpr std::int64_t kMax = 1'000'000'000;

}  // namespace

TEST(1, "orient reports the turn direction") {
    CHECK_EQ(orient({0, 0}, {1, 0}, {0, 1}), 1);
    CHECK_EQ(orient({0, 0}, {1, 0}, {0, -1}), -1);
    CHECK_EQ(orient({0, 0}, {1, 1}, {2, 2}), 0);
    CHECK_EQ(orient({0, 0}, {1, 1}, {-3, -3}), 0);
}

TEST(1, "orient is exact at the extreme coordinates") {
    CHECK_EQ(orient({-kMax, -kMax}, {kMax, -kMax}, {kMax, kMax}), 1);
    CHECK_EQ(orient({-kMax, -kMax}, {kMax, kMax}, {kMax, -kMax}), -1);
    CHECK_EQ(orient({-kMax, -kMax}, {kMax, kMax}, {0, 0}), 0);
}

TEST(1, "orient decides a nearly collinear triple that a double cannot") {
    // Consecutive Fibonacci vectors: the cross product is exactly +-1 despite ~1e18 terms.
    constexpr std::int64_t f42 = 267914296;
    constexpr std::int64_t f43 = 433494437;
    constexpr std::int64_t f44 = 701408733;
    CHECK_EQ(orient({0, 0}, {f44, f43}, {f43, f42}), -1);
    CHECK_EQ(orient({0, 0}, {f43, f42}, {f44, f43}), 1);
}

TEST(1, "tangents between two triangles") {
    const Hull left{{0, 0}, {2, 0}, {1, 3}};
    const Hull right{{4, 1}, {6, 1}, {5, 4}};
    const Tangent upper = upper_tangent(left, right);
    const Tangent lower = lower_tangent(left, right);
    CHECK_EQ(upper.left, 2);
    CHECK_EQ(upper.right, 2);
    CHECK_EQ(lower.left, 1);
    CHECK_EQ(lower.right, 1);
}

TEST(1, "tangents between single points") {
    const Hull left{{0, 0}};
    const Hull right{{3, 7}};
    CHECK_EQ(upper_tangent(left, right).left, 0);
    CHECK_EQ(upper_tangent(left, right).right, 0);
    CHECK_EQ(lower_tangent(left, right).left, 0);
    CHECK_EQ(lower_tangent(left, right).right, 0);
}

TEST(1, "a tangent running along an edge reports the outermost vertices") {
    const Hull left{{0, 0}, {3, 0}, {3, 3}, {0, 3}};
    const Hull right{{5, 0}, {7, 0}, {7, 3}, {5, 3}};
    const Tangent upper = upper_tangent(left, right);
    const Tangent lower = lower_tangent(left, right);
    CHECK_EQ(left[upper.left], Point{0, 3});
    CHECK_EQ(right[upper.right], Point{7, 3});
    CHECK_EQ(left[lower.left], Point{0, 0});
    CHECK_EQ(right[lower.right], Point{7, 0});
}

TEST(1, "tangents work when both hulls share an x coordinate") {
    const Hull left{{0, 0}, {0, 2}};
    const Hull right{{0, 5}, {0, 9}};
    const Tangent upper = upper_tangent(left, right);
    const Tangent lower = lower_tangent(left, right);
    CHECK_EQ(left[upper.left], Point{0, 0});
    CHECK_EQ(right[upper.right], Point{0, 9});
    CHECK_EQ(left[lower.left], Point{0, 0});
    CHECK_EQ(right[lower.right], Point{0, 9});
}

TEST(1, "random hulls match a brute-force tangent search") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        INFO("trial ", trial);
        const std::int64_t span = rng.coin() ? 4 : 40;
        const auto [left, right] =
            two_hulls(random_points(rng, static_cast<std::size_t>(rng.uniform(2, 24)), 0, span));
        if (left.empty() || right.empty()) {
            continue;
        }
        const Tangent upper = upper_tangent(left, right);
        const Tangent lower = lower_tangent(left, right);
        const Tangent expected_upper = brute_tangent(left, right, 1);
        const Tangent expected_lower = brute_tangent(left, right, -1);
        CHECK_EQ(upper.left, expected_upper.left);
        CHECK_EQ(upper.right, expected_upper.right);
        CHECK_EQ(lower.left, expected_lower.left);
        CHECK_EQ(lower.right, expected_lower.right);
    }
}

TEST(2, "merging keeps only the outer vertices") {
    const Hull left{{0, 0}, {2, 0}, {1, 3}};
    const Hull right{{4, 1}, {6, 1}, {5, 4}};
    CHECK_EQ(merge_hulls(left, right), (Hull{{0, 0}, {2, 0}, {6, 1}, {5, 4}, {1, 3}}));
}

TEST(2, "merging with an empty hull returns the other one") {
    const Hull hull{{0, 0}, {1, 0}, {0, 1}};
    CHECK_EQ(merge_hulls({}, hull), hull);
    CHECK_EQ(merge_hulls(hull, {}), hull);
    CHECK(merge_hulls({}, {}).empty());
}

TEST(2, "merging two collinear segments keeps only the endpoints") {
    const Hull left{{0, 0}, {1, 1}};
    const Hull right{{2, 2}, {5, 5}};
    CHECK_EQ(merge_hulls(left, right), (Hull{{0, 0}, {5, 5}}));
}

TEST(2, "vertices that become collinear at the seam are removed") {
    const Hull left{{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    const Hull right{{2, 0}, {3, 0}, {3, 1}, {2, 1}};
    CHECK_EQ(merge_hulls(left, right), (Hull{{0, 0}, {3, 0}, {3, 1}, {0, 1}}));
}

TEST(2, "random hulls merge into the hull of the union") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        INFO("trial ", trial);
        const std::int64_t span = rng.coin() ? 5 : 60;
        auto points = random_points(rng, static_cast<std::size_t>(rng.uniform(2, 40)), 0, span);
        const auto [left, right] = two_hulls(points);
        CHECK_EQ(merge_hulls(left, right), oracle_hull(points));
    }
}

PERF(2, "merging is linear in the hull sizes", 800) {
    const Hull left = lens(22'000, -100'000);
    const Hull right = lens(22'000, 100'000);
    Hull points = left;
    points.insert(points.end(), right.begin(), right.end());
    const Hull expected = oracle_hull(points);
    for (int repeat = 0; repeat < 8; ++repeat) {
        CHECK_EQ(merge_hulls(left, right).size(), expected.size());
    }
    CHECK_EQ(merge_hulls(left, right), expected);
}

TEST(3, "no points gives an empty hull") { CHECK(convex_hull({}).empty()); }

TEST(3, "one and two points are their own hull") {
    const Hull one{{4, 4}};
    CHECK_EQ(convex_hull(one), one);
    const Hull two{{5, 1}, {2, 8}};
    CHECK_EQ(convex_hull(two), (Hull{{2, 8}, {5, 1}}));
}

TEST(3, "duplicates collapse") {
    const Hull points{{1, 1}, {1, 1}, {1, 1}};
    CHECK_EQ(convex_hull(points), (Hull{{1, 1}}));
    const Hull pair{{1, 1}, {2, 3}, {1, 1}, {2, 3}};
    CHECK_EQ(convex_hull(pair), (Hull{{1, 1}, {2, 3}}));
}

TEST(3, "all points on one line reduce to its endpoints") {
    Hull points;
    for (std::int64_t i = 0; i < 25; ++i) {
        points.push_back({i, 2 * i + 1});
    }
    CHECK_EQ(convex_hull(points), (Hull{{0, 1}, {24, 49}}));
}

TEST(3, "points on a vertical line reduce to its endpoints") {
    const Hull points{{7, 3}, {7, -2}, {7, 9}, {7, 0}};
    CHECK_EQ(convex_hull(points), (Hull{{7, -2}, {7, 9}}));
}

TEST(3, "a full grid keeps only its four corners") {
    Hull points;
    for (std::int64_t x = 0; x <= 9; ++x) {
        for (std::int64_t y = 0; y <= 9; ++y) {
            points.push_back({x, y});
        }
    }
    CHECK_EQ(convex_hull(points), (Hull{{0, 0}, {9, 0}, {9, 9}, {0, 9}}));
}

TEST(3, "the extreme coordinate box is handled exactly") {
    Hull points{{-kMax, -kMax}, {kMax, -kMax}, {kMax, kMax}, {-kMax, kMax}, {0, 0}, {kMax - 1, 0}};
    CHECK_EQ(convex_hull(points), (Hull{{-kMax, -kMax}, {kMax, -kMax}, {kMax, kMax}, {-kMax, kMax}}));
}

TEST(3, "random point sets match the oracle hull") {
    Rng rng;
    for (int trial = 0; trial < 4000; ++trial) {
        INFO("trial ", trial);
        const std::int64_t span = rng.coin() ? 3 : (rng.coin() ? 12 : 200);
        const auto points = random_points(rng, static_cast<std::size_t>(rng.uniform(0, 60)), -span, span);
        CHECK_EQ(convex_hull(points), oracle_hull(points));
    }
}

PERF(3, "one million random points", 2000) {
    Rng rng;
    const auto points = random_points(rng, 1'000'000, -kMax, kMax);
    CHECK_EQ(convex_hull(points).size(), oracle_hull(points).size());
}

PERF(3, "every point is on the hull", 400) {
    Rng rng;
    Hull points = lens(22'000, 0);
    rng.shuffle(points);
    CHECK_EQ(convex_hull(points).size(), 88'000);
}

TEST(4, "an empty region contains nothing") {
    const ConvexRegion region(Hull{});
    CHECK(region.hull().empty());
    CHECK(!region.contains({0, 0}));
}

TEST(4, "a single point region contains only that point") {
    const ConvexRegion region(Hull{{3, 3}, {3, 3}});
    CHECK(region.contains({3, 3}));
    CHECK(!region.contains({3, 4}));
}

TEST(4, "a segment region contains only its own points") {
    const ConvexRegion region(Hull{{0, 0}, {6, 3}});
    CHECK(region.contains({0, 0}));
    CHECK(region.contains({2, 1}));
    CHECK(region.contains({6, 3}));
    CHECK(!region.contains({8, 4}));
    CHECK(!region.contains({-2, -1}));
    CHECK(!region.contains({2, 2}));
}

TEST(4, "the boundary of a polygon counts as inside") {
    const ConvexRegion region(Hull{{0, 0}, {4, 0}, {4, 4}, {0, 4}});
    for (const Point& p : Hull{{0, 0}, {2, 0}, {4, 4}, {4, 2}, {0, 3}, {2, 4}}) {
        INFO("p=", p.x, ",", p.y);
        CHECK(region.contains(p));
    }
    for (const Point& p : Hull{{5, 0}, {-1, 0}, {2, 5}, {2, -1}, {4, 5}, {-1, 4}}) {
        INFO("p=", p.x, ",", p.y);
        CHECK(!region.contains(p));
    }
}

TEST(4, "the region keeps its own copy of the hull") {
    Hull input{{0, 0}, {5, 0}, {0, 5}};
    const ConvexRegion region(input);
    input.assign(100, Point{9, 9});
    input.clear();
    input.shrink_to_fit();
    CHECK(region.contains({1, 1}));
    CHECK(!region.contains({9, 9}));
}

TEST(4, "random queries match a test against every edge") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        const std::int64_t span = rng.coin() ? 3 : 25;
        const auto points = random_points(rng, static_cast<std::size_t>(rng.uniform(1, 30)), -span, span);
        const ConvexRegion region(points);
        const Hull hull = oracle_hull(points);
        REQUIRE_EQ(region.hull(), hull);
        for (int q = 0; q < 40; ++q) {
            const Point p{rng.uniform(-span - 2, span + 2), rng.uniform(-span - 2, span + 2)};
            bool inside = true;
            if (hull.size() == 1) {
                inside = p == hull[0];
            } else if (hull.size() == 2) {
                inside = cross(hull[0], hull[1], p) == 0 && hull[0] <= p && p <= hull[1];
            } else {
                for (std::size_t i = 0; i < hull.size(); ++i) {
                    inside = inside && cross(hull[i], hull[(i + 1) % hull.size()], p) >= 0;
                }
            }
            INFO("query ", p.x, ",", p.y);
            REQUIRE_EQ(region.contains(p), inside);
        }
    }
}

PERF(4, "a million queries against a large polygon", 800) {
    Rng rng;
    const ConvexRegion region(lens(22'000, 0));
    std::int64_t inside = 0;
    for (int q = 0; q < 1'000'000; ++q) {
        inside += region.contains({rng.uniform(-25'000, 25'000), rng.uniform(-1'000, 1'000'000'000)});
    }
    CHECK_GT(inside, 0);
}
