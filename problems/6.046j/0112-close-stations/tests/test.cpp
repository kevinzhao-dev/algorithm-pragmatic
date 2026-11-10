#include <algorithm>
#include <array>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Stations = std::vector<Station>;

namespace {

constexpr std::int64_t kMax = 1'000'000'000;

std::int64_t sq_dist(const Station& a, const Station& b) {
    return (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y);
}

bool brute_pair(const Stations& stations, std::int64_t radius_sq) {
    for (std::size_t i = 0; i < stations.size(); ++i) {
        for (std::size_t j = i + 1; j < stations.size(); ++j) {
            if (sq_dist(stations[i], stations[j]) <= radius_sq) {
                return true;
            }
        }
    }
    return false;
}

bool brute_triple(const Stations& stations, std::int64_t radius_sq) {
    for (std::size_t i = 0; i < stations.size(); ++i) {
        for (std::size_t j = i + 1; j < stations.size(); ++j) {
            if (sq_dist(stations[i], stations[j]) > radius_sq) {
                continue;
            }
            for (std::size_t k = j + 1; k < stations.size(); ++k) {
                if (sq_dist(stations[i], stations[k]) <= radius_sq &&
                    sq_dist(stations[j], stations[k]) <= radius_sq) {
                    return true;
                }
            }
        }
    }
    return false;
}

void check_pair(const Stations& stations, std::int64_t radius_sq,
                const std::optional<std::pair<int, int>>& found) {
    CHECK_EQ(found.has_value(), brute_pair(stations, radius_sq));
    if (found) {
        const auto [a, b] = *found;
        REQUIRE(0 <= a && a < b && b < static_cast<int>(stations.size()));
        CHECK_LE(sq_dist(stations[static_cast<std::size_t>(a)], stations[static_cast<std::size_t>(b)]),
                 radius_sq);
    }
}

void check_triple(const Stations& stations, std::int64_t radius_sq,
                  const std::optional<std::array<int, 3>>& found) {
    CHECK_EQ(found.has_value(), brute_triple(stations, radius_sq));
    if (found) {
        const auto [a, b, c] = *found;
        REQUIRE(0 <= a && a < b && b < c && c < static_cast<int>(stations.size()));
        const auto& p = stations[static_cast<std::size_t>(a)];
        const auto& q = stations[static_cast<std::size_t>(b)];
        const auto& r = stations[static_cast<std::size_t>(c)];
        CHECK_LE(sq_dist(p, q), radius_sq);
        CHECK_LE(sq_dist(p, r), radius_sq);
        CHECK_LE(sq_dist(q, r), radius_sq);
    }
}

Stations random_stations(Rng& rng, std::size_t n, std::int64_t span) {
    Stations stations(n);
    for (Station& s : stations) {
        s = {rng.uniform(-span, span), rng.uniform(-span, span)};
    }
    return stations;
}

}  // namespace

TEST(1, "fewer than two stations have no close pair") {
    CHECK(!find_close_pair({}, 100).has_value());
    const Stations one{{0, 0}};
    CHECK(!find_close_pair(one, 100).has_value());
}

TEST(1, "two stations at exactly the radius are close") {
    const Stations stations{{0, 0}, {3, 4}};
    CHECK(find_close_pair(stations, 25).has_value());
    CHECK(!find_close_pair(stations, 24).has_value());
}

TEST(1, "the returned indices refer to the original order") {
    const Stations stations{{100, 100}, {0, 0}, {50, 50}, {1, 0}};
    const auto found = find_close_pair(stations, 2);
    REQUIRE(found.has_value());
    CHECK_EQ(found->first, 1);
    CHECK_EQ(found->second, 3);
}

TEST(1, "duplicates are at distance zero") {
    const Stations stations{{5, 5}, {9, 9}, {5, 5}};
    const auto found = find_close_pair(stations, 0);
    REQUIRE(found.has_value());
    CHECK_EQ(found->first, 0);
    CHECK_EQ(found->second, 2);
}

TEST(1, "a pair straddling the split is found") {
    Stations stations;
    for (std::int64_t i = 0; i < 16; ++i) {
        stations.push_back({i * 100, 0});
    }
    stations[7].x = 750;
    stations[8].x = 760;
    check_pair(stations, 10 * 10, find_close_pair(stations, 100));
}

TEST(1, "stations sharing an x coordinate are handled") {
    const Stations stations{{0, 0}, {0, 100}, {0, 199}, {0, 400}};
    CHECK(find_close_pair(stations, 99 * 99).has_value());
    CHECK(!find_close_pair(stations, 98 * 98).has_value());
}

TEST(1, "distances are compared exactly") {
    const Stations stations{{-kMax, 0}, {kMax, 1}};
    const std::int64_t exact = 2 * kMax * 2 * kMax + 1;
    CHECK(find_close_pair(stations, exact).has_value());
    CHECK(!find_close_pair(stations, exact - 1).has_value());
}

TEST(1, "the largest possible distance does not overflow") {
    const Stations stations{{-kMax, -kMax}, {kMax, kMax}};
    const std::int64_t exact = 2 * (2 * kMax) * (2 * kMax);
    CHECK(find_close_pair(stations, exact).has_value());
    CHECK(!find_close_pair(stations, exact - 1).has_value());
}

TEST(1, "random stations match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 6000; ++trial) {
        INFO("trial ", trial);
        const std::int64_t span = rng.coin() ? 6 : 60;
        const auto stations = random_stations(rng, static_cast<std::size_t>(rng.uniform(2, 40)), span);
        const std::int64_t radius_sq = rng.uniform(0, span * span / 3);
        check_pair(stations, radius_sq, find_close_pair(stations, radius_sq));
    }
}

PERF(1, "a sparse field of five hundred thousand stations", 3000) {
    Rng rng;
    Stations stations;
    for (std::int64_t x = 0; x < 700; ++x) {
        for (std::int64_t y = 0; y < 700; ++y) {
            stations.push_back({x * 1'000'000 + rng.uniform(0, 1000), y * 1'000'000 + rng.uniform(0, 1000)});
        }
    }
    rng.shuffle(stations);
    CHECK(!find_close_pair(stations, 1'000).has_value());
}

PERF(1, "a tall column keeps every station inside the strip", 2000) {
    Stations stations;
    const std::int64_t radius = 1000;
    for (std::int64_t i = 0; i < 400'000; ++i) {
        stations.push_back({i % 500, i * (radius + 1)});
    }
    CHECK(!find_close_pair(stations, radius * radius).has_value());
}

TEST(2, "fewer than three stations have no close triple") {
    CHECK(!find_close_triple({}, 100).has_value());
    const Stations two{{0, 0}, {1, 1}};
    CHECK(!find_close_triple(two, 100).has_value());
}

TEST(2, "three stations all within the radius are reported") {
    const Stations stations{{0, 0}, {3, 4}, {6, 0}};
    CHECK(find_close_triple(stations, 36).has_value());
    CHECK(!find_close_triple(stations, 35).has_value());
}

TEST(2, "two close pairs do not make a triple") {
    const Stations stations{{0, 0}, {1, 0}, {100, 0}, {101, 0}};
    CHECK(!find_close_triple(stations, 4).has_value());
}

TEST(2, "a chain of neighbors does not make a triple") {
    const Stations stations{{0, 0}, {5, 0}, {10, 0}};
    CHECK(!find_close_triple(stations, 25).has_value());
    CHECK(find_close_triple(stations, 100).has_value());
}

TEST(2, "a triple straddling the split is found") {
    Stations stations;
    for (std::int64_t i = 0; i < 16; ++i) {
        stations.push_back({i * 100, 0});
    }
    stations[8] = {700, 0};
    stations[7] = {701, 0};
    stations[9] = {702, 0};
    const auto found = find_close_triple(stations, 25);
    REQUIRE(found.has_value());
    CHECK_EQ((*found), (std::array<int, 3>{7, 8, 9}));
}

TEST(2, "duplicates form a triple") {
    const Stations stations{{2, 2}, {2, 2}, {2, 2}};
    CHECK(find_close_triple(stations, 0).has_value());
    const Stations two{{2, 2}, {2, 2}, {3, 3}};
    CHECK(!find_close_triple(two, 0).has_value());
}

TEST(2, "a triple surrounded by close pairs is found") {
    Stations stations;
    const std::int64_t radius = 100;
    for (std::int64_t i = 0; i < 40; ++i) {
        stations.push_back({-1, i * (radius + 1)});
        stations.push_back({1, i * (radius + 1) + 1});
    }
    stations.push_back({0, 20 * (radius + 1) + 2});
    CHECK(find_close_triple(stations, radius * radius).has_value());
}

TEST(2, "random stations match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 6000; ++trial) {
        INFO("trial ", trial);
        const std::int64_t span = rng.coin() ? 6 : 60;
        const auto stations = random_stations(rng, static_cast<std::size_t>(rng.uniform(3, 30)), span);
        const std::int64_t radius_sq = rng.uniform(0, span * span / 2);
        check_triple(stations, radius_sq, find_close_triple(stations, radius_sq));
    }
}

TEST(2, "many close pairs but no triple") {
    Stations stations;
    const std::int64_t radius = 10;
    for (std::int64_t i = 0; i < 2000; ++i) {
        stations.push_back({0, i * 200});
        stations.push_back({0, i * 200 + radius});
    }
    CHECK(!find_close_triple(stations, radius * radius).has_value());
}

PERF(2, "a dense field of near-duplicates holds no triple", 3000) {
    Rng rng;
    Stations stations;
    for (std::int64_t x = 0; x < 500; ++x) {
        for (std::int64_t y = 0; y < 500; ++y) {
            const Station base{x * 1'000'000, y * 1'000'000};
            stations.push_back(base);
            stations.push_back({base.x + rng.uniform(0, 20), base.y + rng.uniform(0, 20)});
        }
    }
    rng.shuffle(stations);
    CHECK(!find_close_triple(stations, 5'000).has_value());
}
