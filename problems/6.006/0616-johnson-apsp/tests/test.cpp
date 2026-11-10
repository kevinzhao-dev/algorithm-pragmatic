#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t INF = kUnreachable;

using Matrix = std::vector<std::vector<std::int64_t>>;

std::vector<Edge> random_edges(Rng& rng, int n, int m, std::int64_t low, std::int64_t high) {
    std::vector<Edge> edges;
    for (int i = 0; i < m; ++i) {
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                         static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(low, high)});
    }
    return edges;
}

// Non-negative base weights rewritten through random potentials: no negative cycles, plenty of
// negative edges, and (with zero base weights) plenty of zero-weight cycles.
std::vector<Edge> random_no_negative_cycles(Rng& rng, int n, int m, double zero_chance) {
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-40, 40);
    }
    std::vector<Edge> edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        const std::int64_t base = rng.coin(zero_chance) ? 0 : rng.uniform(1, 15);
        edges.push_back({u, v, base + potential[idx(u)] - potential[idx(v)]});
    }
    return edges;
}

// Floyd-Warshall; nullopt when any vertex lies on a negative cycle.
std::optional<Matrix> floyd(int n, const std::vector<Edge>& edges) {
    Matrix d(idx(n), std::vector<std::int64_t>(idx(n), INF));
    for (int v = 0; v < n; ++v) {
        d[idx(v)][idx(v)] = 0;
    }
    for (const Edge& e : edges) {
        d[idx(e.from)][idx(e.to)] = std::min(d[idx(e.from)][idx(e.to)], e.weight);
    }
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (d[idx(i)][idx(k)] != INF && d[idx(k)][idx(j)] != INF) {
                    d[idx(i)][idx(j)] = std::min(d[idx(i)][idx(j)], d[idx(i)][idx(k)] + d[idx(k)][idx(j)]);
                }
            }
        }
    }
    for (int v = 0; v < n; ++v) {
        if (d[idx(v)][idx(v)] < 0) {
            return std::nullopt;
        }
    }
    return d;
}

void check_potentials(const std::vector<Edge>& edges, const std::vector<std::int64_t>& h, int n) {
    REQUIRE_EQ(h.size(), n);
    for (const Edge& e : edges) {
        CHECK_GE(e.weight + h[idx(e.from)] - h[idx(e.to)], 0);
    }
}

std::map<std::pair<int, int>, std::int64_t> lightest_edges(const std::vector<Edge>& edges) {
    std::map<std::pair<int, int>, std::int64_t> lightest;
    for (const Edge& e : edges) {
        const auto [slot, inserted] = lightest.try_emplace({e.from, e.to}, e.weight);
        if (!inserted) {
            slot->second = std::min(slot->second, e.weight);
        }
    }
    return lightest;
}

// Walks hop by hop as a router would, verifying each hop is a real edge and the total matches.
void check_forwarding(int n, const std::vector<Edge>& edges, const RoutingTable& table, const Matrix& expected) {
    const auto lightest = lightest_edges(edges);
    for (int from = 0; from < n; ++from) {
        for (int to = 0; to < n; ++to) {
            INFO("from ", from, " to ", to);
            REQUIRE_EQ(table.distance(from, to), expected[idx(from)][idx(to)]);
            if (expected[idx(from)][idx(to)] == INF || from == to) {
                CHECK(!table.next_hop(from, to).has_value());
                continue;
            }
            int at = from;
            int steps = 0;
            std::int64_t total = 0;
            while (at != to) {
                const auto hop = table.next_hop(at, to);
                REQUIRE(hop.has_value());
                REQUIRE(lightest.count({at, *hop}) != 0);
                total += lightest.at({at, *hop});
                at = *hop;
                REQUIRE(++steps <= n);
            }
            CHECK_EQ(total, expected[idx(from)][idx(to)]);
            const auto route = table.route(from, to);
            REQUIRE(route.has_value());
            CHECK_EQ(route->front(), from);
            CHECK_EQ(route->back(), to);
            CHECK_EQ(route->size(), steps + 1);
        }
    }
}

}  // namespace

TEST(1, "no edges admit trivial potentials") {
    const auto h = johnson_potentials(3, {});
    REQUIRE(h.has_value());
    CHECK_EQ(h->size(), 3);
}

TEST(1, "negative edges without cycles get valid potentials") {
    const std::vector<Edge> edges = {{0, 1, -2}, {1, 2, 1}, {2, 3, -7}};
    const auto h = johnson_potentials(4, edges);
    REQUIRE(h.has_value());
    check_potentials(edges, *h, 4);
}

TEST(1, "a negative cycle gives nullopt") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 0, -2}};
    CHECK_EQ(johnson_potentials(2, edges), std::optional<std::vector<std::int64_t>>());
}

TEST(1, "a negative self-loop is a negative cycle") {
    const std::vector<Edge> edges = {{0, 0, -1}};
    CHECK_EQ(johnson_potentials(1, edges), std::optional<std::vector<std::int64_t>>());
}

TEST(1, "a negative cycle vertex 0 cannot reach still counts") {
    const std::vector<Edge> edges = {{0, 1, 1}, {2, 3, 1}, {3, 2, -2}};
    CHECK_EQ(johnson_potentials(4, edges), std::optional<std::vector<std::int64_t>>());
}

TEST(1, "a zero-weight cycle is not a negative cycle") {
    const std::vector<Edge> edges = {{0, 1, 3}, {1, 2, -5}, {2, 0, 2}};
    const auto h = johnson_potentials(3, edges);
    REQUIRE(h.has_value());
    check_potentials(edges, *h, 3);
}

TEST(1, "random graphs agree with Floyd-Warshall") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const int m = static_cast<int>(rng.uniform(0, 3 * n));
        const auto edges = rng.coin() ? random_edges(rng, n, m, -4, 8) : random_no_negative_cycles(rng, n, m, 0.3);
        INFO("round ", round, " n=", n);
        const auto h = johnson_potentials(n, edges);
        REQUIRE_EQ(h.has_value(), floyd(n, edges).has_value());
        if (h) {
            check_potentials(edges, *h, n);
        }
    }
}

// The edges are listed back to front, so every one of the V rounds is needed.
PERF(1, "potentials in O(VE) on a hostile chain", 1200) {
    const int n = 4000;
    std::vector<Edge> edges;
    for (int v = n - 1; v > 0; --v) {
        edges.push_back({v - 1, v, -1});
    }
    for (int v = 0; v + 2 < n; ++v) {
        edges.push_back({v, v + 2, 9});
    }
    const auto h = johnson_potentials(n, edges);
    REQUIRE(h.has_value());
    check_potentials(edges, *h, n);
}

TEST(2, "a single vertex is at distance zero from itself") {
    const auto d = johnson(1, {});
    REQUIRE(d.has_value());
    CHECK_EQ(*d, Matrix{{0}});
}

TEST(2, "a negative cycle gives nullopt") {
    const std::vector<Edge> edges = {{0, 1, -3}, {1, 0, 1}};
    CHECK_EQ(johnson(2, edges), std::optional<Matrix>());
}

TEST(2, "negative edges and unreachable pairs are reported correctly") {
    const std::vector<Edge> edges = {{0, 1, 3}, {1, 2, -2}, {2, 3, 2}, {0, 3, 4}, {3, 1, 1}};
    const auto d = johnson(4, edges);
    REQUIRE(d.has_value());
    CHECK_EQ(*d, (Matrix{{0, 3, 1, 3}, {INF, 0, -2, 0}, {INF, 3, 0, 2}, {INF, 1, -1, 0}}));
}

TEST(2, "unreachable entries are not shifted by the potentials") {
    const std::vector<Edge> edges = {{0, 1, -5}, {2, 3, -7}};
    const auto d = johnson(4, edges);
    REQUIRE(d.has_value());
    CHECK_EQ(*d, (Matrix{{0, -5, INF, INF}, {INF, 0, INF, INF}, {INF, INF, 0, -7}, {INF, INF, INF, 0}}));
}

TEST(2, "parallel edges use the lightest one and positive self-loops are ignored") {
    const std::vector<Edge> edges = {{0, 1, 4}, {0, 1, -1}, {0, 1, 2}, {1, 1, 5}, {0, 0, 3}};
    const auto d = johnson(2, edges);
    REQUIRE(d.has_value());
    CHECK_EQ(*d, (Matrix{{0, -1}, {INF, 0}}));
}

TEST(2, "random graphs agree with Floyd-Warshall") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const int m = static_cast<int>(rng.uniform(0, 4 * n));
        const auto edges = rng.coin(0.7) ? random_no_negative_cycles(rng, n, m, 0.3) : random_edges(rng, n, m, -3, 7);
        INFO("round ", round, " n=", n);
        REQUIRE_EQ(johnson(n, edges), floyd(n, edges));
    }
}

// A hub makes every pair reachable almost at once, so even a Floyd-Warshall that skips unreachable
// pivots does the full 4000^3 = 6.4e10 steps.
PERF(2, "sparse all-pairs beats the cubic algorithms", 3000) {
    Rng rng;
    const int n = 4000;
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-40, 40);
    }
    std::vector<Edge> edges;
    const auto add = [&](int u, int v) {
        edges.push_back({u, v, rng.uniform(1, 15) + potential[idx(u)] - potential[idx(v)]});
    };
    for (int v = 1; v < n; ++v) {
        add(0, v);
        add(v, 0);
    }
    for (int i = 0; i < n; ++i) {
        add(static_cast<int>(rng.uniform(0, n - 1)), static_cast<int>(rng.uniform(0, n - 1)));
    }
    const auto d = johnson(n, edges);
    REQUIRE(d.has_value());
    CHECK_EQ(d->size(), n);
}

// Skipping the reweighting and letting a lazy-deletion Dijkstra re-process vertices still gets the
// right answers, but on these superincreasing negative edges it re-runs the suffix after every
// gadget: 2^28 re-expansions from a single source. Reweighted, each vertex is expanded once.
PERF(2, "negative edges cannot make Dijkstra re-expand vertices", 500) {
    const int gadgets = 28;
    std::vector<Edge> edges;
    const auto a = [](int i) { return 2 * i; };
    const auto b = [](int i) { return 2 * i + 1; };
    for (int i = 0; i < gadgets; ++i) {
        const std::int64_t bonus = std::int64_t{1} << (gadgets - 1 - i);
        const std::int64_t detour = std::int64_t{4} << (gadgets - i);
        edges.push_back({a(i), a(i + 1), 0});
        edges.push_back({a(i), b(i), detour});
        edges.push_back({b(i), a(i + 1), -detour - bonus});
    }
    const int n = 2 * gadgets + 1;
    const auto d = johnson(n, edges);
    REQUIRE(d.has_value());
    CHECK_EQ((*d)[idx(a(0))][idx(a(gadgets))], -((std::int64_t{1} << gadgets) - 1));
}

TEST(3, "a negative cycle gives no table") {
    const std::vector<Edge> edges = {{0, 1, -3}, {1, 0, 1}};
    CHECK(!RoutingTable::build(2, edges).has_value());
}

TEST(3, "bad vertices throw out_of_range") {
    const auto table = RoutingTable::build(2, {});
    REQUIRE(table.has_value());
    CHECK_THROWS_AS(table->distance(2, 0), std::out_of_range);
    CHECK_THROWS_AS(table->next_hop(0, -1), std::out_of_range);
    CHECK_THROWS_AS(table->route(2, 2), std::out_of_range);
}

TEST(3, "a vertex needs no next hop to reach itself") {
    const auto table = RoutingTable::build(2, {});
    REQUIRE(table.has_value());
    CHECK_EQ(table->distance(1, 1), 0);
    CHECK_EQ(table->next_hop(1, 1), std::optional<int>());
    CHECK_EQ(table->route(1, 1), std::optional(std::vector<int>{1}));
}

TEST(3, "unreachable destinations have no route") {
    const std::vector<Edge> edges = {{0, 1, 1}};
    const auto table = RoutingTable::build(3, edges);
    REQUIRE(table.has_value());
    CHECK_EQ(table->distance(0, 2), INF);
    CHECK_EQ(table->next_hop(0, 2), std::optional<int>());
    CHECK_EQ(table->route(0, 2), std::optional<std::vector<int>>());
}

TEST(3, "next hops follow the lightest path") {
    const std::vector<Edge> edges = {{0, 1, 3}, {1, 2, -2}, {2, 3, 2}, {0, 3, 4}, {3, 1, 1}};
    const auto table = RoutingTable::build(4, edges);
    REQUIRE(table.has_value());
    CHECK_EQ(table->next_hop(0, 3), std::optional<int>(1));
    CHECK_EQ(table->route(0, 3), std::optional(std::vector<int>{0, 1, 2, 3}));
    CHECK_EQ(table->distance(0, 3), 3);
}

TEST(3, "zero-weight cycles do not trap hop-by-hop forwarding") {
    const std::vector<Edge> edges = {{0, 1, 0}, {1, 0, 0}, {0, 2, 2}, {2, 4, 3}, {1, 3, 2}, {3, 4, 3}};
    const auto table = RoutingTable::build(5, edges);
    REQUIRE(table.has_value());
    const auto expected = floyd(5, edges);
    REQUIRE(expected.has_value());
    check_forwarding(5, edges, *table, *expected);
}

TEST(3, "random graphs with many zero-weight cycles forward correctly") {
    Rng rng;
    for (int round = 0; round < 700; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const int m = static_cast<int>(rng.uniform(n / 2, 4 * n));
        const auto edges = random_no_negative_cycles(rng, n, m, 0.6);
        INFO("round ", round, " n=", n);
        const auto table = RoutingTable::build(n, edges);
        REQUIRE(table.has_value());
        const auto expected = floyd(n, edges);
        REQUIRE(expected.has_value());
        check_forwarding(n, edges, *table, *expected);
    }
}

PERF(3, "building the table stays near V Dijkstras", 2500) {
    Rng rng;
    const int n = 2000;
    const std::vector<Edge> edges = random_no_negative_cycles(rng, n, 3 * n, 0.3);
    const auto table = RoutingTable::build(n, edges);
    REQUIRE(table.has_value());
    long long reachable = 0;
    for (int to = 0; to < n; ++to) {
        reachable += table->next_hop(0, to).has_value() ? 1 : 0;
    }
    CHECK_GE(reachable, 0);
}
