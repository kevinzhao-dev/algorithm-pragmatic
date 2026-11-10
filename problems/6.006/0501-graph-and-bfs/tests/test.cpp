#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <queue>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using EdgeList = std::vector<std::pair<int, int>>;

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

std::vector<int> sorted_copy(std::span<const int> values) {
    std::vector<int> out(values.begin(), values.end());
    std::sort(out.begin(), out.end());
    return out;
}

Graph directed_graph(int n, const EdgeList& edges) {
    Graph g(n);
    for (const auto& [u, v] : edges) {
        g.add_directed(u, v);
    }
    return g;
}

Graph undirected_graph(int n, const EdgeList& edges) {
    Graph g(n);
    for (const auto& [a, b] : edges) {
        g.add_undirected(a, b);
    }
    return g;
}

EdgeList random_edges(Rng& rng, int n, int m) {
    EdgeList edges;
    for (int i = 0; i < m; ++i) {
        edges.emplace_back(static_cast<int>(rng.uniform(0, n - 1)),
                           static_cast<int>(rng.uniform(0, n - 1)));
    }
    return edges;
}

// Relaxes every edge until nothing changes; deliberately unrelated to a queue-based BFS.
std::vector<int> brute_distances(int n, const EdgeList& edges, int source) {
    std::vector<int> dist(idx(n), -1);
    dist[idx(source)] = 0;
    for (int round = 0; round < n; ++round) {
        for (const auto& [u, v] : edges) {
            if (dist[idx(u)] != -1 &&
                (dist[idx(v)] == -1 || dist[idx(v)] > dist[idx(u)] + 1)) {
                dist[idx(v)] = dist[idx(u)] + 1;
            }
        }
    }
    return dist;
}

bool has_edge(const EdgeList& edges, int u, int v) {
    return std::find(edges.begin(), edges.end(), std::pair{u, v}) != edges.end();
}

void check_tree(int n, const EdgeList& edges, int source, const BfsTree& tree) {
    REQUIRE_EQ(tree.dist.size(), n);
    REQUIRE_EQ(tree.parent.size(), n);
    for (int v = 0; v < n; ++v) {
        INFO("vertex ", v);
        if (tree.dist[idx(v)] == -1 || v == source) {
            CHECK_EQ(tree.parent[idx(v)], kNoVertex);
            continue;
        }
        const int p = tree.parent[idx(v)];
        REQUIRE(p != kNoVertex);
        CHECK_EQ(tree.dist[idx(p)], tree.dist[idx(v)] - 1);
        CHECK(has_edge(edges, p, v));
    }
}

// True when `walk` is a path in the edge set, from source to target.
bool is_walk(const EdgeList& edges, const std::vector<int>& walk, int source, int target) {
    if (walk.empty() || walk.front() != source || walk.back() != target) {
        return false;
    }
    for (std::size_t i = 0; i + 1 < walk.size(); ++i) {
        if (!has_edge(edges, walk[i], walk[i + 1])) {
            return false;
        }
    }
    return true;
}

// Fewest edges of any walk with an odd edge count; steps exact-length reachability forward.
std::optional<int> brute_odd_length(int n, const EdgeList& edges, int source, int target) {
    std::vector<char> at(idx(n), 0);
    at[idx(source)] = 1;
    for (int length = 1; length <= 2 * n + 1; ++length) {
        std::vector<char> next(idx(n), 0);
        for (const auto& [u, v] : edges) {
            if (at[idx(u)] != 0) {
                next[idx(v)] = 1;
            }
        }
        at = std::move(next);
        if (length % 2 == 1 && at[idx(target)] != 0) {
            return length;
        }
    }
    return std::nullopt;
}

}  // namespace

TEST(1, "a new graph has the requested vertices and no edges") {
    const Graph g(4);
    CHECK_EQ(g.vertex_count(), 4);
    CHECK_EQ(g.edge_count(), 0);
    for (int v = 0; v < 4; ++v) {
        CHECK(g.neighbors(v).empty());
        CHECK_EQ(g.out_degree(v), 0);
        CHECK_EQ(g.in_degree(v), 0);
    }
}

TEST(1, "zero vertices is a valid graph, a negative count is not") {
    const Graph g(0);
    CHECK_EQ(g.vertex_count(), 0);
    CHECK_THROWS_AS(Graph(-1), std::invalid_argument);
}

TEST(1, "a directed edge only appears on its tail") {
    Graph g(3);
    g.add_directed(0, 2);
    CHECK_EQ(sorted_copy(g.neighbors(0)), std::vector<int>{2});
    CHECK(g.neighbors(2).empty());
    CHECK_EQ(g.out_degree(0), 1);
    CHECK_EQ(g.in_degree(2), 1);
    CHECK_EQ(g.in_degree(0), 0);
    CHECK_EQ(g.edge_count(), 1);
}

TEST(1, "an undirected edge appears on both ends and counts once") {
    Graph g(3);
    g.add_undirected(0, 1);
    CHECK_EQ(sorted_copy(g.neighbors(0)), std::vector<int>{1});
    CHECK_EQ(sorted_copy(g.neighbors(1)), std::vector<int>{0});
    CHECK_EQ(g.in_degree(0), 1);
    CHECK_EQ(g.in_degree(1), 1);
    CHECK_EQ(g.edge_count(), 1);
}

TEST(1, "neighbors keep insertion order and parallel edges") {
    Graph g(4);
    g.add_directed(0, 3);
    g.add_directed(0, 1);
    g.add_directed(0, 3);
    const auto adj = g.neighbors(0);
    CHECK_EQ(std::vector<int>(adj.begin(), adj.end()), std::vector<int>{3, 1, 3});
    CHECK_EQ(g.in_degree(3), 2);
}

TEST(1, "a self-loop is stored once, directed or not") {
    Graph g(2);
    g.add_directed(0, 0);
    g.add_undirected(1, 1);
    CHECK_EQ(g.neighbors(0).size(), 1);
    CHECK_EQ(g.neighbors(1).size(), 1);
    CHECK_EQ(g.in_degree(1), 1);
    CHECK_EQ(g.out_degree(1), 1);
    CHECK_EQ(g.edge_count(), 2);
}

TEST(1, "unknown vertices throw out_of_range") {
    Graph g(2);
    CHECK_THROWS_AS(g.add_directed(0, 2), std::out_of_range);
    CHECK_THROWS_AS(g.add_directed(-1, 0), std::out_of_range);
    CHECK_THROWS_AS(g.add_undirected(2, 0), std::out_of_range);
    CHECK_THROWS_AS(g.neighbors(5), std::out_of_range);
    CHECK_THROWS_AS(g.out_degree(-1), std::out_of_range);
    CHECK_THROWS_AS(g.in_degree(2), std::out_of_range);
    CHECK_EQ(g.edge_count(), 0);
}

TEST(1, "degrees match a brute-force count on random graphs") {
    Rng rng;
    const int n = 50;
    Graph g(n);
    EdgeList edges = random_edges(rng, n, 1000);
    for (const auto& [u, v] : edges) {
        g.add_directed(u, v);
    }
    CHECK_EQ(g.edge_count(), edges.size());
    std::size_t total = 0;
    for (int v = 0; v < n; ++v) {
        total += static_cast<std::size_t>(g.out_degree(v));
        CHECK_EQ(g.out_degree(v), std::count_if(edges.begin(), edges.end(),
                                                [&](const auto& e) { return e.first == v; }));
        CHECK_EQ(g.in_degree(v), std::count_if(edges.begin(), edges.end(),
                                               [&](const auto& e) { return e.second == v; }));
    }
    CHECK_EQ(total, edges.size());
}

PERF(1, "adding edges and reading degrees is constant time", 1500) {
    const int n = 1'000'000;
    Graph g(n);
    for (int i = 0; i < 3'000'000; ++i) {
        const int u = i % n;
        const int v = (i * 7 + 3) % n;
        g.add_directed(u, v);
        g.add_directed(v, 0);
    }
    long long total = 0;
    for (int round = 0; round < 3; ++round) {
        for (int v = 0; v < n; ++v) {
            total += g.in_degree(v) + g.out_degree(v);
            total += static_cast<long long>(g.neighbors(v).size());
        }
    }
    CHECK_GT(total, 0);
}

TEST(2, "bfs from an isolated vertex only reaches itself") {
    const Graph g(3);
    const BfsTree tree = bfs(g, 1);
    CHECK_EQ(tree.dist, std::vector<int>{-1, 0, -1});
    CHECK_EQ(tree.parent, std::vector<int>{kNoVertex, kNoVertex, kNoVertex});
}

TEST(2, "bfs computes hop counts and respects edge direction") {
    const EdgeList edges = {{0, 1}, {1, 2}, {0, 3}, {3, 2}, {2, 4}, {4, 0}, {5, 0}};
    const Graph g = directed_graph(6, edges);
    const BfsTree tree = bfs(g, 0);
    CHECK_EQ(tree.dist, std::vector<int>{0, 1, 2, 1, 3, -1});
    check_tree(6, edges, 0, tree);
}

TEST(2, "self-loops and parallel edges do not disturb bfs") {
    const EdgeList edges = {{0, 0}, {0, 1}, {0, 1}, {1, 2}, {2, 2}};
    const Graph g = directed_graph(3, edges);
    const BfsTree tree = bfs(g, 0);
    CHECK_EQ(tree.dist, std::vector<int>{0, 1, 2});
    check_tree(3, edges, 0, tree);
}

TEST(2, "a bad source throws out_of_range") {
    const Graph g(2);
    CHECK_THROWS_AS(bfs(g, 2), std::out_of_range);
    CHECK_THROWS_AS(shortest_path(g, 0, 7), std::out_of_range);
    CHECK_THROWS_AS(level_sets(g, -1), std::out_of_range);
}

TEST(2, "a path from a vertex to itself is just that vertex") {
    const Graph g = directed_graph(2, {{0, 1}});
    CHECK_EQ(shortest_path(g, 1, 1), std::optional(std::vector<int>{1}));
}

TEST(2, "shortest_path reports an unreachable target") {
    const Graph g = directed_graph(3, {{0, 1}, {2, 0}});
    CHECK_EQ(shortest_path(g, 0, 2), std::optional<std::vector<int>>());
}

TEST(2, "shortest_path prefers fewer edges") {
    const EdgeList edges = {{0, 1}, {1, 2}, {2, 3}, {0, 4}, {4, 3}};
    const Graph g = directed_graph(5, edges);
    CHECK_EQ(shortest_path(g, 0, 3), std::optional(std::vector<int>{0, 4, 3}));
}

TEST(2, "level_sets groups vertices by distance") {
    const Graph g = undirected_graph(6, {{0, 1}, {0, 2}, {1, 3}, {2, 3}, {3, 4}});
    auto levels = level_sets(g, 0);
    for (auto& level : levels) {
        std::sort(level.begin(), level.end());
    }
    CHECK_EQ(levels, std::vector<std::vector<int>>{{0}, {1, 2}, {3}, {4}});
}

TEST(2, "random graphs match brute-force distances and valid parents") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 18));
        const int m = static_cast<int>(rng.uniform(0, 3 * n));
        const EdgeList edges = random_edges(rng, n, m);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " edges=", edges, " s=", s);
        const Graph g = directed_graph(n, edges);
        const BfsTree tree = bfs(g, s);
        const std::vector<int> expected = brute_distances(n, edges, s);
        REQUIRE_EQ(tree.dist, expected);
        check_tree(n, edges, s, tree);

        const int t = static_cast<int>(rng.uniform(0, n - 1));
        const auto path = shortest_path(g, s, t);
        REQUIRE_EQ(path.has_value(), expected[idx(t)] != -1);
        if (path) {
            CHECK_EQ(path->size(), expected[idx(t)] + 1);
            CHECK(is_walk(edges, *path, s, t));
        }

        const auto levels = level_sets(g, s);
        std::size_t reachable = 0;
        for (std::size_t d = 0; d < levels.size(); ++d) {
            CHECK(!levels[d].empty());
            reachable += levels[d].size();
            for (const int v : levels[d]) {
                CHECK_EQ(expected[idx(v)], d);
            }
        }
        CHECK_EQ(reachable, std::count_if(expected.begin(), expected.end(),
                                          [](int d) { return d != -1; }));
    }
}

// A queue built on vector::erase(begin()) or a rescan of all vertices per level is quadratic here.
PERF(2, "bfs is linear on a wide star and on a long chain", 1500) {
    const int n = 2'000'000;
    Graph star(n);
    for (int i = 1; i < n; ++i) {
        star.add_directed(0, i);
    }
    const BfsTree star_tree = bfs(star, 0);
    CHECK_EQ(star_tree.dist[idx(n - 1)], 1);

    Graph chain(n);
    for (int i = 0; i + 1 < n; ++i) {
        chain.add_directed(i, i + 1);
    }
    const BfsTree chain_tree = bfs(chain, 0);
    CHECK_EQ(chain_tree.dist[idx(n - 1)], n - 1);
}

TEST(3, "a single edge is an odd walk") {
    const EdgeList edges = {{0, 1}};
    const Graph g = directed_graph(2, edges);
    CHECK_EQ(shortest_odd_walk(g, 0, 1), std::optional(std::vector<int>{0, 1}));
}

TEST(3, "a triangle gives an odd walk back to the start") {
    const EdgeList edges = {{0, 1}, {1, 2}, {2, 0}};
    const Graph g = directed_graph(3, edges);
    const auto walk = shortest_odd_walk(g, 0, 0);
    REQUIRE(walk.has_value());
    CHECK_EQ(walk->size(), 4);
    CHECK(is_walk(edges, *walk, 0, 0));
}

TEST(3, "the empty walk does not count when source equals target") {
    const Graph g(1);
    CHECK_EQ(shortest_odd_walk(g, 0, 0), std::optional<std::vector<int>>());
}

TEST(3, "bipartite graphs have no odd walk between vertices on the same side") {
    const Graph g = undirected_graph(4, {{0, 1}, {1, 2}, {2, 3}, {3, 0}});
    CHECK_EQ(shortest_odd_walk(g, 0, 2), std::optional<std::vector<int>>());
    CHECK_EQ(shortest_odd_walk(g, 1, 1), std::optional<std::vector<int>>());
}

TEST(3, "the shortest odd walk may have to revisit a vertex") {
    // 0 and 2 are two edges apart on the short way; the only odd route detours through a triangle.
    EdgeList edges;
    for (const auto& [a, b] : EdgeList{{0, 1}, {1, 2}, {1, 3}, {3, 4}, {4, 1}}) {
        edges.emplace_back(a, b);
        edges.emplace_back(b, a);
    }
    const Graph g = directed_graph(5, edges);
    const auto walk = shortest_odd_walk(g, 0, 2);
    REQUIRE(walk.has_value());
    CHECK_EQ(walk->size(), 6);
    CHECK(is_walk(edges, *walk, 0, 2));
}

TEST(3, "random graphs match exact-length reachability") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const int m = static_cast<int>(rng.uniform(0, 2 * n));
        const EdgeList edges = random_edges(rng, n, m);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " edges=", edges, " s=", s, " t=", t);
        const Graph g = directed_graph(n, edges);
        const auto walk = shortest_odd_walk(g, s, t);
        const auto expected = brute_odd_length(n, edges, s, t);
        REQUIRE_EQ(walk.has_value(), expected.has_value());
        if (walk) {
            CHECK_EQ(walk->size(), *expected + 1);
            CHECK(is_walk(edges, *walk, s, t));
        }
    }
}

PERF(3, "shortest_odd_walk is linear", 1500) {
    const int n = 1'000'000;
    Graph g(n);
    for (int i = 0; i + 1 < n; ++i) {
        g.add_undirected(i, i + 1);
    }
    g.add_undirected(0, 2);
    const auto walk = shortest_odd_walk(g, 0, n - 1);
    REQUIRE(walk.has_value());
    CHECK_EQ(walk->size() % 2, 0);
}

TEST(4, "a vertex is zero hops from itself") {
    const Graph g(1);
    CHECK_EQ(separation(g, 0, 0), std::optional(0));
}

TEST(4, "neighbours are one hop apart") {
    const Graph g = undirected_graph(2, {{0, 1}});
    CHECK_EQ(separation(g, 0, 1), std::optional(1));
    CHECK_EQ(separation(g, 1, 0), std::optional(1));
}

TEST(4, "different components are not connected") {
    const Graph g = undirected_graph(4, {{0, 1}, {2, 3}});
    CHECK_EQ(separation(g, 0, 3), std::optional<int>());
}

TEST(4, "a bad vertex throws out_of_range") {
    const Graph g(2);
    CHECK_THROWS_AS(separation(g, 0, 2), std::out_of_range);
    CHECK_THROWS_AS(separation(g, -1, 0), std::out_of_range);
}

TEST(4, "separation agrees with bfs on random undirected graphs") {
    Rng rng;
    for (int round = 0; round < 2000; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 40));
        const int m = static_cast<int>(rng.uniform(n / 2, 2 * n));
        const EdgeList edges = random_edges(rng, n, m);
        INFO("round ", round, " n=", n, " edges=", edges);
        const Graph g = undirected_graph(n, edges);
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const BfsTree tree = bfs(g, a);
        for (int b = 0; b < n; ++b) {
            const int expected = tree.dist[idx(b)];
            REQUIRE_EQ(separation(g, a, b), expected == -1 ? std::optional<int>()
                                                           : std::optional<int>(expected));
        }
    }
}

// Each endpoint sits at the end of a short chain, so a one-sided BFS sweeps the whole core per query.
PERF(4, "queries stop as soon as the two frontiers meet", 8000) {
    Rng rng;
    const int core = 1'000'000;
    const int queries = 400;
    const int chain_length = 4;
    Graph g(core + 2 * queries * chain_length);
    for (int i = 0; i < 4'000'000; ++i) {
        g.add_undirected(static_cast<int>(rng.uniform(0, core - 1)),
                         static_cast<int>(rng.uniform(0, core - 1)));
    }
    int next_id = core;
    std::vector<std::pair<int, int>> endpoints;
    for (int query = 0; query < queries; ++query) {
        std::pair<int, int> ends;
        for (int side = 0; side < 2; ++side) {
            int attach = static_cast<int>(rng.uniform(0, core - 1));
            for (int step = 0; step < chain_length; ++step) {
                g.add_undirected(attach, next_id);
                attach = next_id++;
            }
            (side == 0 ? ends.first : ends.second) = attach;
        }
        endpoints.push_back(ends);
    }

    const practice::Stopwatch watch;
    int connected = 0;
    for (const auto& [a, b] : endpoints) {
        connected += separation(g, a, b).has_value() ? 1 : 0;
    }
    CHECK_GT(connected, queries * 3 / 4);
    CHECK_LT(watch.elapsed_ms(), 2500.0);
}
