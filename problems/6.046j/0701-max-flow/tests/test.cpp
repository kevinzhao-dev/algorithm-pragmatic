#include <algorithm>
#include <limits>
#include <map>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct RawEdge {
    int u, v;
    long long cap;
};

std::vector<RawEdge> random_edges(Rng& rng, int n, int m, int max_cap) {
    std::vector<RawEdge> edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u == v) {
            v = (v + 1) % n;
        }
        edges.push_back({u, v, rng.uniform(0, max_cap)});
    }
    return edges;
}

FlowNetwork build(int n, const std::vector<RawEdge>& edges) {
    FlowNetwork net(n);
    for (const auto& e : edges) {
        net.add_edge(e.u, e.v, e.cap);
    }
    return net;
}

// Exhaustive minimum s-t cut over every vertex bipartition (n <= 14).
long long brute_min_cut(int n, const std::vector<RawEdge>& edges, int s, int t) {
    long long best = std::numeric_limits<long long>::max();
    for (int mask = 0; mask < (1 << n); ++mask) {
        if (!(mask >> s & 1) || (mask >> t & 1)) {
            continue;
        }
        long long cap = 0;
        for (const auto& e : edges) {
            if ((mask >> e.u & 1) && !(mask >> e.v & 1)) {
                cap += e.cap;
            }
        }
        best = std::min(best, cap);
    }
    return best;
}

void check_flow(const FlowNetwork& net, int n, const std::vector<RawEdge>& edges, int s, int t,
                long long value) {
    std::vector<long long> balance(n, 0);
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        const long long f = net.flow_on(id);
        REQUIRE(f >= 0);
        REQUIRE(f <= edges[id].cap);
        balance[edges[id].u] -= f;
        balance[edges[id].v] += f;
    }
    for (int v = 0; v < n; ++v) {
        if (v != s && v != t) {
            CHECK_EQ(balance[v], 0);
        }
    }
    CHECK_EQ(balance[t], value);
    CHECK_EQ(balance[s], -value);
}

}  // namespace

TEST(1, "two parallel paths") {
    FlowNetwork net(4);
    net.add_edge(0, 1, 2);
    net.add_edge(1, 3, 2);
    net.add_edge(0, 2, 3);
    net.add_edge(2, 3, 3);
    CHECK_EQ(net.max_flow_dfs(0, 3), 5);
    CHECK_EQ(net.flow_on(1), 2);
}

TEST(1, "rerouting needs a residual back edge") {
    FlowNetwork net(4);
    // A greedy path through 1->2 must be undone for the optimum to appear.
    net.add_edge(0, 1, 1);
    net.add_edge(0, 2, 1);
    net.add_edge(1, 2, 1);
    net.add_edge(1, 3, 1);
    net.add_edge(2, 3, 1);
    CHECK_EQ(net.max_flow_dfs(0, 3), 2);
    CHECK_EQ(net.flow_on(2), 0);
}

TEST(1, "unreachable sink has zero flow") {
    FlowNetwork net(3);
    net.add_edge(0, 1, 7);
    CHECK_EQ(net.max_flow_dfs(0, 2), 0);
    CHECK_EQ(net.flow_on(0), 0);
}

TEST(1, "parallel and antiparallel edges") {
    FlowNetwork net(3);
    net.add_edge(0, 1, 4);
    net.add_edge(0, 1, 3);
    net.add_edge(1, 0, 9);
    net.add_edge(1, 2, 5);
    CHECK_EQ(net.max_flow_dfs(0, 2), 5);
}

TEST(1, "source equal to sink is rejected") {
    FlowNetwork net(2);
    CHECK_THROWS_AS(net.max_flow_dfs(1, 1), std::invalid_argument);
}

TEST(1, "calling twice recomputes from scratch") {
    FlowNetwork net(3);
    net.add_edge(0, 1, 2);
    net.add_edge(1, 2, 2);
    CHECK_EQ(net.max_flow_dfs(0, 2), 2);
    CHECK_EQ(net.max_flow_dfs(0, 2), 2);
    CHECK_EQ(net.flow_on(0), 2);
}

TEST(1, "random networks match the exhaustive minimum cut") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 10));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 30)), 8);
        auto net = build(n, edges);
        const int s = 0;
        const int t = n - 1;
        INFO("round ", round);
        const long long value = net.max_flow_dfs(s, t);
        check_flow(net, n, edges, s, t, value);
        CHECK_EQ(value, brute_min_cut(n, edges, s, t));
    }
}

TEST(2, "shortest augmenting paths ignore a huge cross edge") {
    const long long big = 1'000'000'000;
    FlowNetwork net(4);
    net.add_edge(0, 1, big);
    net.add_edge(0, 2, big);
    net.add_edge(1, 2, 1);
    net.add_edge(1, 3, big);
    net.add_edge(2, 3, big);
    CHECK_EQ(net.max_flow_bfs(0, 3), 2 * big);
    CHECK_EQ(net.augmentations(), 2);
    CHECK_EQ(net.flow_on(2), 0);
}

TEST(2, "edmonds-karp stays within the O(VE) augmentation bound") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 30));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 120)), 20);
        auto net = build(n, edges);
        INFO("round ", round);
        const long long value = net.max_flow_bfs(0, n - 1);
        check_flow(net, n, edges, 0, n - 1, value);
        CHECK_LE(net.augmentations(), n * static_cast<int>(edges.size()));
        if (n <= 12) {
            CHECK_EQ(value, brute_min_cut(n, edges, 0, n - 1));
        }
    }
}

TEST(2, "both algorithms agree") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 40));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 200)), 15);
        auto net = build(n, edges);
        INFO("round ", round);
        CHECK_EQ(net.max_flow_bfs(0, n - 1), net.max_flow_dfs(0, n - 1));
    }
}

PERF(2, "a few thousand vertices and many long paths", 1000) {
    // Layered graph: many shortest augmenting paths of growing length.
    Rng rng;
    const int layers = 60;
    const int width = 25;
    const int n = 2 + layers * width;
    FlowNetwork net(n);
    for (int i = 0; i < width; ++i) {
        net.add_edge(0, 2 + i, 50);
        net.add_edge(2 + (layers - 1) * width + i, 1, 50);
    }
    for (int layer = 0; layer + 1 < layers; ++layer) {
        for (int i = 0; i < width; ++i) {
            for (int j = 0; j < 6; ++j) {
                const int to = static_cast<int>(rng.uniform(0, width - 1));
                net.add_edge(2 + layer * width + i, 2 + (layer + 1) * width + to,
                             rng.uniform(1, 30));
            }
        }
    }
    CHECK_GT(net.max_flow_bfs(0, 1), 0);
}

TEST(3, "the cut certifies a small network") {
    FlowNetwork net(4);
    net.add_edge(0, 1, 3);
    net.add_edge(1, 3, 1);
    net.add_edge(0, 2, 2);
    net.add_edge(2, 3, 1);
    net.max_flow_bfs(0, 3);
    const auto side = net.min_cut_side();
    REQUIRE_EQ(side.size(), 4);
    CHECK(side[0]);
    CHECK(!side[3]);
    long long capacity = 0;
    const std::vector<RawEdge> raw{{0, 1, 3}, {1, 3, 1}, {0, 2, 2}, {2, 3, 1}};
    for (int id : net.cut_edges()) {
        capacity += raw[id].cap;
    }
    CHECK_EQ(capacity, 2);
}

TEST(3, "the cut works for zero flow") {
    FlowNetwork net(3);
    net.add_edge(0, 1, 5);
    net.max_flow_bfs(0, 2);
    CHECK(net.cut_edges().empty());
    const auto side = net.min_cut_side();
    CHECK(side[0] && side[1] && !side[2]);
}

TEST(3, "cut capacity equals flow value on random networks") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 40));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 200)), 15);
        auto net = build(n, edges);
        const int s = 0, t = n - 1;
        const long long value = net.max_flow_bfs(s, t);
        INFO("round ", round);
        const auto side = net.min_cut_side();
        REQUIRE_EQ(side.size(), n);
        CHECK(side[s]);
        CHECK(!side[t]);
        std::vector<int> expected_edges;
        long long capacity = 0;
        for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
            if (side[edges[id].u] && !side[edges[id].v]) {
                expected_edges.push_back(id);
                capacity += edges[id].cap;
            }
        }
        CHECK_EQ(net.cut_edges(), expected_edges);
        CHECK_EQ(capacity, value);
    }
}

TEST(4, "a single path comes back whole") {
    FlowNetwork net(3);
    net.add_edge(0, 1, 4);
    net.add_edge(1, 2, 3);
    net.max_flow_bfs(0, 2);
    const auto paths = net.decompose();
    REQUIRE_EQ(paths.size(), 1);
    CHECK_EQ(paths[0].edges, std::vector<int>{0, 1});
    CHECK_EQ(paths[0].amount, 3);
}

TEST(4, "zero flow has no paths") {
    FlowNetwork net(2);
    net.add_edge(0, 1, 0);
    net.max_flow_bfs(0, 1);
    CHECK(net.decompose().empty());
}

TEST(4, "random flows decompose into valid s-t paths") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 25));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(1, 120)), 15);
        auto net = build(n, edges);
        const int s = 0, t = n - 1;
        const long long value = net.max_flow_bfs(s, t);
        INFO("round ", round);
        const auto paths = net.decompose();
        CHECK_LE(paths.size(), edges.size());
        std::vector<long long> used(edges.size(), 0);
        long long total = 0;
        for (const auto& path : paths) {
            REQUIRE(path.amount > 0);
            REQUIRE(!path.edges.empty());
            total += path.amount;
            CHECK_EQ(edges[path.edges.front()].u, s);
            CHECK_EQ(edges[path.edges.back()].v, t);
            std::set<int> visited{s};
            for (std::size_t i = 0; i < path.edges.size(); ++i) {
                const auto& e = edges[path.edges[i]];
                if (i > 0) {
                    REQUIRE(edges[path.edges[i - 1]].v == e.u);
                }
                CHECK(visited.insert(e.v).second);
                used[path.edges[i]] += path.amount;
            }
        }
        CHECK_EQ(total, value);
        for (std::size_t id = 0; id < edges.size(); ++id) {
            CHECK_LE(used[id], net.flow_on(static_cast<int>(id)));
        }
    }
}
