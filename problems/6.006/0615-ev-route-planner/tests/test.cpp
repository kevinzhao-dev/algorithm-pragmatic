#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t kNever = std::numeric_limits<std::int64_t>::max();

RoadMap flat_map(int n, std::vector<Road> roads, int capacity) {
    return RoadMap{std::vector<int>(idx(n), 5), std::vector<char>(idx(n), 0), std::move(roads), capacity, 1};
}

// Depth-first simulation of the rules; a state is only re-expanded when reached strictly sooner.
void brute_visit(const RoadMap& map, int at, int level, std::int64_t minutes, int target,
                 const std::vector<std::vector<int>>& adjacent,
                 std::vector<std::vector<std::int64_t>>& best_at, std::int64_t& best) {
    if (minutes >= best_at[idx(at)][idx(level)] || minutes >= best) {
        return;
    }
    best_at[idx(at)][idx(level)] = minutes;
    if (at == target) {
        best = minutes;
        return;
    }
    if (map.has_charger[idx(at)] != 0 && level < map.capacity) {
        brute_visit(map, at, level + 1, minutes + map.minutes_per_unit, target, adjacent, best_at, best);
    }
    for (const int road_id : adjacent[idx(at)]) {
        const Road& road = map.roads[idx(road_id)];
        const int to = road.a == at ? road.b : road.a;
        const int used = std::max(0, map.elevation[idx(to)] - map.elevation[idx(at)]);
        if (level - used >= 1) {
            brute_visit(map, to, level - used, minutes + road.minutes, target, adjacent, best_at, best);
        }
    }
}

std::optional<std::int64_t> brute_fastest(const RoadMap& map, int source, int target) {
    const int n = static_cast<int>(map.elevation.size());
    std::vector<std::vector<int>> adjacent(idx(n));
    for (int i = 0; i < static_cast<int>(map.roads.size()); ++i) {
        adjacent[idx(map.roads[idx(i)].a)].push_back(i);
        adjacent[idx(map.roads[idx(i)].b)].push_back(i);
    }
    std::vector<std::vector<std::int64_t>> best_at(idx(n), std::vector<std::int64_t>(idx(map.capacity) + 1, kNever));
    std::int64_t best = kNever;
    brute_visit(map, source, map.capacity, 0, target, adjacent, best_at, best);
    if (best == kNever) {
        return std::nullopt;
    }
    return best;
}

RoadMap random_map(Rng& rng, int n, int roads, int capacity, double charger_chance, int max_elevation = 9) {
    RoadMap map;
    map.capacity = capacity;
    map.minutes_per_unit = rng.uniform(1, 5);
    map.elevation.resize(idx(n));
    map.has_charger.resize(idx(n));
    for (int v = 0; v < n; ++v) {
        map.elevation[idx(v)] = static_cast<int>(rng.uniform(1, max_elevation));
        map.has_charger[idx(v)] = rng.coin(charger_chance) ? 1 : 0;
    }
    for (int i = 0; i < roads; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a != b) {
            map.roads.push_back({a, b, rng.uniform(1, 9)});
        }
    }
    return map;
}

// Executes a plan under the rules and returns the time it really takes.
std::int64_t simulate(const RoadMap& map, int source, int target, const RoutePlan& plan) {
    REQUIRE(!plan.stops.empty());
    CHECK_EQ(plan.stops.front().location, source);
    CHECK_EQ(plan.stops.back().location, target);
    std::int64_t minutes = 0;
    int level = map.capacity;
    for (std::size_t i = 0; i < plan.stops.size(); ++i) {
        const Stop& stop = plan.stops[i];
        INFO("stop ", i, " at ", stop.location);
        REQUIRE(stop.location >= 0 && stop.location < static_cast<int>(map.elevation.size()));
        REQUIRE(stop.charge_units >= 0);
        if (stop.charge_units > 0) {
            REQUIRE(map.has_charger[idx(stop.location)] != 0);
            level += stop.charge_units;
            REQUIRE(level <= map.capacity);
            minutes += stop.charge_units * map.minutes_per_unit;
        }
        if (i + 1 == plan.stops.size()) {
            break;
        }
        const int next = plan.stops[i + 1].location;
        std::int64_t cheapest = kNever;
        for (const Road& road : map.roads) {
            if ((road.a == stop.location && road.b == next) || (road.b == stop.location && road.a == next)) {
                cheapest = std::min(cheapest, road.minutes);
            }
        }
        REQUIRE(cheapest != kNever);
        minutes += cheapest;
        level -= std::max(0, map.elevation[idx(next)] - map.elevation[idx(stop.location)]);
        REQUIRE(level >= 1);
    }
    return minutes;
}

}  // namespace

TEST(1, "start equals end takes no time") {
    const RoadMap map = flat_map(2, {}, 1);
    CHECK_EQ(fastest_time(map, 1, 1), std::optional<std::int64_t>(0));
}

TEST(1, "disconnected intersections are unreachable") {
    const RoadMap map = flat_map(2, {}, 1);
    CHECK_EQ(fastest_time(map, 0, 1), std::optional<std::int64_t>());
}

TEST(1, "bad intersections throw out_of_range") {
    const RoadMap map = flat_map(2, {}, 1);
    CHECK_THROWS_AS(fastest_time(map, 2, 0), std::out_of_range);
    CHECK_THROWS_AS(fastest_time(map, 0, -1), std::out_of_range);
}

TEST(1, "flat roads use no charge, even with a one-unit battery") {
    const RoadMap map = flat_map(3, {{0, 1, 4}, {1, 2, 3}}, 1);
    CHECK_EQ(fastest_time(map, 0, 2), std::optional<std::int64_t>(7));
}

TEST(1, "the charge must stay strictly positive on arrival") {
    // Climbing 2 with 3 units leaves 1, which is allowed; climbing 3 would leave 0.
    const RoadMap possible{{1, 3}, {0, 0}, {{0, 1, 5}}, 3, 1};
    CHECK_EQ(fastest_time(possible, 0, 1), std::optional<std::int64_t>(5));
    const RoadMap impossible{{1, 4}, {0, 0}, {{0, 1, 5}}, 3, 1};
    CHECK_EQ(fastest_time(impossible, 0, 1), std::optional<std::int64_t>());
}

TEST(1, "downhill costs no charge, uphill does") {
    const RoadMap map{{9, 1}, {0, 0}, {{0, 1, 2}}, 1, 1};
    CHECK_EQ(fastest_time(map, 0, 1), std::optional<std::int64_t>(2));
    CHECK_EQ(fastest_time(map, 1, 0), std::optional<std::int64_t>());
}

TEST(1, "the fastest road can strand the car, so a slower one wins") {
    // The quick route climbs 3 over a mountain (1), then has to climb 1 more out of a valley (2),
    // which would leave the battery at 0. The slow route (via 4) climbs only 2 in total.
    const RoadMap map{{1, 4, 2, 3, 2},
                      {0, 0, 0, 0, 0},
                      {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}, {0, 4, 5}, {4, 3, 5}},
                      4,
                      1};
    CHECK_EQ(fastest_time(map, 0, 3), std::optional<std::int64_t>(10));
}

TEST(1, "random maps without chargers match exhaustive search") {
    Rng rng;
    int reached = 0;
    for (int round = 0; round < 1200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        const RoadMap map = random_map(rng, n, static_cast<int>(rng.uniform(0, 2 * n)),
                                       static_cast<int>(rng.uniform(1, 8)), 0.0);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s, " t=", t);
        const auto expected = brute_fastest(map, s, t);
        REQUIRE_EQ(fastest_time(map, s, t), expected);
        reached += expected.has_value() ? 1 : 0;
    }
    CHECK_GT(reached, 200);
}

PERF(1, "plain Dijkstra over charge levels at scale", 700) {
    Rng rng;
    const int n = 800;
    const int capacity = 700;
    RoadMap map;
    map.capacity = capacity;
    map.minutes_per_unit = 1;
    map.has_charger.assign(idx(n), 0);
    map.elevation.resize(idx(n));
    for (int v = 0; v < n; ++v) {
        map.elevation[idx(v)] = 1 + v % 5;
    }
    for (int v = 0; v + 1 < n; ++v) {
        map.roads.push_back({v, v + 1, rng.uniform(1, 50)});
    }
    for (int i = 0; i < 1500; ++i) {
        const int v = static_cast<int>(rng.uniform(0, n - 2));
        const int w = std::min(n - 1, v + static_cast<int>(rng.uniform(2, 20)));
        map.roads.push_back({v, w, rng.uniform(1, 200)});
    }
    const auto time = fastest_time(map, 0, n - 1);
    CHECK(time.has_value());
}

TEST(2, "charging is a cost and only happens when needed") {
    // The climb to 2 needs 3 units; the car arrives at 1 with 1 and must add 2 units at 2 min each.
    const RoadMap map{{1, 3, 5}, {0, 1, 0}, {{0, 1, 1}, {1, 2, 1}}, 3, 2};
    CHECK_EQ(fastest_time(map, 0, 2), std::optional<std::int64_t>(6));
}

TEST(2, "a climb larger than the battery is impossible even with chargers") {
    const RoadMap map{{1, 4}, {1, 1}, {{0, 1, 2}}, 2, 3};
    CHECK_EQ(fastest_time(map, 0, 1), std::optional<std::int64_t>());
}

TEST(2, "a detour to a charger can be the fastest legal route") {
    // Direct 0 -> 2 climbs 4, which a 4-unit battery cannot do. Via 1: arrive with 3, add 1 unit,
    // then climb 3 and arrive with 1.
    const RoadMap map{{1, 2, 5}, {0, 1, 0}, {{0, 2, 1}, {0, 1, 2}, {1, 2, 3}}, 4, 1};
    CHECK_EQ(fastest_time(map, 0, 2), std::optional<std::int64_t>(6));
}

TEST(2, "a small battery can only top up so far") {
    const RoadMap blocked{{1, 2, 3, 4}, {0, 1, 0, 0}, {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}}, 2, 10};
    CHECK_EQ(fastest_time(blocked, 0, 3), std::optional<std::int64_t>());
    const RoadMap reachable{{1, 2, 3}, {0, 1, 0}, {{0, 1, 1}, {1, 2, 1}}, 2, 10};
    CHECK_EQ(fastest_time(reachable, 0, 2), std::optional<std::int64_t>(12));
}

TEST(2, "charging can buy a shorter route") {
    // 0 -> 1 -> 2 is short but needs a top-up; the long flat road never needs charge.
    const RoadMap map{{1, 3, 5, 1},
                      {0, 1, 0, 0},
                      {{0, 1, 1}, {1, 2, 1}, {0, 3, 1}, {3, 2, 30}},
                      3,
                      2};
    CHECK_EQ(fastest_time(map, 0, 2), std::optional<std::int64_t>(6));
}

TEST(2, "random maps with chargers match exhaustive search") {
    Rng rng;
    int reached = 0;
    for (int round = 0; round < 2000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        const RoadMap map = random_map(rng, n, static_cast<int>(rng.uniform(0, 2 * n)),
                                       static_cast<int>(rng.uniform(1, 5)), 0.6, 5);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s, " t=", t);
        const auto expected = brute_fastest(map, s, t);
        REQUIRE_EQ(fastest_time(map, s, t), expected);
        reached += expected.has_value() ? 1 : 0;
    }
    CHECK_GT(reached, 400);
}

// Writing charging as one edge per amount adds about capacity^2/2 relaxations per charger.
// The target sits behind an impossible climb, so every reachable (location, level) gets explored.
PERF(2, "charging one unit at a time keeps a large battery tractable", 800) {
    Rng rng;
    const int n = 100;
    const int capacity = 15'000;
    RoadMap map = random_map(rng, n, 0, capacity, 1.0);
    map.minutes_per_unit = 1;
    for (int v = 0; v < n; ++v) {
        map.elevation[idx(v)] = static_cast<int>(rng.uniform(1, capacity - 1));
    }
    map.elevation[idx(n - 1)] = 3 * capacity;
    for (int v = 0; v + 1 < n; ++v) {
        map.roads.push_back({v, v + 1, rng.uniform(1, 400)});
    }
    for (int i = 0; i < 140; ++i) {
        const int v = static_cast<int>(rng.uniform(0, n - 2));
        const int w = std::min(n - 1, v + static_cast<int>(rng.uniform(2, 20)));
        map.roads.push_back({v, w, rng.uniform(1, 400)});
    }
    CHECK(!fastest_time(map, 0, n - 1).has_value());
}

TEST(3, "no route gives nullopt") {
    const RoadMap map = flat_map(2, {}, 2);
    CHECK(!fastest_plan(map, 0, 1).has_value());
}

TEST(3, "start equals end is a single stop") {
    const RoadMap map = flat_map(2, {}, 2);
    const auto plan = fastest_plan(map, 1, 1);
    REQUIRE(plan.has_value());
    CHECK_EQ(plan->total_minutes, 0);
    REQUIRE_EQ(plan->stops.size(), 1);
    CHECK_EQ(plan->stops[0].location, 1);
    CHECK_EQ(plan->stops[0].charge_units, 0);
}

TEST(3, "a plan reports how much was charged at each stop") {
    const RoadMap map{{1, 3, 5}, {0, 1, 0}, {{0, 1, 1}, {1, 2, 1}}, 3, 2};
    const auto plan = fastest_plan(map, 0, 2);
    REQUIRE(plan.has_value());
    CHECK_EQ(plan->total_minutes, 6);
    REQUIRE_EQ(plan->stops.size(), 3);
    CHECK_EQ(plan->stops[1].charge_units, 2);
    CHECK_EQ(simulate(map, 0, 2, *plan), 6);
}

TEST(3, "random maps produce plans that actually run") {
    Rng rng;
    int charged = 0;
    for (int round = 0; round < 2000; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 8));
        // A line of steep segments: chargers on the way are usually needed to get across.
        RoadMap map = random_map(rng, n, 0, static_cast<int>(rng.uniform(2, 6)), 0.7,
                                 static_cast<int>(rng.uniform(3, 9)));
        for (int v = 0; v + 1 < n; ++v) {
            map.roads.push_back({v, v + 1, rng.uniform(1, 9)});
        }
        const int extra = static_cast<int>(rng.uniform(0, n));
        for (int e = 0; e < extra; ++e) {
            const int a = static_cast<int>(rng.uniform(0, n - 1));
            const int b = static_cast<int>(rng.uniform(0, n - 1));
            if (a != b) {
                map.roads.push_back({a, b, rng.uniform(1, 9)});
            }
        }
        const int s = 0;
        const int t = n - 1;
        INFO("round ", round, " n=", n);
        const auto expected = brute_fastest(map, s, t);
        const auto plan = fastest_plan(map, s, t);
        REQUIRE_EQ(plan.has_value(), expected.has_value());
        if (plan) {
            CHECK_EQ(plan->total_minutes, *expected);
            CHECK_EQ(simulate(map, s, t, *plan), *expected);
            for (const Stop& stop : plan->stops) {
                charged += stop.charge_units > 0 ? 1 : 0;
            }
        }
    }
    CHECK_GT(charged, 100);
}

PERF(3, "plan extraction on a large instance", 1500) {
    Rng rng;
    const int n = 500;
    const int capacity = 3000;
    RoadMap map = random_map(rng, n, 0, capacity, 1.0);
    map.minutes_per_unit = 1;
    for (int v = 0; v < n; ++v) {
        map.elevation[idx(v)] = static_cast<int>(rng.uniform(1, 2500));
    }
    for (int v = 0; v + 1 < n; ++v) {
        map.roads.push_back({v, v + 1, rng.uniform(1, 400)});
    }
    const auto plan = fastest_plan(map, 0, n - 1);
    REQUIRE(plan.has_value());
    CHECK_EQ(simulate(map, 0, n - 1, *plan), plan->total_minutes);
}
