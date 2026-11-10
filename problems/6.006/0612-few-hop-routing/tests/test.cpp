#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t INF = kUnreachable;

std::vector<Edge> random_edges(Rng& rng, int n, int m, std::int64_t low, std::int64_t high) {
    std::vector<Edge> edges;
    for (int i = 0; i < m; ++i) {
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                         static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(low, high)});
    }
    return edges;
}

// Weights produced by adding potential differences to non-negative base weights: no negative
// cycle can exist, but negative edges do.
std::vector<Edge> random_no_negative_cycles(Rng& rng, int n, int m) {
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-25, 25);
    }
    std::vector<Edge> edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        edges.push_back({u, v, rng.uniform(0, 15) + potential[idx(u)] - potential[idx(v)]});
    }
    return edges;
}

// Exact dynamic program over the number of edges: table[h][v] is the lightest walk using <= h edges.
std::vector<std::vector<std::int64_t>> hop_table(int n, const std::vector<Edge>& edges, int source,
                                                 int max_hops) {
    std::vector<std::vector<std::int64_t>> table(idx(max_hops) + 1,
                                                 std::vector<std::int64_t>(idx(n), INF));
    table[0][idx(source)] = 0;
    for (int h = 1; h <= max_hops; ++h) {
        table[idx(h)] = table[idx(h - 1)];
        for (const Edge& e : edges) {
            if (table[idx(h - 1)][idx(e.from)] != INF) {
                table[idx(h)][idx(e.to)] = std::min(table[idx(h)][idx(e.to)],
                                                    table[idx(h - 1)][idx(e.from)] + e.weight);
            }
        }
    }
    return table;
}

int brute_convergence(int n, const std::vector<Edge>& edges, int source) {
    const auto table = hop_table(n, edges, source, n);
    int needed = 0;
    for (int v = 0; v < n; ++v) {
        if (table[idx(n)][idx(v)] == INF) {
            continue;
        }
        int h = 0;
        while (table[idx(h)][idx(v)] != table[idx(n)][idx(v)]) {
            ++h;
        }
        needed = std::max(needed, h);
    }
    return needed;
}

// All vertices sit in `layers` layers behind a single source; only forward edges are ever on a
// shortest path because the back edges are enormously heavy, and potentials add negative edges.
std::vector<Edge> layered_network(Rng& rng, int n, int layers, int extra_forward, int back_edges) {
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-1'000'000, 1'000'000);
    }
    const auto layer_of = [&](int v) { return v == 0 ? 0 : 1 + (v - 1) % layers; };
    std::vector<std::vector<int>> members(idx(layers) + 1);
    members[0].push_back(0);
    for (int v = 1; v < n; ++v) {
        members[idx(layer_of(v))].push_back(v);
    }
    const auto pick = [&](int layer) {
        const auto& group = members[idx(layer)];
        return group[idx(static_cast<int>(rng.uniform(0, static_cast<std::int64_t>(group.size()) - 1)))];
    };
    std::vector<Edge> edges;
    edges.reserve(idx(n + extra_forward + back_edges));
    const auto add = [&](int u, int v, std::int64_t base) {
        edges.push_back({u, v, base + potential[idx(u)] - potential[idx(v)]});
    };
    for (int v = 1; v < n; ++v) {
        add(pick(layer_of(v) - 1), v, rng.uniform(0, 9));
    }
    for (int i = 0; i < extra_forward; ++i) {
        const int layer = static_cast<int>(rng.uniform(1, layers));
        add(pick(layer - 1), pick(layer), rng.uniform(0, 9));
    }
    for (int i = 0; i < back_edges; ++i) {
        const int to = static_cast<int>(rng.uniform(1, layers));
        const int from = static_cast<int>(rng.uniform(to, layers));
        add(pick(from), pick(to), 1'000'000'000);
    }
    rng.shuffle(edges);
    return edges;
}

}  // namespace

TEST(1, "a lone source") {
    CHECK_EQ(converge_routes(1, {}, 0), std::vector<std::int64_t>{0});
}

TEST(1, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(converge_routes(2, {}, 2), std::out_of_range);
    CHECK_THROWS_AS(converge_routes(2, {}, -1), std::out_of_range);
}

TEST(1, "unreachable vertices stay unreachable") {
    const std::vector<Edge> edges = {{0, 1, 5}, {2, 3, -5}};
    CHECK_EQ(converge_routes(4, edges, 0), std::vector<std::int64_t>{0, 5, INF, INF});
}

TEST(1, "negative edges and detours are found") {
    const std::vector<Edge> edges = {{0, 1, 4}, {0, 2, 1}, {2, 1, -2}, {1, 3, 1}};
    CHECK_EQ(converge_routes(4, edges, 0), std::vector<std::int64_t>{0, -1, 1, 0});
}

TEST(1, "the answer does not depend on edge order") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 14));
        std::vector<Edge> edges = random_no_negative_cycles(rng, n, static_cast<int>(rng.uniform(0, 3 * n)));
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const auto expected = hop_table(n, edges, s, n)[idx(n)];
        INFO("round ", round);
        REQUIRE_EQ(converge_routes(n, edges, s), expected);
        rng.shuffle(edges);
        REQUIRE_EQ(converge_routes(n, edges, s), expected);
    }
}

// Round-based relaxation with no early exit would run close to a million rounds here.
PERF(1, "a huge network converges in a couple dozen rounds", 2500) {
    Rng rng;
    const int n = 800'000;
    const std::vector<Edge> edges = layered_network(rng, n, 20, 1'500'000, 400'000);
    const auto dist = converge_routes(n, edges, 0);
    CHECK_NE(dist[idx(n - 1)], INF);
}

TEST(2, "zero hops reach only the source") {
    const std::vector<Edge> edges = {{0, 1, 1}};
    CHECK_EQ(hop_limited_routes(2, edges, 0, 0), std::vector<std::int64_t>{0, INF});
}

TEST(2, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(hop_limited_routes(2, {}, 2, 1), std::out_of_range);
}

TEST(2, "one hop forbids a detour that needs two") {
    const std::vector<Edge> edges = {{0, 2, 1}, {2, 1, 1}, {0, 1, 5}};
    CHECK_EQ(hop_limited_routes(3, edges, 0, 1), std::vector<std::int64_t>{0, 5, 1});
    CHECK_EQ(hop_limited_routes(3, edges, 0, 2), std::vector<std::int64_t>{0, 2, 1});
}

TEST(2, "one round must never chain two new edges") {
    // In-place relaxation in this edge order would reach vertex 2 during the first round.
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}};
    CHECK_EQ(hop_limited_routes(4, edges, 0, 1), std::vector<std::int64_t>{0, 1, INF, INF});
    CHECK_EQ(hop_limited_routes(4, edges, 0, 2), std::vector<std::int64_t>{0, 1, 2, INF});
}

TEST(2, "negative cycles are fine under a hop budget") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 0, -5}};
    CHECK_EQ(hop_limited_routes(2, edges, 0, 1), std::vector<std::int64_t>{0, 1});
    CHECK_EQ(hop_limited_routes(2, edges, 0, 2), std::vector<std::int64_t>{-4, 1});
    CHECK_EQ(hop_limited_routes(2, edges, 0, 3), std::vector<std::int64_t>{-4, -3});
}

TEST(2, "a large hop budget matches unlimited routing when there is no negative cycle") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        const auto edges = random_no_negative_cycles(rng, n, static_cast<int>(rng.uniform(0, 3 * n)));
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round);
        REQUIRE_EQ(hop_limited_routes(n, edges, s, n), hop_table(n, edges, s, n)[idx(n)]);
    }
}

TEST(2, "random graphs match the exact-hop table, negative cycles included") {
    Rng rng;
    for (int round = 0; round < 1200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), -6, 8);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int hops = static_cast<int>(rng.uniform(0, 2 * n));
        const auto table = hop_table(n, edges, s, hops);
        INFO("round ", round, " n=", n, " s=", s, " hops=", hops);
        REQUIRE_EQ(hop_limited_routes(n, edges, s, hops), table[idx(hops)]);
    }
}

PERF(2, "a modest hop budget on a large graph", 2500) {
    Rng rng;
    const int n = 500'000;
    const std::vector<Edge> edges = random_edges(rng, n, 1'500'000, -30, 40);
    const auto dist = hop_limited_routes(n, edges, 0, 25);
    CHECK_EQ(dist.size(), n);
}

TEST(3, "a source with no other reachable vertex needs zero hops") {
    CHECK_EQ(convergence_hops(2, {}, 0), 0);
}

TEST(3, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(convergence_hops(1, {}, 1), std::out_of_range);
}

TEST(3, "a direct edge needs one hop") {
    const std::vector<Edge> edges = {{0, 1, 7}};
    CHECK_EQ(convergence_hops(2, edges, 0), 1);
}

TEST(3, "the shortest path may need more hops than the direct edge") {
    const std::vector<Edge> edges = {{0, 2, 5}, {0, 1, 1}, {1, 2, 1}};
    CHECK_EQ(convergence_hops(3, edges, 0), 2);
}

TEST(3, "ties prefer the path with fewer hops") {
    const std::vector<Edge> edges = {{0, 2, 2}, {0, 1, 1}, {1, 2, 1}};
    CHECK_EQ(convergence_hops(3, edges, 0), 1);
}

TEST(3, "the farthest vertex decides, not the sum") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 2, 1}, {0, 3, 1}};
    CHECK_EQ(convergence_hops(4, edges, 0), 2);
}

TEST(3, "edge order cannot make it finish early") {
    // In-place relaxation in this order finishes everything in round one and would report 1.
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 2, 1}, {2, 3, 1}};
    CHECK_EQ(convergence_hops(4, edges, 0), 3);
}

TEST(3, "random graphs match the exact-hop table") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        std::vector<Edge> edges = random_no_negative_cycles(rng, n, static_cast<int>(rng.uniform(0, 3 * n)));
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        REQUIRE_EQ(convergence_hops(n, edges, s), brute_convergence(n, edges, s));
    }
}

PERF(3, "measuring convergence on a huge network", 2500) {
    Rng rng;
    const int n = 800'000;
    const int layers = 20;
    const std::vector<Edge> edges = layered_network(rng, n, layers, 1'000'000, 200'000);
    CHECK_EQ(convergence_hops(n, edges, 0), layers);
}
