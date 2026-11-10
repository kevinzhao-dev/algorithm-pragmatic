#include <algorithm>
#include <cstdlib>
#include <limits>
#include <queue>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct RawEdge {
    int u, v;
    long long cap;
};

struct Instance {
    int n;
    int s;
    int t;
    std::vector<RawEdge> edges;
};

Instance random_instance(Rng& rng, int n, int m, int max_cap) {
    Instance out{n, 0, n - 1, {}};
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u == v) {
            v = (v + 1) % n;
        }
        out.edges.push_back({u, v, rng.uniform(0, max_cap)});
    }
    return out;
}

DynamicMaxFlow build(const Instance& inst) {
    DynamicMaxFlow flow(inst.n, inst.s, inst.t);
    for (const auto& e : inst.edges) {
        flow.add_edge(e.u, e.v, e.cap);
    }
    return flow;
}

long long brute_min_cut(const Instance& inst) {
    long long best = std::numeric_limits<long long>::max();
    for (int mask = 0; mask < (1 << inst.n); ++mask) {
        if (!(mask >> inst.s & 1) || (mask >> inst.t & 1)) {
            continue;
        }
        long long cap = 0;
        for (const auto& e : inst.edges) {
            if ((mask >> e.u & 1) && !(mask >> e.v & 1)) {
                cap += e.cap;
            }
        }
        best = std::min(best, cap);
    }
    return best;
}

// Flow validity plus optimality: no augmenting path left in the residual graph.
void check_max_flow(const DynamicMaxFlow& g, const Instance& inst) {
    std::vector<long long> balance(inst.n, 0);
    std::vector<std::vector<std::pair<int, long long>>> residual(inst.n);
    for (int id = 0; id < static_cast<int>(inst.edges.size()); ++id) {
        const auto& e = inst.edges[id];
        const long long f = g.flow_on(id);
        REQUIRE(f >= 0);
        REQUIRE(f <= e.cap);
        CHECK_EQ(g.capacity_of(id), e.cap);
        balance[e.u] -= f;
        balance[e.v] += f;
        if (f < e.cap) {
            residual[e.u].emplace_back(e.v, e.cap - f);
        }
        if (f > 0) {
            residual[e.v].emplace_back(e.u, f);
        }
    }
    for (int v = 0; v < inst.n; ++v) {
        if (v != inst.s && v != inst.t) {
            CHECK_EQ(balance[v], 0);
        }
    }
    CHECK_EQ(balance[inst.t], g.value());
    std::vector<char> seen(inst.n, 0);
    std::queue<int> q;
    q.push(inst.s);
    seen[inst.s] = 1;
    while (!q.empty()) {
        const int v = q.front();
        q.pop();
        for (const auto& [to, cap] : residual[v]) {
            if (cap > 0 && !seen[to]) {
                seen[to] = 1;
                q.push(to);
            }
        }
    }
    CHECK(!seen[inst.t]);
}

}  // namespace

TEST(1, "initial maximum flow is valid") {
    Instance inst{4, 0, 3, {{0, 1, 2}, {1, 3, 2}, {0, 2, 3}, {2, 3, 1}}};
    auto g = build(inst);
    CHECK_EQ(g.compute(), 3);
    CHECK_EQ(g.value(), 3);
    check_max_flow(g, inst);
}

TEST(1, "no path gives zero flow") {
    Instance inst{3, 0, 2, {{0, 1, 5}}};
    auto g = build(inst);
    CHECK_EQ(g.compute(), 0);
    check_max_flow(g, inst);
}

TEST(1, "random networks are computed correctly") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 10));
        auto inst = random_instance(rng, n, static_cast<int>(rng.uniform(1, 30)), 9);
        auto g = build(inst);
        INFO("round ", round);
        CHECK_EQ(g.compute(), brute_min_cut(inst));
        check_max_flow(g, inst);
    }
}

TEST(2, "raising a bottleneck raises the flow") {
    Instance inst{3, 0, 2, {{0, 1, 2}, {1, 2, 5}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(0, 4), 4);
    inst.edges[0].cap = 4;
    check_max_flow(g, inst);
    CHECK_EQ(g.set_capacity(0, 9), 5);
    inst.edges[0].cap = 9;
    check_max_flow(g, inst);
}

TEST(2, "raising a slack edge changes nothing") {
    Instance inst{3, 0, 2, {{0, 1, 5}, {1, 2, 2}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(0, 8), 2);
    CHECK_EQ(g.value(), 2);
}

TEST(2, "random capacity increases") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 10));
        auto inst = random_instance(rng, n, static_cast<int>(rng.uniform(1, 30)), 6);
        auto g = build(inst);
        g.compute();
        for (int step = 0; step < 20; ++step) {
            const int id = static_cast<int>(rng.uniform(0, static_cast<int>(inst.edges.size()) - 1));
            const long long before = g.value();
            const long long k = rng.uniform(1, 5);
            const long long after = g.set_capacity(id, inst.edges[id].cap + k);
            inst.edges[id].cap += k;
            INFO("round ", round, " step ", step);
            CHECK_EQ(after, g.value());
            check_max_flow(g, inst);
            CHECK_LE(after - before, k);
            CHECK_GE(after, before);
            CHECK_EQ(after, brute_min_cut(inst));
        }
    }
}

TEST(3, "lowering a slack edge keeps the same flow") {
    Instance inst{3, 0, 2, {{0, 1, 5}, {1, 2, 2}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(0, 3), 2);
    inst.edges[0].cap = 3;
    check_max_flow(g, inst);
    CHECK_EQ(g.value(), 2);
}

TEST(3, "lowering down to exactly the current flow") {
    Instance inst{3, 0, 2, {{0, 1, 5}, {1, 2, 2}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(0, 2), 2);
    inst.edges[0].cap = 2;
    check_max_flow(g, inst);
}

TEST(4, "cutting a bottleneck reduces the flow") {
    Instance inst{3, 0, 2, {{0, 1, 5}, {1, 2, 5}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(0, 2), 2);
    inst.edges[0].cap = 2;
    check_max_flow(g, inst);
    CHECK_EQ(g.flow_on(0), 2);
}

TEST(4, "flow is rerouted when an alternative exists") {
    Instance inst{4, 0, 3, {{0, 1, 4}, {1, 3, 4}, {0, 2, 4}, {2, 3, 4}, {1, 2, 4}}};
    auto g = build(inst);
    CHECK_EQ(g.compute(), 8);
    CHECK_EQ(g.set_capacity(1, 0), 4);
    inst.edges[1].cap = 0;
    check_max_flow(g, inst);
    CHECK_EQ(g.flow_on(1), 0);
}

TEST(4, "cancelling through a chain reaches the source and the sink") {
    Instance inst{5, 0, 4, {{0, 1, 3}, {1, 2, 3}, {2, 3, 3}, {3, 4, 3}}};
    auto g = build(inst);
    CHECK_EQ(g.compute(), 3);
    CHECK_EQ(g.set_capacity(2, 1), 1);
    inst.edges[2].cap = 1;
    check_max_flow(g, inst);
    for (int id = 0; id < 4; ++id) {
        CHECK_LE(g.flow_on(id), 1);
    }
}

TEST(4, "lowering an edge leaving the source") {
    Instance inst{3, 0, 2, {{0, 1, 5}, {1, 2, 5}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(0, 0), 0);
    inst.edges[0].cap = 0;
    check_max_flow(g, inst);
}

TEST(4, "lowering an edge entering the sink") {
    Instance inst{3, 0, 2, {{0, 1, 5}, {1, 2, 5}}};
    auto g = build(inst);
    g.compute();
    CHECK_EQ(g.set_capacity(1, 3), 3);
    inst.edges[1].cap = 3;
    check_max_flow(g, inst);
}

TEST(4, "the sequence of random capacity changes stays maximum") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 9));
        auto inst = random_instance(rng, n, static_cast<int>(rng.uniform(1, 30)), 6);
        auto g = build(inst);
        g.compute();
        for (int step = 0; step < 25; ++step) {
            const int id = static_cast<int>(rng.uniform(0, static_cast<int>(inst.edges.size()) - 1));
            const long long old_cap = inst.edges[id].cap;
            const long long new_cap = rng.uniform(0, 8);
            const long long before = g.value();
            const long long after = g.set_capacity(id, new_cap);
            inst.edges[id].cap = new_cap;
            INFO("round ", round, " step ", step, " id ", id, " ", old_cap, "->", new_cap);
            CHECK_EQ(after, g.value());
            check_max_flow(g, inst);
            CHECK_LE(std::llabs(after - before), std::llabs(new_cap - old_cap));
            CHECK_EQ(after, brute_min_cut(inst));
        }
    }
}

PERF(4, "unit updates avoid a full recomputation", 3000) {
    Rng rng;
    const int n = 2000;
    auto inst = random_instance(rng, n, 16'000, 40);
    // Make the sink well connected so that the flow is large.
    for (int i = 1; i < n - 1; ++i) {
        if (i % 3 == 0) {
            inst.edges.push_back({i, n - 1, 25});
            inst.edges.push_back({0, i, 25});
        }
    }
    auto g = build(inst);
    CHECK_GT(g.compute(), 0);
    long long sum = 0;
    for (int step = 0; step < 12'000; ++step) {
        const int id = static_cast<int>(rng.uniform(0, static_cast<int>(inst.edges.size()) - 1));
        const long long delta = rng.coin() ? 1 : -1;
        const long long r = std::max<long long>(0, g.capacity_of(id) + delta);
        sum += g.set_capacity(id, r);
    }
    CHECK_GE(sum, 0);
}
