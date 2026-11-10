#include <algorithm>
#include <cstddef>
#include <optional>
#include <queue>
#include <set>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr int A = kZoneA;
constexpr int B = kZoneB;

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

bool same_tenant(int x, int y) { return x >= 0 && x == y; }

// Spells out the rule from the statement: which tiles sit on either side of this one segment.
bool segment_allowed(const TileMap& tiles, Corner from, Corner to) {
    const int n = static_cast<int>(tiles.size());
    const auto [r1, c1] = std::min(from, to);
    const auto [r2, c2] = std::max(from, to);
    if (r1 == r2) {
        if (r1 == 0 || r1 == n) {
            return true;
        }
        return !same_tenant(tiles[idx(r1 - 1)][idx(c1)], tiles[idx(r1)][idx(c1)]);
    }
    if (c1 == 0 || c1 == n) {
        return true;
    }
    return !same_tenant(tiles[idx(r1)][idx(c1 - 1)], tiles[idx(r1)][idx(c1)]);
}

std::vector<Corner> corners_touching(const TileMap& tiles, int zone) {
    const int n = static_cast<int>(tiles.size());
    std::set<Corner> corners;
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (tiles[idx(r)][idx(c)] == zone) {
                corners.insert({{r, c}, {r + 1, c}, {r, c + 1}, {r + 1, c + 1}});
            }
        }
    }
    return {corners.begin(), corners.end()};
}

std::vector<Corner> neighbors_of(const TileMap& tiles, Corner at) {
    const int n = static_cast<int>(tiles.size());
    std::vector<Corner> out;
    for (const auto& [dr, dc] : std::vector<Corner>{{0, 1}, {0, -1}, {1, 0}, {-1, 0}}) {
        const Corner next{at.first + dr, at.second + dc};
        if (next.first >= 0 && next.first <= n && next.second >= 0 && next.second <= n &&
            segment_allowed(tiles, at, next)) {
            out.push_back(next);
        }
    }
    return out;
}

// A separate BFS from each A-corner; quartic overall, which is fine for tiny grids.
std::optional<int> brute_length(const TileMap& tiles) {
    const auto sources = corners_touching(tiles, A);
    const auto targets = corners_touching(tiles, B);
    const std::set<Corner> target_set(targets.begin(), targets.end());
    if (target_set.empty()) {
        return std::nullopt;
    }
    std::optional<int> best;
    for (const Corner source : sources) {
        std::vector<std::pair<Corner, int>> frontier{{source, 0}};
        std::set<Corner> seen{source};
        for (std::size_t head = 0; head < frontier.size(); ++head) {
            const auto [at, d] = frontier[head];
            if (target_set.count(at) != 0) {
                if (!best || d < *best) {
                    best = d;
                }
                break;
            }
            for (const Corner next : neighbors_of(tiles, at)) {
                if (seen.insert(next).second) {
                    frontier.emplace_back(next, d + 1);
                }
            }
        }
    }
    return best;
}

void enumerate_paths(const TileMap& tiles, Corner at, int length, std::set<Corner>& on_path,
                     const std::set<Corner>& targets, std::optional<int>& best) {
    if (targets.count(at) != 0 && (!best || length < *best)) {
        best = length;
    }
    if (best && length >= *best) {
        return;
    }
    for (const Corner next : neighbors_of(tiles, at)) {
        if (on_path.insert(next).second) {
            enumerate_paths(tiles, next, length + 1, on_path, targets, best);
            on_path.erase(next);
        }
    }
}

// Enumerates every simple path along the grid lines; only feasible on tiny boards.
std::optional<int> exhaustive_length(const TileMap& tiles) {
    const auto sources = corners_touching(tiles, A);
    const auto target_list = corners_touching(tiles, B);
    const std::set<Corner> targets(target_list.begin(), target_list.end());
    std::optional<int> best;
    if (targets.empty()) {
        return best;
    }
    for (const Corner source : sources) {
        std::set<Corner> on_path{source};
        enumerate_paths(tiles, source, 0, on_path, targets, best);
    }
    return best;
}

TileMap random_tiles(Rng& rng, int n, int tenants, double zone_chance) {
    TileMap tiles(idx(n), std::vector<int>(idx(n)));
    for (auto& row : tiles) {
        for (int& cell : row) {
            if (rng.coin(zone_chance)) {
                cell = rng.coin() ? A : B;
            } else {
                cell = static_cast<int>(rng.uniform(0, tenants - 1));
            }
        }
    }
    return tiles;
}

void check_route(const TileMap& tiles, const std::vector<Corner>& route, int expected_length) {
    REQUIRE_EQ(route.size(), expected_length + 1);
    const auto sources = corners_touching(tiles, A);
    const auto targets = corners_touching(tiles, B);
    CHECK(std::find(sources.begin(), sources.end(), route.front()) != sources.end());
    CHECK(std::find(targets.begin(), targets.end(), route.back()) != targets.end());
    const int n = static_cast<int>(tiles.size());
    for (std::size_t i = 0; i < route.size(); ++i) {
        REQUIRE(route[i].first >= 0 && route[i].first <= n);
        REQUIRE(route[i].second >= 0 && route[i].second <= n);
        if (i + 1 < route.size()) {
            const int gap = std::abs(route[i].first - route[i + 1].first) +
                            std::abs(route[i].second - route[i + 1].second);
            REQUIRE_EQ(gap, 1);
            REQUIRE(segment_allowed(tiles, route[i], route[i + 1]));
        }
    }
}

}  // namespace

TEST(1, "a map without one of the zones has no route") {
    const TileMap only_a = {{A, 0}, {0, 0}};
    CHECK_EQ(shortest_tray_length(only_a), std::optional<int>());
    const TileMap only_b = {{B, 1}, {1, 2}};
    CHECK_EQ(shortest_tray_length(only_b), std::optional<int>());
}

TEST(1, "zones that share a corner need no tray at all") {
    const TileMap side_by_side = {{A, B}, {0, 0}};
    CHECK_EQ(shortest_tray_length(side_by_side), std::optional<int>(0));
    const TileMap diagonal = {{A, 7}, {8, B}};
    CHECK_EQ(shortest_tray_length(diagonal), std::optional<int>(0));
}

TEST(1, "a one-tile gap on the outer boundary is one segment") {
    const TileMap tiles = {{A, 7, B}, {7, 7, 7}, {7, 7, 7}};
    CHECK_EQ(shortest_tray_length(tiles), std::optional<int>(1));
}

TEST(1, "borders inside one tenant force a detour around the wall") {
    // Everything between the two zones is one tenant, so the cable must follow the outer boundary.
    const TileMap tiles = {{7, A, 7}, {7, 7, 7}, {7, B, 7}};
    CHECK_EQ(shortest_tray_length(tiles), std::optional<int>(5));
}

TEST(1, "a border between two different tenants is usable") {
    const TileMap tiles = {{7, A, 7}, {7, 8, 7}, {7, B, 7}};
    CHECK_EQ(shortest_tray_length(tiles), std::optional<int>(1));
}

TEST(1, "a zone walled in by a single tenant is unreachable") {
    const TileMap tiles = {{7, 7, 7, 7}, {7, A, 7, 7}, {7, 7, 7, 7}, {7, 7, 7, B}};
    CHECK_EQ(shortest_tray_length(tiles), std::optional<int>());
}

TEST(1, "every corner of every zone tile is a valid endpoint") {
    const TileMap tiles = {{A, A, 3}, {3, 3, 3}, {3, 3, B}};
    CHECK_EQ(shortest_tray_length(tiles), std::optional<int>(3));
}

TEST(1, "tiny boards match enumeration of every simple path") {
    Rng rng;
    int solved = 0;
    for (int round = 0; round < 250; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 3));
        const TileMap tiles = random_tiles(rng, n, 2, 0.3);
        INFO("round ", round, " tiles=", tiles);
        const auto expected = exhaustive_length(tiles);
        REQUIRE_EQ(shortest_tray_length(tiles), expected);
        solved += expected.has_value() ? 1 : 0;
    }
    CHECK_GT(solved, 30);
}

TEST(1, "random boards match one BFS per source corner") {
    Rng rng;
    for (int round = 0; round < 250; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const TileMap tiles = random_tiles(rng, n, static_cast<int>(rng.uniform(1, 4)),
                                           rng.real() * 0.25 + 0.03);
        INFO("round ", round, " tiles=", tiles);
        REQUIRE_EQ(shortest_tray_length(tiles), brute_length(tiles));
    }
}

// A separate BFS from each of the ~4,000 A-corners visits four million corners every time.
PERF(1, "many sources are handled by a single traversal", 2500) {
    Rng rng;
    const int n = 2000;
    TileMap tiles(idx(n), std::vector<int>(idx(n), 5));
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            tiles[idx(r)][idx(c)] = static_cast<int>(rng.uniform(0, 1'000'000));
        }
    }
    for (int r = 0; r < n; ++r) {
        tiles[idx(r)][0] = A;
    }
    tiles[idx(n - 1)][idx(n - 1)] = B;
    const auto length = shortest_tray_length(tiles);
    CHECK_EQ(length, std::optional<int>(n - 2));
}

TEST(2, "no route is reported the same way as stage 1") {
    const TileMap tiles = {{A, 7}, {7, 7}};
    CHECK_EQ(shortest_tray_route(tiles), std::optional<std::vector<Corner>>());
}

TEST(2, "a zero-length route is a single shared corner") {
    const TileMap tiles = {{A, B}, {7, 7}};
    const auto route = shortest_tray_route(tiles);
    REQUIRE(route.has_value());
    check_route(tiles, *route, 0);
}

TEST(2, "a detour around a wall is returned segment by segment") {
    const TileMap tiles = {{7, A, 7}, {7, 7, 7}, {7, B, 7}};
    const auto route = shortest_tray_route(tiles);
    REQUIRE(route.has_value());
    check_route(tiles, *route, 5);
}

TEST(2, "random boards return valid shortest routes") {
    Rng rng;
    for (int round = 0; round < 600; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const TileMap tiles = random_tiles(rng, n, static_cast<int>(rng.uniform(1, 4)),
                                           rng.real() * 0.25 + 0.03);
        INFO("round ", round, " tiles=", tiles);
        const auto expected = brute_length(tiles);
        const auto route = shortest_tray_route(tiles);
        REQUIRE_EQ(route.has_value(), expected.has_value());
        if (route) {
            check_route(tiles, *route, *expected);
        }
    }
}

PERF(2, "route extraction stays linear in the grid", 2500) {
    Rng rng;
    const int n = 2000;
    TileMap tiles(idx(n), std::vector<int>(idx(n)));
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            tiles[idx(r)][idx(c)] = static_cast<int>(rng.uniform(0, 1'000'000));
        }
    }
    tiles[0][0] = A;
    tiles[idx(n - 1)][idx(n - 1)] = B;
    const auto route = shortest_tray_route(tiles);
    REQUIRE(route.has_value());
    CHECK_EQ(route->size(), 2 * (n - 2) + 1);
}
