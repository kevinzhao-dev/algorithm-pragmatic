#include <algorithm>
#include <functional>
#include <limits>
#include <numeric>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::vector<WeightedEdge> random_graph(Rng& rng, int n, int m, int max_w) {
    std::vector<WeightedEdge> edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u == v) {
            v = (v + 1) % n;
        }
        edges.push_back({u, v, rng.uniform(1, max_w)});
    }
    return edges;
}

// Local checker: the chosen edges form a forest that connects whatever `edges` connects.
void check_forest(int n, const std::vector<WeightedEdge>& edges, const SpanningForest& forest,
                  long long expected_total) {
    std::vector<int> parent(n);
    std::iota(parent.begin(), parent.end(), 0);
    const std::function<int(int)> root = [&](int x) {
        return parent[x] == x ? x : parent[x] = root(parent[x]);
    };
    long long total = 0;
    for (int id : forest.edges) {
        REQUIRE(id >= 0 && id < static_cast<int>(edges.size()));
        const int a = root(edges[id].u);
        const int b = root(edges[id].v);
        REQUIRE(a != b);  // no cycle
        parent[a] = b;
        total += edges[id].w;
    }
    CHECK_EQ(total, forest.total);
    CHECK_EQ(forest.total, expected_total);
    for (const auto& e : edges) {
        CHECK(root(e.u) == root(e.v));  // spanning: every graph edge stays within one component
    }
}

// Exhaustive minimum spanning forest over all edge subsets.
long long brute_mst(int n, const std::vector<WeightedEdge>& edges) {
    long long best = std::numeric_limits<long long>::max();
    const int m = static_cast<int>(edges.size());
    int max_edges = -1;
    for (int mask = 0; mask < (1 << m); ++mask) {
        std::vector<int> parent(n);
        std::iota(parent.begin(), parent.end(), 0);
        const std::function<int(int)> root = [&](int x) {
            return parent[x] == x ? x : parent[x] = root(parent[x]);
        };
        bool forest = true;
        long long total = 0;
        int taken = 0;
        for (int i = 0; i < m && forest; ++i) {
            if (mask >> i & 1) {
                const int a = root(edges[i].u);
                const int b = root(edges[i].v);
                forest = a != b;
                parent[a] = b;
                total += edges[i].w;
                ++taken;
            }
        }
        if (forest) {
            if (taken > max_edges) {
                max_edges = taken;
                best = total;
            } else if (taken == max_edges) {
                best = std::min(best, total);
            }
        }
    }
    return best;
}

}  // namespace

TEST(1, "no edges and isolated vertices") {
    const std::vector<WeightedEdge> none;
    const auto forest = kruskal(3, none);
    CHECK_EQ(forest.total, 0);
    CHECK(forest.edges.empty());
}

TEST(1, "picks the two cheap edges of a triangle") {
    const std::vector<WeightedEdge> edges{{0, 1, 5}, {1, 2, 1}, {0, 2, 2}};
    const auto forest = kruskal(3, edges);
    check_forest(3, edges, forest, 3);
}

TEST(1, "parallel edges use the cheaper one") {
    const std::vector<WeightedEdge> edges{{0, 1, 9}, {0, 1, 4}};
    const auto forest = kruskal(2, edges);
    check_forest(2, edges, forest, 4);
    CHECK_EQ(forest.edges, std::vector<int>{1});
}

TEST(1, "disconnected graphs give a forest") {
    const std::vector<WeightedEdge> edges{{0, 1, 3}, {2, 3, 7}};
    const auto forest = kruskal(5, edges);
    check_forest(5, edges, forest, 10);
    CHECK_EQ(forest.edges.size(), 2);
}

TEST(1, "random graphs match exhaustive search") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 7));
        const int m = static_cast<int>(rng.uniform(1, 10));
        const auto edges = random_graph(rng, n, m, 6);
        INFO("round ", round);
        check_forest(n, edges, kruskal(n, edges), brute_mst(n, edges));
    }
}

TEST(2, "prim on a disconnected graph covers every component") {
    const std::vector<WeightedEdge> edges{{0, 1, 3}, {2, 3, 7}, {2, 3, 1}};
    const auto forest = prim(4, edges);
    check_forest(4, edges, forest, 4);
}

TEST(2, "isolated vertices are skipped") {
    const auto forest = prim(3, std::vector<WeightedEdge>{});
    CHECK_EQ(forest.total, 0);
    CHECK(forest.edges.empty());
}

TEST(2, "prim matches kruskal on random graphs") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 40));
        const int m = static_cast<int>(rng.uniform(1, 200));
        const auto edges = random_graph(rng, n, m, 50);
        INFO("round ", round);
        const auto reference = kruskal(n, edges);
        check_forest(n, edges, prim(n, edges), reference.total);
    }
}

PERF(2, "prim on a large sparse graph", 2500) {
    Rng rng;
    const int n = 300'000;
    std::vector<WeightedEdge> edges;
    for (int i = 1; i < n; ++i) {
        edges.push_back({i, static_cast<int>(rng.uniform(0, i - 1)), rng.uniform(1, 1'000'000)});
    }
    for (int i = 0; i < 1'000'000; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u != v) {
            edges.push_back({u, v, rng.uniform(1, 1'000'000)});
        }
    }
    const auto forest = prim(n, edges);
    CHECK_EQ(forest.edges.size(), n - 1);
}

TEST(3, "k equal to n keeps singletons") {
    const std::vector<WeightedEdge> edges{{0, 1, 1}};
    CHECK_EQ(cluster(2, edges, 2), std::vector<int>{0, 1});
}

TEST(3, "k equal to one merges everything") {
    const std::vector<WeightedEdge> edges{{0, 1, 1}, {1, 2, 9}};
    CHECK_EQ(cluster(3, edges, 1), std::vector<int>{0, 0, 0});
}

TEST(3, "labels are renumbered by the smallest member") {
    const std::vector<WeightedEdge> edges{{2, 3, 1}, {0, 1, 1}, {1, 2, 100}};
    CHECK_EQ(cluster(4, edges, 2), std::vector<int>{0, 0, 1, 1});
}

TEST(3, "the partition maximizes spacing on small complete graphs") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 6));
        const int k = static_cast<int>(rng.uniform(1, n));
        std::vector<WeightedEdge> edges;
        std::vector<std::vector<long long>> w(n, std::vector<long long>(n, 0));
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                w[u][v] = w[v][u] = rng.uniform(1, 9);
                edges.push_back({u, v, w[u][v]});
            }
        }
        const auto spacing = [&](const std::vector<int>& label) {
            long long gap = std::numeric_limits<long long>::max();
            for (int u = 0; u < n; ++u) {
                for (int v = u + 1; v < n; ++v) {
                    if (label[u] != label[v]) {
                        gap = std::min(gap, w[u][v]);
                    }
                }
            }
            return gap;
        };
        const auto got = cluster(n, edges, k);
        REQUIRE_EQ(got.size(), n);
        long long best = std::numeric_limits<long long>::min();
        // Enumerate all assignments of n vertices into exactly k labeled groups.
        std::vector<int> label(n, 0);
        const std::function<void(int, int)> enumerate = [&](int v, int used) {
            if (v == n) {
                if (used == k) {
                    best = std::max(best, spacing(label));
                }
                return;
            }
            for (int l = 0; l <= std::min(used, k - 1); ++l) {
                label[v] = l;
                enumerate(v + 1, std::max(used, l + 1));
            }
        };
        enumerate(0, 0);
        INFO("round ", round, " k=", k);
        CHECK_EQ(spacing(got), best);
        // Exactly k groups, numbered by smallest member.
        int next = 0;
        std::vector<int> seen(k, -1);
        for (int v = 0; v < n; ++v) {
            REQUIRE(got[v] >= 0 && got[v] < k);
            if (seen[got[v]] == -1) {
                seen[got[v]] = v;
                CHECK_EQ(got[v], next);
                ++next;
            }
        }
        CHECK_EQ(next, k);
    }
}
