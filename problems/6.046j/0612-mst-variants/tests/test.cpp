#include <algorithm>
#include <functional>
#include <numeric>
#include <optional>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Kruskal reference; ties by id, so with distinct weights the MST is exact.
std::vector<int> oracle_mst(int n, const std::vector<Edge>& edges) {
    std::vector<int> order(edges.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return edges[a].w < edges[b].w; });
    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);
    const std::function<int(int)> root = [&](int x) {
        return parent[x] == x ? x : parent[x] = root(parent[x]);
    };
    std::vector<int> out;
    for (int id : order) {
        const int a = root(edges[id].u);
        const int b = root(edges[id].v);
        if (a != b) {
            parent[a] = b;
            out.push_back(id);
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

long long weight_of(const std::vector<Edge>& edges, const std::vector<int>& ids) {
    long long total = 0;
    for (int id : ids) {
        total += edges[id].w;
    }
    return total;
}

bool spanning_tree(int n, const std::vector<Edge>& edges, const std::vector<int>& ids) {
    if (static_cast<int>(ids.size()) != n - 1) {
        return false;
    }
    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);
    const std::function<int(int)> root = [&](int x) {
        return parent[x] == x ? x : parent[x] = root(parent[x]);
    };
    for (int id : ids) {
        const int a = root(edges[id].u);
        const int b = root(edges[id].v);
        if (a == b) {
            return false;
        }
        parent[a] = b;
    }
    return true;
}

// Connected random graph with distinct weights.
std::vector<Edge> random_connected(Rng& rng, int n, int extra) {
    std::vector<Edge> edges;
    for (int v = 1; v < n; ++v) {
        edges.push_back({v, static_cast<int>(rng.uniform(0, v - 1)), 0});
    }
    for (int i = 0; i < extra; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (v == u) {
            v = (v + 1) % n;
        }
        edges.push_back({u, v, 0});
    }
    auto weights = rng.permutation(edges.size());
    for (std::size_t i = 0; i < edges.size(); ++i) {
        edges[i].w = weights[i] + 1;
    }
    return edges;
}

// Number of minimum-weight spanning trees, by subset enumeration.
int count_msts(int n, const std::vector<Edge>& edges) {
    const int m = static_cast<int>(edges.size());
    long long best = std::numeric_limits<long long>::max();
    int count = 0;
    for (int mask = 0; mask < (1 << m); ++mask) {
        if (__builtin_popcount(mask) != n - 1) {
            continue;
        }
        std::vector<int> ids;
        for (int i = 0; i < m; ++i) {
            if (mask >> i & 1) {
                ids.push_back(i);
            }
        }
        if (!spanning_tree(n, edges, ids)) {
            continue;
        }
        const long long w = weight_of(edges, ids);
        if (w < best) {
            best = w;
            count = 1;
        } else if (w == best) {
            ++count;
        }
    }
    return count;
}

}  // namespace

TEST(1, "distinct weights always have a unique MST") {
    Rng rng;
    for (int round = 0; round < 80; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 8));
        const auto edges = random_connected(rng, n, static_cast<int>(rng.uniform(0, 8)));
        CHECK(has_unique_mst(n, edges));
    }
}

TEST(1, "a triangle with equal weights is not unique") {
    const std::vector<Edge> edges{{0, 1, 1}, {1, 2, 1}, {0, 2, 1}};
    CHECK(!has_unique_mst(3, edges));
}

TEST(1, "equal weights across components can still be unique") {
    // Two separate equal-weight edges, both forced.
    const std::vector<Edge> edges{{0, 1, 5}, {2, 3, 5}, {1, 2, 1}};
    CHECK(has_unique_mst(4, edges));
}

TEST(1, "parallel equal edges are interchangeable") {
    const std::vector<Edge> edges{{0, 1, 2}, {0, 1, 2}};
    CHECK(!has_unique_mst(2, edges));
}

TEST(1, "random graphs with ties match enumeration") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 6));
        auto edges = random_connected(rng, n, static_cast<int>(rng.uniform(0, 6)));
        for (auto& e : edges) {
            e.w = rng.uniform(1, 3);
        }
        INFO("round ", round);
        CHECK_EQ(has_unique_mst(n, edges), count_msts(n, edges) == 1);
    }
}

TEST(2, "two vertices, one phase") {
    const std::vector<Edge> edges{{0, 1, 3}};
    const auto result = boruvka(2, edges);
    CHECK_EQ(result.edges, std::vector<int>{0});
    CHECK_EQ(result.phases, 1);
}

TEST(2, "a star merges everything in one phase") {
    std::vector<Edge> edges;
    for (int v = 1; v < 16; ++v) {
        edges.push_back({0, v, v});
    }
    const auto result = boruvka(16, edges);
    CHECK_EQ(result.phases, 1);
    CHECK_EQ(result.edges.size(), 15);
}

TEST(2, "random graphs match the unique MST within lg n phases") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 300));
        const auto edges = random_connected(rng, n, static_cast<int>(rng.uniform(0, 800)));
        const auto result = boruvka(n, edges);
        INFO("round ", round);
        CHECK_EQ(result.edges, oracle_mst(n, edges));
        int lg = 0;
        while ((1 << lg) < n) {
            ++lg;
        }
        CHECK_LE(result.phases, lg);
    }
}

PERF(2, "borůvka on a large graph", 1500) {
    Rng rng;
    const int n = 200'000;
    const auto edges = random_connected(rng, n, 800'000);
    const auto result = boruvka(n, edges);
    CHECK_EQ(result.edges.size(), n - 1);
}

TEST(3, "the algorithm as written splits at the vertex midpoint") {
    // On a path graph the recursion happens to be correct.
    std::vector<Edge> edges;
    for (int v = 1; v < 16; ++v) {
        edges.push_back({v - 1, v, 100 - v});
    }
    const auto result = divide_and_conquer_mst(16, edges);
    std::vector<int> all(15);
    std::iota(all.begin(), all.end(), 0);
    CHECK_EQ(result, all);
}

TEST(3, "a broken recursion matches a test-local implementation") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int nn = static_cast<int>(rng.uniform(2, 40));
        const auto edges = random_connected(rng, nn, static_cast<int>(rng.uniform(0, 60)));
        const std::function<std::vector<int>(int, int, const std::vector<int>&)> local =
            [&](int lo, int hi, const std::vector<int>& ids) -> std::vector<int> {
            if (hi - lo <= 1) {
                return {};
            }
            const int mid = lo + (hi - lo) / 2;
            std::vector<int> left, right;
            int cross = -1;
            for (int id : ids) {
                const bool a = edges[id].u < mid;
                const bool b = edges[id].v < mid;
                if (a && b) {
                    left.push_back(id);
                } else if (!a && !b) {
                    right.push_back(id);
                } else if (cross == -1 || edges[id].w < edges[cross].w) {
                    cross = id;
                }
            }
            auto l = local(lo, mid, left);
            auto r = local(mid, hi, right);
            l.insert(l.end(), r.begin(), r.end());
            if (cross != -1) {
                l.push_back(cross);
            }
            return l;
        };
        std::vector<int> all(edges.size());
        std::iota(all.begin(), all.end(), 0);
        auto expected = local(0, nn, all);
        std::sort(expected.begin(), expected.end());
        INFO("round ", round);
        CHECK_EQ(divide_and_conquer_mst(nn, edges), expected);
    }
}

TEST(3, "the trap really fools divide and conquer") {
    const DcTrap trap = divide_and_conquer_trap();
    REQUIRE(trap.n >= 2);
    // Distinct weights, connected, and both halves connected on their own.
    auto weights = trap.edges;
    std::sort(weights.begin(), weights.end(),
              [](const Edge& a, const Edge& b) { return a.w < b.w; });
    for (std::size_t i = 0; i + 1 < weights.size(); ++i) {
        REQUIRE(weights[i].w != weights[i + 1].w);
    }
    std::vector<int> all(trap.edges.size());
    std::iota(all.begin(), all.end(), 0);
    REQUIRE(spanning_tree(trap.n, trap.edges, oracle_mst(trap.n, trap.edges)));

    const int mid = trap.n / 2;
    for (const auto& [lo, hi] : std::vector<std::pair<int, int>>{{0, mid}, {mid, trap.n}}) {
        std::vector<Edge> inside;
        for (const auto& e : trap.edges) {
            if (lo <= e.u && e.u < hi && lo <= e.v && e.v < hi) {
                inside.push_back({e.u - lo, e.v - lo, e.w});
            }
        }
        REQUIRE(hi - lo == 1 || spanning_tree(hi - lo, inside, oracle_mst(hi - lo, inside)));
    }

    const auto got = divide_and_conquer_mst(trap.n, trap.edges);
    REQUIRE(spanning_tree(trap.n, trap.edges, got));
    CHECK_GT(weight_of(trap.edges, got), weight_of(trap.edges, oracle_mst(trap.n, trap.edges)));
}

TEST(4, "a graph that is already a tree loses nothing") {
    const std::vector<Edge> edges{{0, 1, 3}, {1, 2, 1}};
    CHECK_EQ(cycle_breaking_mst(3, edges), std::vector<int>{0, 1});
}

TEST(4, "parallel edges form a cycle of length two") {
    const std::vector<Edge> edges{{0, 1, 9}, {0, 1, 2}};
    CHECK_EQ(cycle_breaking_mst(2, edges), std::vector<int>{1});
}

TEST(4, "random graphs converge to the unique MST") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 120));
        const auto edges = random_connected(rng, n, static_cast<int>(rng.uniform(0, 400)));
        INFO("round ", round);
        CHECK_EQ(cycle_breaking_mst(n, edges), oracle_mst(n, edges));
    }
}

PERF(4, "cycle breaking on thousands of edges", 1500) {
    Rng rng;
    const auto edges = random_connected(rng, 2000, 8000);
    CHECK_EQ(cycle_breaking_mst(2000, edges).size(), 1999);
}

TEST(5, "edges joining components are kept") {
    IncrementalMst forest(3);
    CHECK_EQ(forest.add_edge(0, 1, 5), std::optional<int>{});
    CHECK_EQ(forest.add_edge(1, 2, 7), std::optional<int>{});
    CHECK_EQ(forest.weight(), 12);
    CHECK_EQ(forest.edges(), std::vector<int>{0, 1});
}

TEST(5, "a light chord evicts the heaviest edge on the cycle") {
    IncrementalMst forest(3);
    forest.add_edge(0, 1, 5);
    forest.add_edge(1, 2, 7);
    CHECK_EQ(forest.add_edge(0, 2, 1), std::optional<int>{1});
    CHECK_EQ(forest.weight(), 6);
    CHECK_EQ(forest.edges(), std::vector<int>{0, 2});
}

TEST(5, "a heavy chord is rejected and reports itself") {
    IncrementalMst forest(3);
    forest.add_edge(0, 1, 5);
    forest.add_edge(1, 2, 7);
    CHECK_EQ(forest.add_edge(0, 2, 100), std::optional<int>{2});
    CHECK_EQ(forest.weight(), 12);
}

TEST(5, "every prefix matches a fresh Kruskal run") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 40));
        IncrementalMst forest(n);
        std::vector<Edge> edges;
        auto weights = rng.permutation(n * 6);
        std::size_t previous_size = 0;
        for (int step = 0; step < n * 6; ++step) {
            const int u = static_cast<int>(rng.uniform(0, n - 1));
            int v = static_cast<int>(rng.uniform(0, n - 1));
            if (u == v) {
                v = (v + 1) % n;
            }
            const long long w = weights[step] + 1;
            edges.push_back({u, v, w});
            const auto evicted = forest.add_edge(u, v, w);
            const auto forest_edges = forest.edges();
            INFO("round ", round, " step ", step);
            std::vector<int> order(edges.size());
            std::iota(order.begin(), order.end(), 0);
            std::sort(order.begin(), order.end(),
                      [&](int a, int b) { return edges[a].w < edges[b].w; });
            std::vector<int> parent(n);
            std::iota(parent.begin(), parent.end(), 0);
            const std::function<int(int)> root = [&](int x) {
                return parent[x] == x ? x : parent[x] = root(parent[x]);
            };
            std::vector<int> expected;
            for (int id : order) {
                const int a = root(edges[id].u);
                const int b = root(edges[id].v);
                if (a != b) {
                    parent[a] = b;
                    expected.push_back(id);
                }
            }
            std::sort(expected.begin(), expected.end());
            CHECK_EQ(forest_edges, expected);
            CHECK_EQ(forest.weight(), weight_of(edges, expected));
            CHECK_EQ(evicted.has_value(), forest_edges.size() == previous_size);
            if (evicted) {
                CHECK(*evicted >= 0 && *evicted <= step);
                CHECK(!std::binary_search(forest_edges.begin(), forest_edges.end(), *evicted));
            }
            previous_size = forest_edges.size();
        }
    }
}

PERF(5, "hundreds of thousands of insertions", 2500) {
    Rng rng;
    const int n = 2000;
    IncrementalMst forest(n);
    auto weights = rng.permutation(150'000);
    for (int step = 0; step < 150'000; ++step) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u == v) {
            v = (v + 1) % n;
        }
        forest.add_edge(u, v, weights[step] + 1);
    }
    CHECK_GE(forest.weight(), 0);
}
