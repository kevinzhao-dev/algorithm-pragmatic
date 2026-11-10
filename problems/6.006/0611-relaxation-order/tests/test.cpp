#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t INF = kUnreachable;

struct WEdge {
    int u;
    int v;
    std::int64_t w;
};

using Order = std::vector<RelaxedEdge>;

WeightedGraph make_graph(int n, const std::vector<WEdge>& edges) {
    WeightedGraph g(idx(n));
    for (const WEdge& e : edges) {
        g[idx(e.u)].push_back({e.v, e.w});
    }
    return g;
}

// The sample graph used by the hand-worked tests; deliberately not the one from the course.
const std::vector<WEdge>& sample_edges() {
    static const std::vector<WEdge> edges = {{0, 1, 2}, {0, 3, 7}, {1, 2, 1}, {1, 4, 3},
                                             {3, 1, 1}, {3, 4, 1}, {2, 5, 4}, {4, 5, 1}};
    return edges;
}

// Only forward edges along a shuffled order, so the graph is acyclic.
std::vector<WEdge> random_dag(Rng& rng, int n, int m, std::int64_t low, std::int64_t high) {
    const std::vector<int> label = rng.permutation(idx(n));
    std::vector<WEdge> edges;
    for (int i = 0; i < m && n >= 2; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a < b) {
            edges.push_back({label[idx(a)], label[idx(b)], rng.uniform(low, high)});
        }
    }
    return edges;
}

std::vector<WEdge> random_general(Rng& rng, int n, int m, std::int64_t high) {
    std::vector<WEdge> edges;
    for (int i = 0; i < m; ++i) {
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)), static_cast<int>(rng.uniform(0, n - 1)),
                         rng.coin(0.3) ? 0 : rng.uniform(0, high)});
    }
    return edges;
}

std::vector<std::vector<int>> ascending_heads(const WeightedGraph& g) {
    std::vector<std::vector<int>> heads(g.size());
    for (std::size_t u = 0; u < g.size(); ++u) {
        for (const Arc& a : g[u]) {
            heads[u].push_back(a.to);
        }
        std::sort(heads[u].begin(), heads[u].end());
    }
    return heads;
}

std::vector<char> reachable(const WeightedGraph& g, int source) {
    std::vector<char> seen(g.size(), 0);
    std::vector<int> stack{source};
    seen[idx(source)] = 1;
    while (!stack.empty()) {
        const int u = stack.back();
        stack.pop_back();
        for (const Arc& a : g[idx(u)]) {
            if (seen[idx(a.to)] == 0) {
                seen[idx(a.to)] = 1;
                stack.push_back(a.to);
            }
        }
    }
    return seen;
}

// Quadratic model of the DAG rule: repeatedly take the smallest ready reachable vertex.
RelaxationTrace model_dag(const WeightedGraph& g, int source) {
    const auto reach = reachable(g, source);
    const auto heads = ascending_heads(g);
    RelaxationTrace trace{{}, std::vector<std::int64_t>(g.size(), INF)};
    trace.dist[idx(source)] = 0;
    std::vector<char> done(g.size(), 0);
    const int n = static_cast<int>(g.size());
    for (;;) {
        int next = -1;
        for (int v = 0; v < n && next == -1; ++v) {
            if (reach[idx(v)] == 0 || done[idx(v)] != 0) {
                continue;
            }
            bool ready = true;
            for (int u = 0; u < n && ready; ++u) {
                if (reach[idx(u)] == 0 || done[idx(u)] != 0) {
                    continue;
                }
                for (const Arc& a : g[idx(u)]) {
                    ready = ready && a.to != v;
                }
            }
            if (ready) {
                next = v;
            }
        }
        if (next == -1) {
            return trace;
        }
        done[idx(next)] = 1;
        for (const int head : heads[idx(next)]) {
            trace.order.emplace_back(next, head);
        }
        for (const Arc& a : g[idx(next)]) {
            trace.dist[idx(a.to)] = std::min(trace.dist[idx(a.to)], trace.dist[idx(next)] + a.weight);
        }
    }
}

// Quadratic model of Dijkstra with the specified tie-break.
RelaxationTrace model_dijkstra(const WeightedGraph& g, int source) {
    const auto heads = ascending_heads(g);
    RelaxationTrace trace{{}, std::vector<std::int64_t>(g.size(), INF)};
    trace.dist[idx(source)] = 0;
    std::vector<char> settled(g.size(), 0);
    for (;;) {
        int next = -1;
        for (int v = 0; v < static_cast<int>(g.size()); ++v) {
            if (settled[idx(v)] == 0 && trace.dist[idx(v)] != INF &&
                (next == -1 || trace.dist[idx(v)] < trace.dist[idx(next)])) {
                next = v;
            }
        }
        if (next == -1) {
            return trace;
        }
        settled[idx(next)] = 1;
        for (const int head : heads[idx(next)]) {
            trace.order.emplace_back(next, head);
        }
        for (const Arc& a : g[idx(next)]) {
            trace.dist[idx(a.to)] = std::min(trace.dist[idx(a.to)], trace.dist[idx(next)] + a.weight);
        }
    }
}

// Exhaustively explores every tie-break that Dijkstra could make while matching `order`.
bool brute_is_legal(const WeightedGraph& g, const Order& order, std::size_t position,
                    std::vector<char>& settled, std::vector<std::int64_t>& dist) {
    const int n = static_cast<int>(g.size());
    std::int64_t minimum = INF;
    bool any = false;
    for (int v = 0; v < n; ++v) {
        if (settled[idx(v)] == 0 && !g[idx(v)].empty() && dist[idx(v)] != INF) {
            minimum = std::min(minimum, dist[idx(v)]);
            any = true;
        }
    }
    if (!any) {
        return position == order.size();
    }
    for (int u = 0; u < n; ++u) {
        if (settled[idx(u)] != 0 || g[idx(u)].empty() || dist[idx(u)] != minimum) {
            continue;
        }
        const std::size_t degree = g[idx(u)].size();
        if (position + degree > order.size()) {
            continue;
        }
        std::vector<int> group;
        bool valid = true;
        for (std::size_t i = position; i < position + degree; ++i) {
            valid = valid && order[i].first == u;
            group.push_back(order[i].second);
        }
        if (!valid) {
            continue;
        }
        std::sort(group.begin(), group.end());
        std::vector<int> heads;
        for (const Arc& a : g[idx(u)]) {
            heads.push_back(a.to);
        }
        std::sort(heads.begin(), heads.end());
        if (group != heads) {
            continue;
        }
        settled[idx(u)] = 1;
        std::vector<std::int64_t> relaxed = dist;
        for (const Arc& a : g[idx(u)]) {
            relaxed[idx(a.to)] = std::min(relaxed[idx(a.to)], relaxed[idx(u)] + a.weight);
        }
        if (brute_is_legal(g, order, position + degree, settled, relaxed)) {
            settled[idx(u)] = 0;
            return true;
        }
        settled[idx(u)] = 0;
    }
    return false;
}

bool brute_legal(const WeightedGraph& g, int source, const Order& order) {
    std::vector<char> settled(g.size(), 0);
    std::vector<std::int64_t> dist(g.size(), INF);
    dist[idx(source)] = 0;
    return brute_is_legal(g, order, 0, settled, dist);
}

// A random run of Dijkstra with random tie-breaks, random adjacency order within a vertex.
Order random_legal_order(Rng& rng, const WeightedGraph& g, int source) {
    Order order;
    std::vector<char> settled(g.size(), 0);
    std::vector<std::int64_t> dist(g.size(), INF);
    dist[idx(source)] = 0;
    for (;;) {
        std::vector<int> minimal;
        std::int64_t minimum = INF;
        for (int v = 0; v < static_cast<int>(g.size()); ++v) {
            if (settled[idx(v)] == 0 && !g[idx(v)].empty() && dist[idx(v)] != INF) {
                if (dist[idx(v)] < minimum) {
                    minimum = dist[idx(v)];
                    minimal.clear();
                }
                if (dist[idx(v)] == minimum) {
                    minimal.push_back(v);
                }
            }
        }
        if (minimal.empty()) {
            return order;
        }
        const int u = minimal[idx(static_cast<int>(rng.uniform(0, static_cast<std::int64_t>(minimal.size()) - 1)))];
        settled[idx(u)] = 1;
        std::vector<int> heads;
        for (const Arc& a : g[idx(u)]) {
            heads.push_back(a.to);
        }
        rng.shuffle(heads);
        for (const int h : heads) {
            order.emplace_back(u, h);
        }
        for (const Arc& a : g[idx(u)]) {
            dist[idx(a.to)] = std::min(dist[idx(a.to)], minimum + a.weight);
        }
    }
}

}  // namespace

TEST(1, "the sample DAG is relaxed in topological order") {
    const WeightedGraph g = make_graph(6, sample_edges());
    const RelaxationTrace trace = dag_relaxation_trace(g, 0);
    CHECK_EQ(trace.order, (Order{{0, 1}, {0, 3}, {3, 1}, {3, 4}, {1, 2}, {1, 4}, {2, 5}, {4, 5}}));
    CHECK_EQ(trace.dist, std::vector<std::int64_t>{0, 2, 3, 7, 5, 6});
}

TEST(1, "a source with no outgoing edges relaxes nothing") {
    const RelaxationTrace trace = dag_relaxation_trace(make_graph(2, {{1, 0, 5}}), 0);
    CHECK(trace.order.empty());
    CHECK_EQ(trace.dist, std::vector<std::int64_t>{0, INF});
}

TEST(1, "edges out of unreachable vertices are not tried") {
    const WeightedGraph g = make_graph(4, {{0, 1, 1}, {2, 3, 1}, {2, 0, 1}});
    CHECK_EQ(dag_relaxation_trace(g, 0).order, (Order{{0, 1}}));
}

TEST(1, "the order is decided on the reachable part only") {
    // Vertex 1 has a predecessor 3 that the source cannot reach; it must not block anything.
    const WeightedGraph g = make_graph(4, {{0, 2, 1}, {0, 1, 1}, {3, 1, 1}, {1, 2, 1}});
    CHECK_EQ(dag_relaxation_trace(g, 0).order, (Order{{0, 1}, {0, 2}, {1, 2}}));
}

TEST(1, "negative weights and parallel edges are handled") {
    const WeightedGraph g = make_graph(3, {{0, 1, 5}, {0, 1, 2}, {1, 2, -7}});
    const auto trace = dag_relaxation_trace(g, 0);
    CHECK_EQ(trace.dist, std::vector<std::int64_t>{0, 2, -5});
    CHECK_EQ(trace.order, (Order{{0, 1}, {0, 1}, {1, 2}}));
}

TEST(1, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(dag_relaxation_trace(WeightedGraph(1), 1), std::out_of_range);
}

TEST(1, "random DAGs match a quadratic model") {
    Rng rng;
    for (int round = 0; round < 1200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 16));
        const auto edges = random_dag(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), -6, 12);
        const WeightedGraph g = make_graph(n, edges);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        const auto expected = model_dag(g, s);
        const auto got = dag_relaxation_trace(g, s);
        REQUIRE_EQ(got.order, expected.order);
        REQUIRE_EQ(got.dist, expected.dist);
    }
}

PERF(1, "smallest-first topological order at scale", 1500) {
    Rng rng;
    const int n = 300'000;
    WeightedGraph g(idx(n));
    for (int v = 1; v < n; ++v) {
        g[idx(static_cast<int>(rng.uniform(0, v - 1)))].push_back({v, 1});
    }
    for (int i = 0; i < 900'000; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a < b) {
            g[idx(a)].push_back({b, 1});
        }
    }
    CHECK_GT(dag_relaxation_trace(g, 0).order.size(), n - 1);
}

TEST(2, "the sample graph is settled by distance") {
    const WeightedGraph g = make_graph(6, sample_edges());
    const RelaxationTrace trace = dijkstra_trace(g, 0);
    CHECK_EQ(trace.order, (Order{{0, 1}, {0, 3}, {1, 2}, {1, 4}, {2, 5}, {4, 5}, {3, 1}, {3, 4}}));
    CHECK_EQ(trace.dist, std::vector<std::int64_t>{0, 2, 3, 7, 5, 6});
}

TEST(2, "ties break towards the smaller id") {
    const WeightedGraph g = make_graph(4, {{0, 2, 1}, {0, 1, 1}, {2, 3, 1}, {1, 3, 1}});
    CHECK_EQ(dijkstra_trace(g, 0).order, (Order{{0, 1}, {0, 2}, {1, 3}, {2, 3}}));
}

TEST(2, "edges into already settled vertices still count as attempts") {
    const WeightedGraph g = make_graph(2, {{0, 1, 1}, {1, 0, 1}});
    CHECK_EQ(dijkstra_trace(g, 0).order, (Order{{0, 1}, {1, 0}}));
}

TEST(2, "cycles and self-loops terminate") {
    const WeightedGraph g = make_graph(2, {{0, 0, 0}, {0, 1, 3}, {1, 1, 2}});
    const auto trace = dijkstra_trace(g, 0);
    CHECK_EQ(trace.order, (Order{{0, 0}, {0, 1}, {1, 1}}));
    CHECK_EQ(trace.dist, std::vector<std::int64_t>{0, 3});
}

TEST(2, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(dijkstra_trace(WeightedGraph(1), -1), std::out_of_range);
}

TEST(2, "random graphs match a quadratic model") {
    Rng rng;
    for (int round = 0; round < 1200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 16));
        const auto edges = random_general(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), 12);
        const WeightedGraph g = make_graph(n, edges);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        const auto expected = model_dijkstra(g, s);
        const auto got = dijkstra_trace(g, s);
        REQUIRE_EQ(got.order, expected.order);
        REQUIRE_EQ(got.dist, expected.dist);
    }
}

PERF(2, "dijkstra trace at scale", 2000) {
    Rng rng;
    const int n = 300'000;
    WeightedGraph g(idx(n));
    for (int v = 1; v < n; ++v) {
        g[idx(static_cast<int>(rng.uniform(0, v - 1)))].push_back({v, rng.uniform(1, 50)});
    }
    for (int i = 0; i < 900'000; ++i) {
        g[idx(static_cast<int>(rng.uniform(0, n - 1)))].push_back(
            {static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(1, 50)});
    }
    CHECK_GE(dijkstra_trace(g, 0).order.size(), n - 1);
}

TEST(3, "the actual trace is legal") {
    const WeightedGraph g = make_graph(6, sample_edges());
    CHECK(is_legal_dijkstra_order(g, 0, dijkstra_trace(g, 0).order));
}

TEST(3, "the DAG relaxation order is not a legal Dijkstra order") {
    const WeightedGraph g = make_graph(6, sample_edges());
    CHECK(!is_legal_dijkstra_order(g, 0, dag_relaxation_trace(g, 0).order));
}

TEST(3, "either vertex may go first on a tie") {
    const WeightedGraph g = make_graph(4, {{0, 1, 1}, {0, 2, 1}, {1, 3, 1}, {2, 3, 1}});
    const Order first = {{0, 1}, {0, 2}, {1, 3}, {2, 3}};
    const Order second = {{0, 2}, {0, 1}, {2, 3}, {1, 3}};
    CHECK(is_legal_dijkstra_order(g, 0, first));
    CHECK(is_legal_dijkstra_order(g, 0, second));
}

TEST(3, "a zero-weight edge must still be relaxed before its head is extracted") {
    // 1 reaches 2 for free, so 1 must go first even though both finish at distance 0.
    const WeightedGraph g = make_graph(3, {{0, 1, 0}, {0, 2, 1}, {1, 2, 0}, {2, 0, 5}});
    const Order correct = {{0, 1}, {0, 2}, {1, 2}, {2, 0}};
    const Order wrong = {{0, 1}, {0, 2}, {2, 0}, {1, 2}};
    CHECK(is_legal_dijkstra_order(g, 0, correct));
    CHECK(!is_legal_dijkstra_order(g, 0, wrong));
}

TEST(3, "a split group is illegal") {
    const WeightedGraph g = make_graph(3, {{0, 1, 1}, {0, 2, 1}, {1, 0, 1}});
    const Order order = {{0, 1}, {1, 0}, {0, 2}};
    CHECK(!is_legal_dijkstra_order(g, 0, order));
}

TEST(3, "missing and repeated edges are illegal") {
    const WeightedGraph g = make_graph(3, {{0, 1, 1}, {0, 2, 1}, {1, 2, 1}});
    const Order missing = {{0, 1}, {0, 2}};
    const Order repeated = {{0, 1}, {0, 2}, {1, 2}, {1, 2}};
    const Order extra_tail = {{0, 1}, {0, 2}, {1, 2}, {2, 1}};
    CHECK(!is_legal_dijkstra_order(g, 0, missing));
    CHECK(!is_legal_dijkstra_order(g, 0, repeated));
    CHECK(!is_legal_dijkstra_order(g, 0, extra_tail));
}

TEST(3, "edges from unreachable vertices or unknown ids are illegal") {
    const WeightedGraph g = make_graph(3, {{0, 1, 1}, {2, 1, 1}});
    const Order unreachable_tail = {{2, 1}, {0, 1}};
    CHECK(!is_legal_dijkstra_order(g, 0, unreachable_tail));
    const Order unknown = {{7, 1}};
    CHECK(!is_legal_dijkstra_order(g, 0, unknown));
    const Order correct = {{0, 1}};
    CHECK(is_legal_dijkstra_order(g, 0, correct));
}

TEST(3, "a source with no outgoing edges has an empty legal order") {
    const WeightedGraph g = make_graph(2, {{1, 0, 1}});
    CHECK(is_legal_dijkstra_order(g, 0, {}));
    const Order extra = {{1, 0}};
    CHECK(!is_legal_dijkstra_order(g, 0, extra));
}

TEST(3, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(is_legal_dijkstra_order(WeightedGraph(1), 2, {}), std::out_of_range);
}

TEST(3, "random legal orders and mutations match an exhaustive tie-break search") {
    Rng rng;
    int legal = 0;
    int illegal = 0;
    for (int round = 0; round < 3000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        const auto edges = random_general(rng, n, static_cast<int>(rng.uniform(0, 2 * n)), 4);
        const WeightedGraph g = make_graph(n, edges);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        Order order = random_legal_order(rng, g, s);
        INFO("round ", round, " n=", n, " s=", s, " order=", order);
        if (rng.coin() && !order.empty()) {
            const std::size_t i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(order.size()) - 1));
            switch (rng.uniform(0, 3)) {
                case 0:
                    order.erase(order.begin() + static_cast<long>(i));
                    break;
                case 1: {
                    const RelaxedEdge copy = order[i];
                    order.insert(order.begin() + static_cast<long>(i), copy);
                    break;
                }
                case 2:
                    order[i].second = static_cast<int>(rng.uniform(0, n - 1));
                    break;
                default:
                    if (i + 1 < order.size()) {
                        std::swap(order[i], order[i + 1]);
                    }
                    break;
            }
        }
        const bool expected = brute_legal(g, s, order);
        REQUIRE_EQ(is_legal_dijkstra_order(g, s, order), expected);
        (expected ? legal : illegal) += 1;
    }
    CHECK_GT(legal, 300);
    CHECK_GT(illegal, 300);
}

PERF(3, "checking a long order is near-linear", 2500) {
    Rng rng;
    const int n = 300'000;
    WeightedGraph g(idx(n));
    for (int v = 1; v < n; ++v) {
        g[idx(static_cast<int>(rng.uniform(0, v - 1)))].push_back({v, rng.uniform(1, 50)});
    }
    for (int i = 0; i < 700'000; ++i) {
        g[idx(static_cast<int>(rng.uniform(0, n - 1)))].push_back(
            {static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(1, 50)});
    }
    const auto trace = dijkstra_trace(g, 0);
    CHECK(is_legal_dijkstra_order(g, 0, trace.order));
}
