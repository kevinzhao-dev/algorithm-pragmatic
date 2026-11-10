#include <algorithm>
#include <limits>
#include <queue>
#include <set>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using netsim::SyncNetwork;
using netsim::Topology;
using practice::Rng;

namespace {

std::vector<std::vector<int>> adjacency(const Topology& t) {
    std::vector<std::vector<int>> adj(t.size());
    for (int v = 0; v < t.size(); ++v) {
        for (int port = 0; port < t.degree(v); ++port) {
            adj[v].push_back(t.neighbor(v, port));
        }
    }
    return adj;
}

std::vector<int> bfs_distances(const Topology& t, int root) {
    const auto adj = adjacency(t);
    std::vector<int> dist(t.size(), -1);
    std::queue<int> q;
    q.push(root);
    dist[root] = 0;
    while (!q.empty()) {
        const int v = q.front();
        q.pop();
        for (int w : adj[v]) {
            if (dist[w] == -1) {
                dist[w] = dist[v] + 1;
                q.push(w);
            }
        }
    }
    return dist;
}

int lg_ceil(int n) {
    int lg = 0;
    while ((1 << lg) < n) {
        ++lg;
    }
    return lg;
}

std::vector<Topology> sample_topologies() {
    std::vector<Topology> out;
    out.push_back(Topology::ring(7));
    out.push_back(Topology::path(10));
    out.push_back(Topology::clique(8));
    out.push_back(Topology::grid(4, 5));
    out.push_back(Topology::random_connected(30, 25, 11));
    out.push_back(Topology::random_connected(60, 120, 12));
    return out;
}

}  // namespace

TEST(1, "exactly the largest uid wins") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 12));
        auto uids = rng.permutation(static_cast<std::size_t>(n));
        std::vector<CliqueLeader> procs;
        for (int v = 0; v < n; ++v) {
            procs.emplace_back(uids[v] * 7 + 1);
        }
        SyncNetwork<CliqueLeader> net(Topology::clique(n), std::move(procs));
        const auto stats = net.run(5);
        INFO("round ", round, " n=", n);
        CHECK_EQ(stats.rounds, 1);
        CHECK_EQ(stats.messages, n * (n - 1));
        int leaders = 0;
        int leader_vertex = -1;
        for (int v = 0; v < n; ++v) {
            if (net.process(v).is_leader()) {
                ++leaders;
                leader_vertex = v;
            }
            CHECK(net.process(v).halted());
        }
        CHECK_EQ(leaders, 1);
        REQUIRE(leader_vertex >= 0);
        CHECK_EQ(uids[leader_vertex], n - 1);
    }
}

TEST(2, "a two-node network") {
    std::vector<BfsTree> procs;
    procs.emplace_back(true);
    procs.emplace_back(false);
    SyncNetwork<BfsTree> net(Topology::path(2), std::move(procs));
    net.run(100);
    CHECK(net.process(0).finished());
    CHECK_EQ(net.process(1).parent_port(), 0);
    CHECK_EQ(net.process(1).depth(), 1);
    CHECK_EQ(net.process(0).children_ports(), std::vector<int>{0});
}

TEST(2, "trees on assorted graphs are breadth-first and terminate") {
    int index = 0;
    for (const Topology& t : sample_topologies()) {
        ++index;
        const int n = t.size();
        const int root = index % n;
        std::vector<BfsTree> procs;
        for (int v = 0; v < n; ++v) {
            procs.emplace_back(v == root);
        }
        const auto dist = bfs_distances(t, root);
        const int ecc = *std::max_element(dist.begin(), dist.end());
        SyncNetwork<BfsTree> net(t, std::move(procs));
        const auto stats = net.run(5000);
        INFO("topology ", index);
        // Everyone halted and the root knows the tree is complete.
        for (int v = 0; v < n; ++v) {
            CHECK(net.process(v).halted());
        }
        CHECK(net.process(root).finished());
        CHECK_EQ(net.process(root).depth(), 0);
        CHECK_EQ(net.process(root).parent_port(), -1);
        std::vector<std::set<int>> kids(n);
        for (int v = 0; v < n; ++v) {
            CHECK_EQ(net.process(v).depth(), dist[v]);
            if (v != root) {
                const int port = net.process(v).parent_port();
                REQUIRE(port >= 0 && port < t.degree(v));
                const int parent = t.neighbor(v, port);
                CHECK_EQ(dist[parent], dist[v] - 1);
                CHECK(kids[parent].insert(t.reverse_port(v, port)).second);
            }
        }
        for (int v = 0; v < n; ++v) {
            std::vector<int> expected(kids[v].begin(), kids[v].end());
            CHECK_EQ(net.process(v).children_ports(), expected);
        }
        CHECK_LE(stats.messages, 4 * t.edge_count() + n);
        CHECK_LE(stats.rounds, 3 * ecc + 8);
    }
}

TEST(2, "a star uses one round per level") {
    Topology t(9);
    for (int v = 1; v < 9; ++v) {
        t.add_edge(0, v);
    }
    std::vector<BfsTree> procs;
    for (int v = 0; v < 9; ++v) {
        procs.emplace_back(v == 0);
    }
    SyncNetwork<BfsTree> net(t, std::move(procs));
    net.run(100);
    CHECK(net.process(0).finished());
    for (int v = 1; v < 9; ++v) {
        CHECK_EQ(net.process(v).depth(), 1);
        CHECK(net.process(v).children_ports().empty());
    }
}

TEST(3, "a single isolated vertex joins immediately") {
    std::vector<Luby> procs;
    procs.emplace_back(1, 1);
    SyncNetwork<Luby> net(Topology(1), std::move(procs));
    net.run(10);
    CHECK(net.process(0).in_mis());
    CHECK(net.process(0).decided());
}

TEST(3, "independent and maximal on many graphs and seeds") {
    int seed_base = 0;
    for (const Topology& t : sample_topologies()) {
        const auto adj = adjacency(t);
        const int n = t.size();
        for (int trial = 0; trial < 50; ++trial) {
            ++seed_base;
            std::vector<Luby> procs;
            for (int v = 0; v < n; ++v) {
                procs.emplace_back(static_cast<std::uint64_t>(seed_base) * 1000u + v,
                                   static_cast<std::uint64_t>(n));
            }
            SyncNetwork<Luby> net(t, std::move(procs));
            const auto stats = net.run(100'000);
            INFO("seed ", seed_base, " n=", n);
            std::vector<char> in(n, 0);
            for (int v = 0; v < n; ++v) {
                REQUIRE(net.process(v).decided());
                in[v] = net.process(v).in_mis();
            }
            for (int v = 0; v < n; ++v) {
                bool neighbor_in = false;
                for (int w : adj[v]) {
                    if (in[v] && in[w]) {
                        CHECK(!(in[v] && in[w]));
                    }
                    neighbor_in = neighbor_in || in[w];
                }
                CHECK(in[v] || neighbor_in);  // maximality
            }
            const int phases = static_cast<int>(stats.rounds / 2);
            CHECK_LE(phases, 12 * lg_ceil(std::max(n, 2)) + 12);
            CHECK_LE(stats.messages, 2 * static_cast<std::int64_t>(t.edge_count()) * stats.rounds);
        }
    }
}

TEST(3, "large cliques take a logarithmic number of phases") {
    const int n = 256;
    std::vector<Luby> procs;
    for (int v = 0; v < n; ++v) {
        procs.emplace_back(777u + static_cast<std::uint64_t>(v), static_cast<std::uint64_t>(n));
    }
    SyncNetwork<Luby> net(Topology::clique(n), std::move(procs));
    const auto stats = net.run(100'000);
    int in_set = 0;
    for (int v = 0; v < n; ++v) {
        in_set += net.process(v).in_mis() ? 1 : 0;
    }
    CHECK_EQ(in_set, 1);
    CHECK_LE(stats.rounds, 2 * (12 * lg_ceil(n) + 12));
}

namespace {

struct WeightedTopology {
    Topology t;
    std::vector<std::vector<long long>> weights;  // per vertex, per port
};

WeightedTopology random_weighted(int n, int extra, std::uint64_t seed) {
    Topology t = Topology::random_connected(n, extra, seed);
    std::mt19937_64 rng(seed ^ 0xABCDEF);
    std::vector<std::vector<long long>> weights(n);
    for (int v = 0; v < n; ++v) {
        weights[v].assign(static_cast<std::size_t>(t.degree(v)), 0);
    }
    for (int v = 0; v < n; ++v) {
        for (int port = 0; port < t.degree(v); ++port) {
            const int u = t.neighbor(v, port);
            if (v < u) {
                const long long w =
                    static_cast<long long>(std::uniform_int_distribution<int>(1, 50)(rng));
                weights[v][port] = w;
                weights[u][t.reverse_port(v, port)] = w;
            }
        }
    }
    return {std::move(t), std::move(weights)};
}

std::vector<long long> dijkstra(const WeightedTopology& g, int root) {
    const int n = g.t.size();
    std::vector<long long> dist(n, kInf);
    using Entry = std::pair<long long, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;
    dist[root] = 0;
    heap.emplace(0, root);
    while (!heap.empty()) {
        const auto [d, v] = heap.top();
        heap.pop();
        if (d > dist[v]) {
            continue;
        }
        for (int port = 0; port < g.t.degree(v); ++port) {
            const int w = g.t.neighbor(v, port);
            if (d + g.weights[v][port] < dist[w]) {
                dist[w] = d + g.weights[v][port];
                heap.emplace(dist[w], w);
            }
        }
    }
    return dist;
}

}  // namespace

TEST(4, "the root stays at distance zero with no parent") {
    std::vector<SyncBellmanFord> procs;
    procs.emplace_back(true, std::vector<long long>{3}, 2);
    procs.emplace_back(false, std::vector<long long>{3}, 2);
    SyncNetwork<SyncBellmanFord> net(Topology::path(2), std::move(procs));
    net.run(10);
    CHECK_EQ(net.process(0).distance(), 0);
    CHECK_EQ(net.process(0).parent_port(), -1);
    CHECK_EQ(net.process(1).distance(), 3);
    CHECK_EQ(net.process(1).parent_port(), 0);
}

TEST(4, "n-1 rounds suffice and distances match Dijkstra") {
    for (std::uint64_t seed = 1; seed <= 40; ++seed) {
        const int n = 2 + static_cast<int>(seed) % 30;
        const auto g = random_weighted(n, n, seed);
        const int root = static_cast<int>(seed % static_cast<std::uint64_t>(n));
        std::vector<SyncBellmanFord> procs;
        for (int v = 0; v < n; ++v) {
            procs.emplace_back(v == root, g.weights[v], n - 1);
        }
        SyncNetwork<SyncBellmanFord> net(g.t, std::move(procs));
        const auto stats = net.run(n + 5);
        const auto expected = dijkstra(g, root);
        INFO("seed ", seed);
        CHECK_EQ(stats.rounds, n - 1);
        CHECK_LE(stats.messages, 2 * static_cast<std::int64_t>(g.t.edge_count()) * (n - 1));
        for (int v = 0; v < n; ++v) {
            CHECK_EQ(net.process(v).distance(), expected[v]);
            if (v != root) {
                const int port = net.process(v).parent_port();
                REQUIRE(port >= 0 && port < g.t.degree(v));
                CHECK_EQ(expected[g.t.neighbor(v, port)] + g.weights[v][port], expected[v]);
            }
        }
    }
}

TEST(4, "a long low-weight path needs nearly n rounds") {
    const int n = 12;
    Topology t(n);
    for (int v = 0; v + 1 < n; ++v) {
        t.add_edge(v, v + 1);
    }
    t.add_edge(0, n - 1);  // a heavy shortcut that the long path beats
    std::vector<std::vector<long long>> weights(n);
    for (int v = 0; v < n; ++v) {
        weights[v].assign(static_cast<std::size_t>(t.degree(v)), 1);
    }
    const int shortcut_port_0 = t.degree(0) - 1;
    const int shortcut_port_last = t.degree(n - 1) - 1;
    weights[0][shortcut_port_0] = 1000;
    weights[n - 1][shortcut_port_last] = 1000;

    std::vector<SyncBellmanFord> procs;
    for (int v = 0; v < n; ++v) {
        procs.emplace_back(v == 0, weights[v], n - 1);
    }
    SyncNetwork<SyncBellmanFord> net(t, std::move(procs));
    net.run(n + 5);
    CHECK_EQ(net.process(n - 1).distance(), n - 1);
}
