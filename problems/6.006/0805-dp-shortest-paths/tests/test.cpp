#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Edges = std::vector<Edge>;

// Minimum weight over all walks of at most `steps` edges, by enumerating them.
void enumerate_walks(const Edges& edges, int at, int steps, std::int64_t weight,
                     std::vector<std::int64_t>& best) {
    best[static_cast<std::size_t>(at)] = std::min(best[static_cast<std::size_t>(at)], weight);
    if (steps == 0) {
        return;
    }
    for (const Edge& edge : edges) {
        if (edge.from == at) {
            enumerate_walks(edges, edge.to, steps - 1, weight + edge.weight, best);
        }
    }
}

// Single-source shortest distances from Bellman-Ford with -infinity propagation.
std::vector<std::int64_t> bellman_ford(int n, const Edges& edges, int source) {
    std::vector<std::int64_t> dist(static_cast<std::size_t>(n), kUnreachable);
    dist[static_cast<std::size_t>(source)] = 0;
    for (int round = 0; round + 1 < n; ++round) {
        for (const Edge& e : edges) {
            if (dist[static_cast<std::size_t>(e.from)] != kUnreachable &&
                dist[static_cast<std::size_t>(e.from)] + e.weight < dist[static_cast<std::size_t>(e.to)]) {
                dist[static_cast<std::size_t>(e.to)] = dist[static_cast<std::size_t>(e.from)] + e.weight;
            }
        }
    }
    for (int round = 0; round < n; ++round) {
        for (const Edge& e : edges) {
            const std::int64_t from = dist[static_cast<std::size_t>(e.from)];
            if (from == kUnreachable) {
                continue;
            }
            const std::int64_t reach = from == kMinusInfinity ? kMinusInfinity : from + e.weight;
            if (reach < dist[static_cast<std::size_t>(e.to)]) {
                dist[static_cast<std::size_t>(e.to)] = kMinusInfinity;
            }
        }
    }
    return dist;
}

// Random graph with negative edges but no negative cycles, via vertex potentials.
Edges random_no_negative_cycles(Rng& rng, int n, int m) {
    const auto potentials = rng.ints(static_cast<std::size_t>(n), -9, 9);
    Edges edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        const std::int64_t base = rng.uniform(0, 6);
        edges.push_back({u, v,
                         base + potentials[static_cast<std::size_t>(u)] -
                             potentials[static_cast<std::size_t>(v)]});
    }
    return edges;
}

Edges random_any(Rng& rng, int n, int m) {
    Edges edges;
    for (int i = 0; i < m; ++i) {
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                         static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(-4, 6)});
    }
    return edges;
}

std::int64_t cheapest_edge(const Edges& edges, int from, int to) {
    std::int64_t best = kUnreachable;
    for (const Edge& edge : edges) {
        if (edge.from == from && edge.to == to) {
            best = std::min(best, edge.weight);
        }
    }
    return best;
}

}  // namespace

TEST(1, "zero edges reach only the source") {
    const Edges edges{{0, 1, 5}};
    const auto dist = shortest_within_edges(2, edges, 0, 0);
    CHECK_EQ(dist[0], 0);
    CHECK_EQ(dist[1], kUnreachable);
}

TEST(1, "the edge limit is respected even when edges appear in path order") {
    // A single round that relaxes edges in place would walk the whole chain at once.
    const Edges edges{{0, 1, 1}, {1, 2, 1}, {2, 3, 1}};
    const auto dist = shortest_within_edges(4, edges, 0, 1);
    CHECK_EQ(dist[1], 1);
    CHECK_EQ(dist[2], kUnreachable);
    CHECK_EQ(dist[3], kUnreachable);
}

TEST(1, "a longer path can be cheaper once more edges are allowed") {
    const Edges edges{{0, 2, 10}, {0, 1, 1}, {1, 2, 1}};
    CHECK_EQ(shortest_within_edges(3, edges, 0, 1)[2], 10);
    CHECK_EQ(shortest_within_edges(3, edges, 0, 2)[2], 2);
}

TEST(1, "negative weights work and walks may repeat a cycle") {
    const Edges edges{{0, 1, -1}, {1, 0, -1}};
    CHECK_EQ(shortest_within_edges(2, edges, 0, 4)[0], -4);
    CHECK_EQ(shortest_within_edges(2, edges, 0, 4)[1], -3);
}

TEST(1, "random graphs match enumerating all walks") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 6));
        const Edges edges = random_any(rng, n, static_cast<int>(rng.uniform(0, 8)));
        const int source = static_cast<int>(rng.uniform(0, n - 1));
        const int k = static_cast<int>(rng.uniform(0, 5));
        INFO("trial ", trial, " n ", n, " k ", k);
        std::vector<std::int64_t> expected(static_cast<std::size_t>(n), kUnreachable);
        enumerate_walks(edges, source, k, 0, expected);
        REQUIRE_EQ(shortest_within_edges(n, edges, source, k), expected);
    }
}

PERF(1, "many rounds on a sparse graph", 800) {
    Rng rng;
    const Edges edges = random_any(rng, 2000, 40'000);
    CHECK_EQ(shortest_within_edges(2000, edges, 0, 500)[0] <= 0, true);
}

TEST(2, "a single vertex is at distance zero from itself") {
    const AllPairs pairs(1, {});
    CHECK_EQ(pairs.distance(0, 0), 0);
}

TEST(2, "unreachable pairs stay unreachable and do not overflow") {
    const Edges edges{{0, 1, 5}};
    const AllPairs pairs(3, edges);
    CHECK_EQ(pairs.distance(1, 0), kUnreachable);
    CHECK_EQ(pairs.distance(0, 2), kUnreachable);
}

TEST(2, "parallel edges keep the cheapest") {
    const Edges edges{{0, 1, 9}, {0, 1, 2}, {0, 1, 5}};
    const AllPairs pairs(2, edges);
    CHECK_EQ(pairs.distance(0, 1), 2);
}

TEST(2, "negative edges are handled without any negative cycle") {
    const Edges edges{{0, 1, 4}, {1, 2, -2}, {0, 2, 3}};
    const AllPairs pairs(3, edges);
    CHECK_EQ(pairs.distance(0, 2), 2);
}

TEST(2, "matches Bellman-Ford from every source") {
    Rng rng;
    for (int trial = 0; trial < 400; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const Edges edges = random_no_negative_cycles(rng, n, static_cast<int>(rng.uniform(0, 40)));
        const AllPairs pairs(n, edges);
        for (int source = 0; source < n; ++source) {
            const auto expected = bellman_ford(n, edges, source);
            for (int target = 0; target < n; ++target) {
                INFO("trial ", trial, " from ", source, " to ", target);
                REQUIRE_EQ(pairs.distance(source, target), expected[static_cast<std::size_t>(target)]);
            }
        }
    }
}

PERF(2, "seven hundred vertices in cubic time", 2000) {
    Rng rng;
    const Edges edges = random_no_negative_cycles(rng, 700, 30'000);
    const AllPairs pairs(700, edges);
    CHECK_EQ(pairs.distance(3, 3), 0);
}

TEST(3, "trivial paths") {
    const Edges edges{{0, 1, 5}};
    const AllPairs pairs(3, edges);
    CHECK_EQ(pairs.path(2, 2), std::vector<int>{2});
    CHECK_EQ(pairs.path(0, 1), std::vector<int>{0, 1});
    CHECK(pairs.path(1, 0).empty());
    CHECK(pairs.path(0, 2).empty());
}

TEST(3, "zero-weight cycles do not trap the path") {
    const Edges edges{{0, 1, 0}, {1, 0, 0}, {1, 2, 3}};
    const AllPairs pairs(3, edges);
    const auto path = pairs.path(0, 2);
    REQUIRE_EQ(path.size(), 3);
    CHECK_EQ(path, std::vector<int>{0, 1, 2});
}

TEST(3, "random paths are real, short and sum to the distance") {
    Rng rng;
    for (int trial = 0; trial < 400; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const Edges edges = random_no_negative_cycles(rng, n, static_cast<int>(rng.uniform(0, 40)));
        const AllPairs pairs(n, edges);
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                INFO("trial ", trial, " from ", u, " to ", v);
                const auto path = pairs.path(u, v);
                if (pairs.distance(u, v) == kUnreachable) {
                    REQUIRE(path.empty());
                    continue;
                }
                REQUIRE(!path.empty());
                REQUIRE_EQ(path.front(), u);
                REQUIRE_EQ(path.back(), v);
                REQUIRE(static_cast<int>(path.size()) <= n);
                std::int64_t total = 0;
                for (std::size_t i = 0; i + 1 < path.size(); ++i) {
                    const std::int64_t step = cheapest_edge(edges, path[i], path[i + 1]);
                    REQUIRE(step != kUnreachable);
                    total += step;
                }
                REQUIRE_EQ(total, pairs.distance(u, v));
            }
        }
    }
}

TEST(4, "no negative cycle is reported on a clean graph") {
    const Edges edges{{0, 1, -1}, {1, 2, -1}};
    CHECK(!AllPairs(3, edges).has_negative_cycle());
}

TEST(4, "a negative self-loop is a negative cycle") {
    const Edges edges{{0, 0, -1}};
    CHECK(AllPairs(1, edges).has_negative_cycle());
}

TEST(4, "pairs through the cycle are minus infinity, others stay finite") {
    // 0 -> 1 <-> 2 is a negative loop reachable from 0; 3 -> 4 is unrelated.
    const Edges edges{{0, 1, 1}, {1, 2, -3}, {2, 1, 1}, {3, 4, 7}};
    const AllPairs pairs(5, edges);
    CHECK(pairs.has_negative_cycle());
    CHECK_EQ(pairs.distance(0, 2), kMinusInfinity);
    CHECK_EQ(pairs.distance(2, 1), kMinusInfinity);
    CHECK_EQ(pairs.distance(3, 4), 7);
    CHECK_EQ(pairs.distance(0, 3), kUnreachable);
    CHECK(pairs.path(0, 2).empty());
    CHECK_EQ(pairs.path(3, 4), std::vector<int>{3, 4});
}

TEST(4, "distances that double every round do not overflow") {
    // On a complete graph of very negative edges, each intermediate vertex can double the
    // magnitude of the best walk, so an unclamped implementation overflows quickly.
    const int n = 70;
    Edges edges;
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            if (u != v) {
                edges.push_back({u, v, -std::int64_t{1'000'000'000'000'000}});
            }
        }
    }
    const AllPairs pairs(n, edges);
    CHECK(pairs.has_negative_cycle());
    CHECK_EQ(pairs.distance(0, 69), kMinusInfinity);
    CHECK_EQ(pairs.distance(5, 5), kMinusInfinity);
}

TEST(4, "random graphs match Bellman-Ford with minus infinity") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        const Edges edges = random_any(rng, n, static_cast<int>(rng.uniform(0, 25)));
        const AllPairs pairs(n, edges);
        bool expected_cycle = false;
        for (int source = 0; source < n; ++source) {
            const auto expected = bellman_ford(n, edges, source);
            for (int target = 0; target < n; ++target) {
                INFO("trial ", trial, " from ", source, " to ", target);
                REQUIRE_EQ(pairs.distance(source, target), expected[static_cast<std::size_t>(target)]);
                expected_cycle = expected_cycle || expected[static_cast<std::size_t>(target)] == kMinusInfinity;
            }
        }
        REQUIRE_EQ(pairs.has_negative_cycle(), expected_cycle);
    }
}
