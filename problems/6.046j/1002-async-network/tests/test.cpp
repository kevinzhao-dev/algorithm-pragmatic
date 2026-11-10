#include <algorithm>
#include <cmath>
#include <queue>
#include <random>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using netsim::AsyncNetwork;
using netsim::random_delay;
using netsim::Topology;
using netsim::unit_delay;
using practice::Rng;

namespace {

std::vector<int> bfs_distances(const Topology& t, int root) {
    std::vector<int> dist(t.size(), -1);
    std::queue<int> q;
    q.push(root);
    dist[root] = 0;
    while (!q.empty()) {
        const int v = q.front();
        q.pop();
        for (int port = 0; port < t.degree(v); ++port) {
            const int w = t.neighbor(v, port);
            if (dist[w] == -1) {
                dist[w] = dist[v] + 1;
                q.push(w);
            }
        }
    }
    return dist;
}

std::vector<Topology> topologies() {
    std::vector<Topology> out;
    out.push_back(Topology::ring(9));
    out.push_back(Topology::path(12));
    out.push_back(Topology::grid(5, 5));
    out.push_back(Topology::clique(9));
    out.push_back(Topology::random_connected(40, 60, 7));
    out.push_back(Topology::random_connected(80, 40, 8));
    return out;
}

}  // namespace

TEST(1, "two nodes settle on the larger value") {
    std::vector<MaxFlood> procs;
    procs.emplace_back(3);
    procs.emplace_back(8);
    AsyncNetwork<MaxFlood> net(Topology::path(2), std::move(procs));
    net.run();
    CHECK_EQ(net.process(0).value(), 8);
    CHECK_EQ(net.process(1).value(), 8);
}

TEST(1, "every process learns the maximum under random delays") {
    Rng rng;
    int idx = 0;
    for (const Topology& t : topologies()) {
        ++idx;
        for (int trial = 0; trial < 20; ++trial) {
            const int n = t.size();
            std::vector<long long> values(n);
            for (auto& v : values) {
                v = rng.uniform(-1000, 1000);
            }
            const long long expected = *std::max_element(values.begin(), values.end());
            std::vector<MaxFlood> procs;
            for (long long v : values) {
                procs.emplace_back(v);
            }
            AsyncNetwork<MaxFlood> net(t, std::move(procs),
                                       trial == 0 ? unit_delay() : random_delay(100u + trial, 9));
            const auto stats = net.run();
            INFO("topology ", idx, " trial ", trial);
            for (int v = 0; v < n; ++v) {
                CHECK_EQ(net.process(v).value(), expected);
            }
            CHECK_LE(stats.messages, 2 * static_cast<std::int64_t>(t.edge_count()) * n);
        }
    }
}

TEST(2, "the root has distance zero") {
    std::vector<AsyncBfs> procs;
    procs.emplace_back(true);
    procs.emplace_back(false);
    AsyncNetwork<AsyncBfs> net(Topology::path(2), std::move(procs));
    net.run();
    CHECK_EQ(net.process(0).distance(), 0);
    CHECK_EQ(net.process(1).distance(), 1);
    CHECK_EQ(net.process(1).parent_port(), 0);
    CHECK_EQ(net.process(0).parent_port(), -1);
}

TEST(2, "final distances are breadth-first regardless of timing") {
    int idx = 0;
    for (const Topology& t : topologies()) {
        ++idx;
        const auto expected = bfs_distances(t, 0);
        for (int trial = 0; trial < 25; ++trial) {
            std::vector<AsyncBfs> procs;
            for (int v = 0; v < t.size(); ++v) {
                procs.emplace_back(v == 0);
            }
            AsyncNetwork<AsyncBfs> net(t, std::move(procs), random_delay(50u + trial, 20));
            net.run();
            INFO("topology ", idx, " trial ", trial);
            for (int v = 0; v < t.size(); ++v) {
                CHECK_EQ(net.process(v).distance(), expected[v]);
                if (v != 0) {
                    const int port = net.process(v).parent_port();
                    REQUIRE(port >= 0 && port < t.degree(v));
                    CHECK_EQ(expected[t.neighbor(v, port)], expected[v] - 1);
                }
            }
        }
    }
}

TEST(2, "late fast paths force corrections, unit delays do not") {
    // On a ring, a fast clockwise wave reaches far vertices first with a long path; the short
    // counterclockwise path arrives later and every such vertex has to correct itself.
    const Topology t = Topology::ring(24);
    std::int64_t unit_messages = 0;
    {
        std::vector<AsyncBfs> procs;
        for (int v = 0; v < t.size(); ++v) {
            procs.emplace_back(v == 0);
        }
        AsyncNetwork<AsyncBfs> net(t, std::move(procs), unit_delay());
        unit_messages = net.run().messages;
    }
    std::int64_t skewed_messages = 0;
    {
        std::vector<AsyncBfs> procs;
        for (int v = 0; v < t.size(); ++v) {
            procs.emplace_back(v == 0);
        }
        AsyncNetwork<AsyncBfs> net(t, std::move(procs), [](int, int port, std::int64_t) {
            return port == Topology::kClockwise ? std::int64_t{1} : std::int64_t{1000};
        });
        skewed_messages = net.run().messages;
    }
    CHECK_LE(unit_messages, 2 * t.edge_count());
    CHECK_GT(skewed_messages, unit_messages);
}

TEST(3, "a two-node network counts two") {
    std::vector<CountingTree> procs;
    procs.emplace_back(true);
    procs.emplace_back(false);
    AsyncNetwork<CountingTree> net(Topology::path(2), std::move(procs));
    net.run();
    CHECK_EQ(net.process(0).root_count(), 2);
}

TEST(3, "an isolated root counts itself") {
    std::vector<CountingTree> procs;
    procs.emplace_back(true);
    AsyncNetwork<CountingTree> net(Topology(1), std::move(procs));
    net.run();
    CHECK_EQ(net.process(0).root_count(), 1);
}

TEST(3, "counts every node under arbitrary timing with few messages") {
    int idx = 0;
    for (const Topology& t : topologies()) {
        ++idx;
        for (int trial = 0; trial < 25; ++trial) {
            std::vector<CountingTree> procs;
            for (int v = 0; v < t.size(); ++v) {
                procs.emplace_back(v == 0);
            }
            AsyncNetwork<CountingTree> net(t, std::move(procs), random_delay(7u + trial, 15));
            const auto stats = net.run();
            INFO("topology ", idx, " trial ", trial);
            CHECK_EQ(net.process(0).root_count(), t.size());
            CHECK_LE(stats.messages, 4 * static_cast<std::int64_t>(t.edge_count()) + t.size());
        }
    }
}

namespace {

struct WeightedTopology {
    Topology t;
    std::vector<std::vector<long long>> weights;
};

WeightedTopology random_weighted(int n, int extra, std::uint64_t seed) {
    Topology t = Topology::random_connected(n, extra, seed);
    std::mt19937_64 rng(seed ^ 0x55AA);
    std::vector<std::vector<long long>> weights(n);
    for (int v = 0; v < n; ++v) {
        weights[v].assign(static_cast<std::size_t>(t.degree(v)), 0);
    }
    for (int v = 0; v < n; ++v) {
        for (int port = 0; port < t.degree(v); ++port) {
            const int u = t.neighbor(v, port);
            if (v < u || (v == u)) {
                const long long w = static_cast<long long>(
                    std::uniform_int_distribution<int>(1, 40)(rng));
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

// The lecture's two-parallel-edge chain: segment i has an "upper" edge of weight 2^(k-1-i) and a
// "lower" edge of weight 0; forward upper links are fast and lower links exponentially slow.
struct Gadget {
    Topology t;
    std::vector<std::vector<long long>> weights;
    int k;
};

Gadget make_gadget(int k) {
    Topology t(k + 1);
    std::vector<std::vector<long long>> weights(k + 1);
    for (int i = 0; i < k; ++i) {
        const long long up = 1LL << (k - 1 - i);
        t.add_edge(i, i + 1);
        weights[i].push_back(up);
        weights[i + 1].push_back(up);
        t.add_edge(i, i + 1);
        weights[i].push_back(0);
        weights[i + 1].push_back(0);
    }
    return {std::move(t), std::move(weights), k};
}

netsim::RunStats run_gadget(const Gadget& g) {
    std::vector<AsyncBellmanFord> procs;
    for (int v = 0; v <= g.k; ++v) {
        procs.emplace_back(v == 0, g.weights[v]);
    }
    const int k = g.k;
    // Forward channels of vertex i use ports (i==0 ? 0,1 : 2,3); everything else crawls.
    netsim::DelayFn adversary = [k](int from, int port, std::int64_t) -> std::int64_t {
        const int upper = from == 0 ? 0 : 2;
        const int lower = from == 0 ? 1 : 3;
        const bool forward = from < k && (port == upper || port == lower);
        if (!forward) {
            return 1'000'000'000'000'000;
        }
        if (port == upper) {
            return 1;
        }
        return static_cast<std::int64_t>(std::pow(3.0, k - 1 - from));  // lower link of segment i
    };
    AsyncNetwork<AsyncBellmanFord> net(g.t, std::move(procs), adversary);
    const auto stats = net.run(50'000'000);
    // Whatever the schedule, the final distances are still correct.
    for (int v = 0; v <= k; ++v) {
        CHECK_EQ(net.process(v).distance(), 0);
    }
    return stats;
}

}  // namespace

TEST(4, "converges to correct distances under random timing") {
    for (std::uint64_t seed = 1; seed <= 30; ++seed) {
        const int n = 2 + static_cast<int>(seed) * 2;
        const auto g = random_weighted(n, n, seed);
        const int root = static_cast<int>(seed) % n;
        std::vector<AsyncBellmanFord> procs;
        for (int v = 0; v < n; ++v) {
            procs.emplace_back(v == root, g.weights[v]);
        }
        AsyncNetwork<AsyncBellmanFord> net(g.t, std::move(procs), random_delay(seed + 500, 8));
        net.run();
        const auto expected = dijkstra(g, root);
        INFO("seed ", seed);
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

TEST(4, "adversarial timing costs exponentially many messages") {
    // messages must grow by a factor of about 2 per extra segment under this schedule.
    const auto small = run_gadget(make_gadget(4));
    const auto large = run_gadget(make_gadget(14));
    CHECK_GE(large.messages, 1 << 14);
    CHECK_GE(large.messages, small.messages * 200);
}

TEST(4, "the same gadget is cheap when good links are delivered first") {
    const Gadget g = make_gadget(14);
    std::vector<AsyncBellmanFord> procs;
    for (int v = 0; v <= g.k; ++v) {
        procs.emplace_back(v == 0, g.weights[v]);
    }
    const int k = g.k;
    AsyncNetwork<AsyncBellmanFord> net(g.t, std::move(procs),
                                       [k](int from, int port, std::int64_t) -> std::int64_t {
                                           const int upper = from == 0 ? 0 : 2;
                                           return port == upper && from < k ? 2 : 1;
                                       });
    const auto stats = net.run();
    for (int v = 0; v <= g.k; ++v) {
        CHECK_EQ(net.process(v).distance(), 0);
    }
    CHECK_LE(stats.messages, 4 * static_cast<std::int64_t>(g.t.edge_count()));
}
