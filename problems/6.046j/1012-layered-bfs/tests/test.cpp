#include <algorithm>
#include <queue>
#include <vector>

#include "06_net_sim.hpp"
#include "practice.hpp"
#include "solution.hpp"

using netsim::AsyncNetwork;
using netsim::DelayFn;
using netsim::random_delay;
using netsim::Topology;
using netsim::unit_delay;

namespace {

std::vector<int> bfs_levels(const Topology& t, int root) {
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

int diameter(const Topology& t) {
    int d = 0;
    for (int v = 0; v < t.size(); ++v) {
        const auto levels = bfs_levels(t, v);
        d = std::max(d, *std::max_element(levels.begin(), levels.end()));
    }
    return d;
}

void run_and_check(const Topology& t, int root, DelayFn delay, std::int64_t d) {
    const int n = t.size();
    std::vector<LayeredBfs> procs;
    for (int v = 0; v < n; ++v) {
        procs.emplace_back(v == root);
    }
    const auto levels = bfs_levels(t, root);
    const int diam = diameter(t);
    AsyncNetwork<LayeredBfs> net(t, std::move(procs), std::move(delay));
    const auto stats = net.run();
    REQUIRE(net.process(root).root_done());
    CHECK_EQ(net.process(root).level(), 0);
    CHECK_EQ(net.process(root).parent_port(), -1);
    CHECK(net.outputs(root).empty());
    for (int v = 0; v < n; ++v) {
        CHECK_EQ(net.process(v).level(), levels[v]);
        if (v != root) {
            REQUIRE_EQ(net.outputs(v).size(), 1);
            const int port = static_cast<int>(net.outputs(v).front().value);
            CHECK_EQ(port, net.process(v).parent_port());
            REQUIRE(port >= 0 && port < t.degree(v));
            CHECK_EQ(levels[t.neighbor(v, port)], levels[v] - 1);
        }
    }
    const std::int64_t e = t.edge_count();
    CHECK_LE(stats.messages, 4 * e + 2 * static_cast<std::int64_t>(n) * (diam + 2));
    CHECK_LE(stats.time, 3 * d * static_cast<std::int64_t>((diam + 3) * (diam + 3)));
}

std::vector<Topology> topologies() {
    std::vector<Topology> out;
    out.push_back(Topology::path(2));
    out.push_back(Topology::ring(10));
    out.push_back(Topology::path(15));
    out.push_back(Topology::grid(5, 7));
    out.push_back(Topology::clique(9));
    out.push_back(Topology::random_connected(40, 30, 21));
    out.push_back(Topology::random_connected(80, 120, 22));
    return out;
}

}  // namespace

TEST(1, "two nodes finish in one phase") {
    run_and_check(Topology::path(2), 0, unit_delay(), 1);
}

TEST(1, "a clique finishes after discovering everyone at once") {
    run_and_check(Topology::clique(12), 3, unit_delay(), 1);
}

TEST(1, "a long path grows one level per phase") {
    run_and_check(Topology::path(20), 0, unit_delay(), 1);
}

TEST(1, "assorted graphs and roots under unit delays") {
    int index = 0;
    for (const Topology& t : topologies()) {
        INFO("topology ", ++index);
        run_and_check(t, index % t.size(), unit_delay(), 1);
    }
}

TEST(1, "random delays never cause a correction") {
    int index = 0;
    for (const Topology& t : topologies()) {
        for (int trial = 0; trial < 20; ++trial) {
            INFO("topology ", index, " trial ", trial);
            run_and_check(t, trial % t.size(), random_delay(900u + 7u * trial, 12), 12);
        }
        ++index;
    }
}

TEST(1, "adversarial skew cannot create a wrong parent") {
    int index = 0;
    for (const Topology& t : topologies()) {
        // Messages from low-numbered vertices crawl, others fly: shortcuts lose the race.
        DelayFn delay = [](int from, int port, std::int64_t) -> std::int64_t {
            return from % 3 == 0 ? std::int64_t{40} : std::int64_t{1 + port % 2};
        };
        INFO("topology ", ++index);
        run_and_check(t, 0, std::move(delay), 40);
    }
}

TEST(1, "the message bound is linear in E + n*diam, not n*E") {
    // A dense graph with small diameter: layered BFS must stay near 4E + 2n(diam+2).
    const Topology t = Topology::random_connected(200, 4000, 5);
    const int n = t.size();
    const int diam = diameter(t);
    std::vector<LayeredBfs> procs;
    for (int v = 0; v < n; ++v) {
        procs.emplace_back(v == 0);
    }
    AsyncNetwork<LayeredBfs> net(t, std::move(procs), random_delay(4242, 20));
    const auto stats = net.run();
    REQUIRE(net.process(0).root_done());
    const std::int64_t e = t.edge_count();
    CHECK_LE(stats.messages, 4 * e + 2 * static_cast<std::int64_t>(n) * (diam + 2));
    CHECK_LT(stats.messages, 8 * e);
}
