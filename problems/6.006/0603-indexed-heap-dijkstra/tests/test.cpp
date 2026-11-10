#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t INF = kUnreachable;

struct WEdge {
    int u;
    int v;
    std::int64_t w;
};

WeightedGraph make_graph(int n, const std::vector<WEdge>& edges) {
    WeightedGraph g(idx(n));
    for (const WEdge& e : edges) {
        g[idx(e.u)].push_back({e.v, e.w});
    }
    return g;
}

std::vector<WEdge> random_edges(Rng& rng, int n, int m, std::int64_t max_weight) {
    std::vector<WEdge> edges;
    for (int i = 0; i < m; ++i) {
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                         static_cast<int>(rng.uniform(0, n - 1)),
                         rng.coin(0.2) ? 0 : rng.uniform(0, max_weight)});
    }
    return edges;
}

std::vector<std::int64_t> brute_dist(int n, const std::vector<WEdge>& edges, int source) {
    std::vector<std::int64_t> dist(idx(n), INF);
    dist[idx(source)] = 0;
    for (int round = 0; round < n; ++round) {
        for (const WEdge& e : edges) {
            if (dist[idx(e.u)] != INF && dist[idx(e.u)] + e.w < dist[idx(e.v)]) {
                dist[idx(e.v)] = dist[idx(e.u)] + e.w;
            }
        }
    }
    return dist;
}

void check_parents(int n, const std::vector<WEdge>& edges, int source, const ShortestPaths& paths) {
    REQUIRE_EQ(paths.parent.size(), n);
    CHECK_EQ(paths.parent[idx(source)], kNoParent);
    for (int v = 0; v < n; ++v) {
        if (v == source || paths.dist[idx(v)] == INF) {
            continue;
        }
        const int p = paths.parent[idx(v)];
        INFO("vertex ", v, " parent ", p);
        REQUIRE(p >= 0 && p < n);
        bool tight = false;
        for (const WEdge& e : edges) {
            tight = tight || (e.u == p && e.v == v && paths.dist[idx(p)] != INF &&
                              paths.dist[idx(p)] + e.w == paths.dist[idx(v)]);
        }
        CHECK(tight);
        int at = v;
        for (int steps = 0; steps <= n && at != source; ++steps) {
            at = paths.parent[idx(at)];
            REQUIRE(at != kNoParent);
        }
        CHECK_EQ(at, source);
    }
}

}  // namespace

TEST(1, "a new heap is empty") {
    const IndexedMinHeap<int> heap(5);
    CHECK(heap.empty());
    CHECK_EQ(heap.size(), 0);
    CHECK(!heap.contains(4));
}

TEST(1, "a negative capacity is rejected") {
    CHECK_THROWS_AS(IndexedMinHeap<int>(-1), std::invalid_argument);
}

TEST(1, "pop_min returns keys in ascending order") {
    IndexedMinHeap<int> heap(4);
    heap.push(0, 30);
    heap.push(1, 10);
    heap.push(2, 40);
    heap.push(3, 20);
    CHECK_EQ(heap.size(), 4);
    CHECK_EQ(heap.pop_min(), (std::pair{1, 10}));
    CHECK_EQ(heap.pop_min(), (std::pair{3, 20}));
    CHECK_EQ(heap.pop_min(), (std::pair{0, 30}));
    CHECK_EQ(heap.pop_min(), (std::pair{2, 40}));
    CHECK(heap.empty());
}

TEST(1, "contains and key reflect the current state") {
    IndexedMinHeap<int> heap(3);
    heap.push(2, 7);
    CHECK(heap.contains(2));
    CHECK(!heap.contains(1));
    CHECK_EQ(heap.key(2), 7);
    CHECK_THROWS_AS(heap.key(1), std::out_of_range);
}

TEST(1, "bad ids and duplicate pushes throw") {
    IndexedMinHeap<int> heap(2);
    CHECK_THROWS_AS(heap.contains(2), std::out_of_range);
    CHECK_THROWS_AS(heap.contains(-1), std::out_of_range);
    CHECK_THROWS_AS(heap.push(5, 1), std::out_of_range);
    heap.push(0, 1);
    CHECK_THROWS_AS(heap.push(0, 2), std::invalid_argument);
    CHECK_THROWS_AS(heap.decrease_key(1, 3), std::out_of_range);
}

TEST(1, "popping an empty heap throws out_of_range") {
    IndexedMinHeap<int> heap(1);
    CHECK_THROWS_AS(heap.pop_min(), std::out_of_range);
    heap.push(0, 1);
    heap.pop_min();
    CHECK_THROWS_AS(heap.pop_min(), std::out_of_range);
}

TEST(1, "decrease_key moves an item to the front") {
    IndexedMinHeap<int> heap(3);
    heap.push(0, 10);
    heap.push(1, 20);
    heap.push(2, 30);
    CHECK(heap.decrease_key(2, 5));
    CHECK_EQ(heap.key(2), 5);
    CHECK_EQ(heap.pop_min(), (std::pair{2, 5}));
}

TEST(1, "decrease_key ignores keys that are not smaller") {
    IndexedMinHeap<int> heap(2);
    heap.push(0, 10);
    CHECK(!heap.decrease_key(0, 10));
    CHECK(!heap.decrease_key(0, 11));
    CHECK_EQ(heap.key(0), 10);
}

TEST(1, "an id can return after it was popped") {
    IndexedMinHeap<int> heap(2);
    heap.push(0, 1);
    heap.pop_min();
    CHECK(!heap.contains(0));
    heap.push(0, 5);
    CHECK_EQ(heap.pop_min(), (std::pair{0, 5}));
}

TEST(1, "clear empties the heap and leaves it reusable") {
    IndexedMinHeap<int> heap(3);
    heap.push(0, 1);
    heap.push(2, 2);
    heap.clear();
    CHECK(heap.empty());
    CHECK(!heap.contains(0));
    heap.push(0, 9);
    heap.push(1, 8);
    CHECK_EQ(heap.pop_min(), (std::pair{1, 8}));
}

TEST(1, "random operations match an ordered set") {
    Rng rng;
    const int capacity = 64;
    IndexedMinHeap<std::int64_t> heap(capacity);
    std::set<std::pair<std::int64_t, int>> model;
    std::map<int, std::int64_t> key_of;
    for (int step = 0; step < 60'000; ++step) {
        INFO("step ", step);
        const int op = static_cast<int>(rng.uniform(0, 3));
        const int id = static_cast<int>(rng.uniform(0, capacity - 1));
        const std::int64_t key = rng.uniform(0, 400);
        if (op == 0 && key_of.count(id) == 0) {
            heap.push(id, key);
            model.emplace(key, id);
            key_of[id] = key;
        } else if (op == 1 && !model.empty()) {
            const auto [popped_id, popped_key] = heap.pop_min();
            // Ties may break either way, but the key must be the smallest and belong to that id.
            REQUIRE_EQ(popped_key, model.begin()->first);
            REQUIRE(key_of.count(popped_id) != 0);
            REQUIRE_EQ(key_of[popped_id], popped_key);
            model.erase({popped_key, popped_id});
            key_of.erase(popped_id);
        } else if (op == 2 && key_of.count(id) != 0) {
            const bool expected = key < key_of[id];
            REQUIRE_EQ(heap.decrease_key(id, key), expected);
            if (expected) {
                model.erase({key_of[id], id});
                model.emplace(key, id);
                key_of[id] = key;
            }
        } else if (op == 3) {
            REQUIRE_EQ(heap.contains(id), key_of.count(id) != 0);
            if (key_of.count(id) != 0) {
                REQUIRE_EQ(heap.key(id), key_of[id]);
            }
        }
        REQUIRE_EQ(heap.size(), model.size());
    }
}

TEST(1, "operations cost O(log n) comparisons") {
    using Key = Counted<int>;
    const int n = 1 << 16;
    const int logn = 16;
    Key::comparisons() = 0;
    IndexedMinHeap<Key> heap(n);
    for (int i = 0; i < n; ++i) {
        heap.push(i, Key(n - i));
    }
    CHECK_LE(Key::comparisons(), static_cast<std::uint64_t>(n) * (logn + 2));

    Key::comparisons() = 0;
    for (int i = n - 1; i >= 0; i -= 2) {
        heap.decrease_key(i, Key(-i - 1));
    }
    CHECK_LE(Key::comparisons(), static_cast<std::uint64_t>(n / 2) * (logn + 3));

    Key::comparisons() = 0;
    while (!heap.empty()) {
        heap.pop_min();
    }
    CHECK_LE(Key::comparisons(), static_cast<std::uint64_t>(n) * (2 * logn + 2));
}

// Rewriting the whole position table on every clear costs ten trillion writes here.
PERF(1, "clear only pays for the items present", 1500) {
    IndexedMinHeap<int> heap(5'000'000);
    for (int round = 0; round < 2'000'000; ++round) {
        heap.push(round % 4'000'000, round);
        heap.push((round + 1) % 4'000'000, round);
        heap.clear();
    }
    CHECK(heap.empty());
}

PERF(1, "a million mixed operations", 2500) {
    Rng rng;
    const int n = 1'000'000;
    IndexedMinHeap<std::int64_t> heap(n);
    for (int i = 0; i < n; ++i) {
        heap.push(i, rng.uniform(0, 1'000'000'000));
    }
    for (int i = 0; i < n; ++i) {
        heap.decrease_key(static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(-1000, 500'000'000));
    }
    std::int64_t last = -2'000'000'000;
    for (int i = 0; i < n; ++i) {
        const auto [id, key] = heap.pop_min();
        CHECK_GE(key, last);
        last = key;
    }
}

TEST(2, "a single vertex graph") {
    const auto paths = dijkstra(WeightedGraph(1), 0);
    CHECK_EQ(paths.dist, std::vector<std::int64_t>{0});
    CHECK_EQ(paths.parent, std::vector<int>{kNoParent});
}

TEST(2, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(dijkstra(WeightedGraph(2), 2), std::out_of_range);
    CHECK_THROWS_AS(dijkstra(WeightedGraph(2), -1), std::out_of_range);
}

TEST(2, "a negative weight anywhere in the graph is rejected") {
    const WeightedGraph g = make_graph(3, {{0, 1, 1}, {2, 1, -1}});
    CHECK_THROWS_AS(dijkstra(g, 0), std::invalid_argument);
}

TEST(2, "the cheaper indirect route is chosen and parents point along it") {
    const std::vector<WEdge> edges = {{0, 1, 10}, {0, 2, 3}, {2, 1, 2}, {1, 3, 1}, {2, 3, 9}};
    const auto paths = dijkstra(make_graph(4, edges), 0);
    CHECK_EQ(paths.dist, std::vector<std::int64_t>{0, 5, 3, 6});
    check_parents(4, edges, 0, paths);
}

TEST(2, "unreachable vertices stay at kUnreachable with no parent") {
    const std::vector<WEdge> edges = {{0, 1, 1}};
    const auto paths = dijkstra(make_graph(3, edges), 0);
    CHECK_EQ(paths.dist, std::vector<std::int64_t>{0, 1, INF});
    CHECK_EQ(paths.parent[idx(2)], kNoParent);
}

TEST(2, "zero-weight edges and cycles are fine") {
    const std::vector<WEdge> edges = {{0, 1, 0}, {1, 2, 0}, {2, 1, 0}, {2, 0, 4}};
    const auto paths = dijkstra(make_graph(3, edges), 0);
    CHECK_EQ(paths.dist, std::vector<std::int64_t>{0, 0, 0});
    check_parents(3, edges, 0, paths);
}

TEST(2, "parallel edges use the lightest") {
    const std::vector<WEdge> edges = {{0, 1, 5}, {0, 1, 2}, {0, 1, 3}};
    const auto paths = dijkstra(make_graph(2, edges), 0);
    CHECK_EQ(paths.dist[1], 2);
}

TEST(2, "random graphs match repeated relaxation") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 18));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 4 * n)), 30);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        const auto paths = dijkstra(make_graph(n, edges), s);
        REQUIRE_EQ(paths.dist, brute_dist(n, edges, s));
        check_parents(n, edges, s, paths);
    }
}

// A linear scan for the minimum costs V^2 = 2.5e11 comparisons on this sparse graph.
PERF(2, "half a million sparse vertices", 2500) {
    Rng rng;
    const int n = 500'000;
    WeightedGraph g(idx(n));
    for (int v = 1; v < n; ++v) {
        const int u = static_cast<int>(rng.uniform(0, v - 1));
        g[idx(u)].push_back({v, rng.uniform(1, 1000)});
    }
    for (int i = 0; i < 1'500'000; ++i) {
        g[idx(static_cast<int>(rng.uniform(0, n - 1)))].push_back(
            {static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(1, 1000)});
    }
    const auto paths = dijkstra(g, 0);
    CHECK_EQ(paths.dist[0], 0);
}

TEST(3, "a query from a vertex to itself is distance zero") {
    RouteEngine engine(WeightedGraph(2));
    CHECK_EQ(engine.distance(1, 1), std::optional<std::int64_t>(0));
    CHECK_EQ(engine.route(1, 1), std::optional(std::vector<int>{1}));
}

TEST(3, "a negative weight is rejected at construction") {
    CHECK_THROWS_AS(RouteEngine(make_graph(2, {{0, 1, -3}})), std::invalid_argument);
}

TEST(3, "bad vertices throw out_of_range") {
    RouteEngine engine(WeightedGraph(2));
    CHECK_THROWS_AS(engine.distance(2, 0), std::out_of_range);
    CHECK_THROWS_AS(engine.route(0, -1), std::out_of_range);
}

TEST(3, "unreachable targets give nullopt") {
    RouteEngine engine(make_graph(3, {{0, 1, 1}}));
    CHECK_EQ(engine.distance(0, 2), std::optional<std::int64_t>());
    CHECK_EQ(engine.route(2, 0), std::optional<std::vector<int>>());
}

TEST(3, "a route is a sequence of real edges with the shortest total weight") {
    const std::vector<WEdge> edges = {{0, 1, 10}, {0, 2, 3}, {2, 1, 2}, {1, 3, 1}, {2, 3, 9}};
    RouteEngine engine(make_graph(4, edges));
    const auto route = engine.route(0, 3);
    REQUIRE(route.has_value());
    CHECK_EQ(*route, std::vector<int>{0, 2, 1, 3});
}

TEST(3, "earlier queries leave no state behind") {
    const std::vector<WEdge> edges = {{0, 1, 1}, {1, 2, 1}, {0, 2, 5}, {2, 0, 1}};
    RouteEngine engine(make_graph(3, edges));
    CHECK_EQ(engine.distance(0, 2), std::optional<std::int64_t>(2));
    CHECK_EQ(engine.distance(2, 1), std::optional<std::int64_t>(2));
    CHECK_EQ(engine.distance(1, 0), std::optional<std::int64_t>(2));
    CHECK_EQ(engine.distance(0, 2), std::optional<std::int64_t>(2));
    CHECK_EQ(engine.route(2, 1), std::optional(std::vector<int>{2, 0, 1}));
}

TEST(3, "random queries match a fresh Dijkstra") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 25));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 4 * n)), 20);
        const WeightedGraph g = make_graph(n, edges);
        RouteEngine engine(g);
        for (int query = 0; query < 60; ++query) {
            const int a = static_cast<int>(rng.uniform(0, n - 1));
            const int b = static_cast<int>(rng.uniform(0, n - 1));
            const auto expected = brute_dist(n, edges, a)[idx(b)];
            INFO("round ", round, " query ", query, " ", a, "->", b);
            const auto got = engine.distance(a, b);
            REQUIRE_EQ(got, expected == INF ? std::optional<std::int64_t>() : std::optional(expected));
            const auto route = engine.route(a, b);
            REQUIRE_EQ(route.has_value(), got.has_value());
            if (route) {
                REQUIRE_EQ(route->front(), a);
                REQUIRE_EQ(route->back(), b);
                std::int64_t total = 0;
                for (std::size_t i = 0; i + 1 < route->size(); ++i) {
                    std::int64_t best = INF;
                    for (const WEdge& e : edges) {
                        if (e.u == (*route)[i] && e.v == (*route)[i + 1]) {
                            best = std::min(best, e.w);
                        }
                    }
                    REQUIRE(best != INF);
                    total += best;
                }
                CHECK_EQ(total, *got);
            }
        }
    }
}

// Resetting one million entries per query costs a minute of memset across these queries.
PERF(3, "short queries on a huge grid do not pay for the whole graph", 3000) {
    Rng rng;
    const int side = 1000;
    const int n = side * side;
    WeightedGraph g(idx(n));
    const auto id = [&](int r, int c) { return r * side + c; };
    for (int r = 0; r < side; ++r) {
        for (int c = 0; c < side; ++c) {
            if (r + 1 < side) {
                g[idx(id(r, c))].push_back({id(r + 1, c), rng.uniform(1, 9)});
                g[idx(id(r + 1, c))].push_back({id(r, c), rng.uniform(1, 9)});
            }
            if (c + 1 < side) {
                g[idx(id(r, c))].push_back({id(r, c + 1), rng.uniform(1, 9)});
                g[idx(id(r, c + 1))].push_back({id(r, c), rng.uniform(1, 9)});
            }
        }
    }
    RouteEngine engine(std::move(g));
    std::int64_t total = 0;
    for (int query = 0; query < 150'000; ++query) {
        const int r = static_cast<int>(rng.uniform(0, side - 4));
        const int c = static_cast<int>(rng.uniform(0, side - 4));
        total += engine.distance(id(r, c), id(r + 2, c + 1)).value_or(0);
    }
    CHECK_GT(total, 0);
}
