#include <algorithm>
#include <cmath>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

std::size_t ceil_lg(std::size_t n) {
    std::size_t bits = 0;
    while ((std::size_t{1} << bits) < n) {
        ++bits;
    }
    return bits;
}

template <typename V>
std::vector<V> sorted(std::vector<V> values) {
    std::sort(values.begin(), values.end());
    return values;
}

std::vector<std::size_t> brute_1d(const std::vector<int>& keys, int a, int b) {
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < keys.size(); ++i) {
        if (a <= keys[i] && keys[i] <= b) {
            out.push_back(i);
        }
    }
    return out;
}

std::vector<std::size_t> brute_2d(const std::vector<Point<int>>& pts, int x1, int x2, int y1,
                                  int y2) {
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < pts.size(); ++i) {
        if (x1 <= pts[i].x && pts[i].x <= x2 && y1 <= pts[i].y && pts[i].y <= y2) {
            out.push_back(i);
        }
    }
    return out;
}

std::vector<Point<int>> random_points(Rng& rng, std::size_t n, int range) {
    std::vector<Point<int>> pts(n);
    for (auto& p : pts) {
        p.x = static_cast<int>(rng.uniform(0, range));
        p.y = static_cast<int>(rng.uniform(0, range));
    }
    return pts;
}

}  // namespace

TEST(1, "empty key set answers every query with nothing") {
    RangeTree1D<int> tree({});
    CHECK_EQ(tree.count(-5, 5), 0);
    CHECK(tree.report(-5, 5).empty());
    CHECK(tree.canonical(-5, 5).empty());
}

TEST(1, "a reversed range is empty") {
    RangeTree1D<int> tree({1, 2, 3});
    CHECK_EQ(tree.count(3, 1), 0);
    CHECK(tree.report(3, 1).empty());
    CHECK(tree.canonical(3, 1).empty());
}

TEST(1, "all keys equal: boundaries are inclusive") {
    RangeTree1D<int> tree({7, 7, 7, 7});
    CHECK_EQ(tree.count(7, 7), 4);
    CHECK_EQ(tree.count(6, 6), 0);
    CHECK_EQ(tree.count(8, 9), 0);
    CHECK_EQ(sorted(tree.report(0, 100)), std::vector<std::size_t>{0, 1, 2, 3});
}

TEST(1, "indices refer to the original input order") {
    RangeTree1D<int> tree({30, 10, 40, 20});
    CHECK_EQ(sorted(tree.report(15, 35)), std::vector<std::size_t>{0, 3});
    CHECK_EQ(tree.count(15, 35), 2);
}

TEST(1, "canonical pieces partition exactly the answer") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 200));
        const auto keys = rng.ints(n, 0, 60);
        const RangeTree1D<int> tree(keys);
        const int a = static_cast<int>(rng.uniform(-5, 65));
        const int b = static_cast<int>(rng.uniform(-5, 65));
        INFO("n=", n, " a=", a, " b=", b);

        const auto pieces = tree.canonical(a, b);
        CHECK_LE(pieces.size(), 2 * ceil_lg(n) + 2);

        std::vector<std::size_t> positions;
        for (const auto& [l, r] : pieces) {
            REQUIRE(l < r);
            REQUIRE(r <= n);
            for (std::size_t i = l; i < r; ++i) {
                positions.push_back(i);
            }
        }
        std::sort(positions.begin(), positions.end());
        CHECK(std::adjacent_find(positions.begin(), positions.end()) == positions.end());
        CHECK_EQ(positions.size(), tree.count(a, b));
        if (!positions.empty()) {
            CHECK_EQ(positions.back() - positions.front() + 1, positions.size());
        }
    }
}

TEST(1, "random queries match a linear scan") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 400));
        const auto keys = rng.ints(n, -100, 100);
        const RangeTree1D<int> tree(keys);
        for (int q = 0; q < 30; ++q) {
            const int a = static_cast<int>(rng.uniform(-120, 120));
            const int b = static_cast<int>(rng.uniform(a - 5, 130));
            INFO("n=", n, " a=", a, " b=", b);
            const auto expected = brute_1d(keys, a, b);
            CHECK_EQ(tree.count(a, b), expected.size());
            CHECK_EQ(sorted(tree.report(a, b)), expected);
        }
    }
}

TEST(1, "count does logarithmically many comparisons") {
    std::vector<Counted<int>> keys;
    for (int i = 0; i < 1 << 16; ++i) {
        keys.emplace_back(i * 3);
    }
    const RangeTree1D<Counted<int>> tree(std::move(keys));
    const std::size_t lg = ceil_lg(1 << 16);
    Rng rng;
    for (int q = 0; q < 2000; ++q) {
        const Counted<int> a(static_cast<int>(rng.uniform(-10, 200'000)));
        const Counted<int> b(static_cast<int>(rng.uniform(-10, 200'000)));
        Counted<int>::comparisons() = 0;
        const auto answer = tree.count(a, b);
        static_cast<void>(answer);
        if (!CHECK_LE(Counted<int>::comparisons(), 4 * lg + 8)) return;
    }
}

TEST(2, "empty point set and empty rectangles") {
    RangeTree2D<int> none({});
    CHECK_EQ(none.count(0, 9, 0, 9), 0);
    RangeTree2D<int> some({{1, 1}, {2, 2}});
    CHECK_EQ(some.count(2, 1, 0, 9), 0);
    CHECK_EQ(some.count(0, 9, 2, 1), 0);
    CHECK(some.report(2, 1, 0, 9).empty());
    CHECK(some.report(0, 9, 2, 1).empty());
}

TEST(2, "a single point on the boundary is inside") {
    RangeTree2D<int> tree({{5, -3}});
    CHECK_EQ(tree.count(5, 5, -3, -3), 1);
    CHECK_EQ(tree.count(4, 4, -3, -3), 0);
    CHECK_EQ(tree.count(5, 5, -2, -2), 0);
    CHECK_EQ(tree.report(0, 9, -9, 0), std::vector<std::size_t>{0});
}

TEST(2, "duplicate points are reported separately") {
    RangeTree2D<int> tree({{2, 2}, {2, 2}, {2, 3}});
    CHECK_EQ(tree.count(2, 2, 2, 2), 2);
    CHECK_EQ(sorted(tree.report(2, 2, 2, 3)), std::vector<std::size_t>{0, 1, 2});
}

TEST(2, "points sharing one coordinate") {
    std::vector<Point<int>> pts;
    for (int i = 0; i < 64; ++i) {
        pts.push_back({7, i});
    }
    const RangeTree2D<int> tree(pts);
    CHECK_EQ(tree.count(7, 7, 10, 19), 10);
    CHECK_EQ(tree.count(6, 8, 60, 100), 4);
    CHECK_EQ(tree.count(8, 9, 0, 63), 0);
}

TEST(2, "random rectangles match a linear scan") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 300));
        const int range = static_cast<int>(rng.uniform(2, 80));
        const auto pts = random_points(rng, n, range);
        const RangeTree2D<int> tree(pts);
        for (int q = 0; q < 40; ++q) {
            const int x1 = static_cast<int>(rng.uniform(-3, range + 3));
            const int x2 = static_cast<int>(rng.uniform(x1 - 3, range + 3));
            const int y1 = static_cast<int>(rng.uniform(-3, range + 3));
            const int y2 = static_cast<int>(rng.uniform(y1 - 3, range + 3));
            INFO("n=", n, " rect=", x1, ",", x2, ",", y1, ",", y2);
            const auto expected = brute_2d(pts, x1, x2, y1, y2);
            CHECK_EQ(tree.count(x1, x2, y1, y2), expected.size());
            CHECK_EQ(sorted(tree.report(x1, x2, y1, y2)), expected);
        }
    }
}

TEST(2, "count does O(log^2 n) comparisons") {
    Rng rng;
    const std::size_t n = 1 << 14;
    std::vector<Point<Counted<int>>> pts(n);
    for (auto& p : pts) {
        p.x = Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000)));
        p.y = Counted<int>(static_cast<int>(rng.uniform(0, 1'000'000)));
    }
    const RangeTree2D<Counted<int>> tree(std::move(pts));
    const std::size_t lg = ceil_lg(n);
    for (int q = 0; q < 500; ++q) {
        const Counted<int> x1(static_cast<int>(rng.uniform(0, 1'000'000)));
        const Counted<int> x2(static_cast<int>(rng.uniform(0, 1'000'000)));
        const Counted<int> y1(static_cast<int>(rng.uniform(0, 1'000'000)));
        const Counted<int> y2(static_cast<int>(rng.uniform(0, 1'000'000)));
        Counted<int>::comparisons() = 0;
        const auto answer = tree.count(x1, x2, y1, y2);
        static_cast<void>(answer);
        if (!CHECK_LE(Counted<int>::comparisons(), 8 * lg * lg)) return;
    }
}

PERF(2, "one hundred thousand rectangle counts", 1500) {
    Rng rng;
    const auto pts = random_points(rng, 100'000, 1'000'000);
    const RangeTree2D<int> tree(pts);
    std::size_t total = 0;
    for (int q = 0; q < 100'000; ++q) {
        const int x1 = static_cast<int>(rng.uniform(0, 1'000'000));
        const int y1 = static_cast<int>(rng.uniform(0, 1'000'000));
        total += tree.count(x1, x1 + 300'000, y1, y1 + 300'000);
    }
    CHECK_GT(total, 0);
}

TEST(3, "empty rectangle sums to zero") {
    WeightedRangeTree<int, long long> tree({{1, 2, 10}});
    CHECK_EQ(tree.sum(3, 2, 0, 9), 0);
    CHECK_EQ(tree.sum(0, 9, 3, 2), 0);
    CHECK_EQ(tree.count(3, 2, 0, 9), 0);
}

TEST(3, "sums and counts over a small log") {
    WeightedRangeTree<int, long long> tree(
        {{0, 100, 5}, {0, 200, 7}, {10, 100, 11}, {10, 900, 13}, {20, 200, 17}});
    CHECK_EQ(tree.sum(0, 10, 100, 200), 5 + 7 + 11);
    CHECK_EQ(tree.count(0, 10, 100, 200), 3);
    CHECK_EQ(tree.sum(0, 20, 200, 900), 7 + 13 + 17);
    CHECK_EQ(tree.sum(11, 19, 0, 1000), 0);
}

TEST(3, "random sums match a linear scan") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 300));
        std::vector<WeightedPoint<int, long long>> pts(n);
        for (auto& p : pts) {
            p.x = static_cast<int>(rng.uniform(0, 50));
            p.y = static_cast<int>(rng.uniform(0, 50));
            p.weight = rng.uniform(-1000, 1000);
        }
        const WeightedRangeTree<int, long long> tree(pts);
        for (int q = 0; q < 40; ++q) {
            const int x1 = static_cast<int>(rng.uniform(-3, 55));
            const int x2 = static_cast<int>(rng.uniform(x1 - 3, 55));
            const int y1 = static_cast<int>(rng.uniform(-3, 55));
            const int y2 = static_cast<int>(rng.uniform(y1 - 3, 55));
            long long expected_sum = 0;
            std::size_t expected_count = 0;
            for (const auto& p : pts) {
                if (x1 <= p.x && p.x <= x2 && y1 <= p.y && p.y <= y2) {
                    expected_sum += p.weight;
                    ++expected_count;
                }
            }
            INFO("rect=", x1, ",", x2, ",", y1, ",", y2);
            CHECK_EQ(tree.sum(x1, x2, y1, y2), expected_sum);
            CHECK_EQ(tree.count(x1, x2, y1, y2), expected_count);
        }
    }
}

PERF(3, "aggregate queries on a large log", 1500) {
    Rng rng;
    std::vector<WeightedPoint<int, long long>> pts(100'000);
    for (auto& p : pts) {
        p.x = static_cast<int>(rng.uniform(0, 1'000'000));
        p.y = static_cast<int>(rng.uniform(0, 1'000'000));
        p.weight = rng.uniform(1, 5);
    }
    const WeightedRangeTree<int, long long> tree(std::move(pts));
    long long total = 0;
    for (int q = 0; q < 100'000; ++q) {
        const int x1 = static_cast<int>(rng.uniform(0, 1'000'000));
        const int y1 = static_cast<int>(rng.uniform(0, 1'000'000));
        total += tree.sum(x1, x1 + 300'000, y1, y1 + 300'000);
    }
    CHECK_GT(total, 0);
}
