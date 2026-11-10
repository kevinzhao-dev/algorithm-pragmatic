#include <algorithm>
#include <optional>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Edges = std::vector<std::pair<int, int>>;

namespace {

Edges random_edges(Rng& rng, int n, int m) {
    Edges edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        int v = static_cast<int>(rng.uniform(0, n - 1));
        if (u == v) {
            v = (v + 1) % n;
        }
        edges.emplace_back(u, v);
    }
    return edges;
}

bool is_cover(const Edges& edges, const std::vector<int>& cover) {
    const std::set<int> c(cover.begin(), cover.end());
    if (c.size() != cover.size()) {
        return false;
    }
    for (const auto& [u, v] : edges) {
        if (!c.count(u) && !c.count(v)) {
            return false;
        }
    }
    return true;
}

int brute_min_cover(int n, const Edges& edges) {
    int best = n;
    for (int mask = 0; mask < (1 << n); ++mask) {
        const int size = __builtin_popcount(mask);
        if (size >= best) {
            continue;
        }
        bool ok = true;
        for (const auto& [u, v] : edges) {
            ok = ok && ((mask >> u & 1) || (mask >> v & 1));
        }
        if (ok) {
            best = size;
        }
    }
    return best;
}

// A graph with a planted cover of size k: every edge touches one of the k hub vertices.
Edges planted(Rng& rng, int n, int k, int m) {
    Edges edges;
    for (int i = 0; i < m; ++i) {
        const int hub = static_cast<int>(rng.uniform(0, k - 1));
        int other = static_cast<int>(rng.uniform(k, n - 1));
        if (rng.coin(0.1)) {
            other = static_cast<int>(rng.uniform(0, n - 1));
            if (other == hub) {
                other = (other + 1) % n;
            }
        }
        if (rng.coin()) {
            edges.emplace_back(hub, other);
        } else {
            edges.emplace_back(other, hub);
        }
    }
    return edges;
}

}  // namespace

TEST(1, "no edges need no cover") {
    const auto cover = vc_search(5, Edges{}, 0);
    REQUIRE(cover.has_value());
    CHECK(cover->empty());
}

TEST(1, "a triangle needs two vertices") {
    const Edges edges{{0, 1}, {1, 2}, {0, 2}};
    CHECK(!vc_search(3, edges, 1).has_value());
    const auto cover = vc_search(3, edges, 2);
    REQUIRE(cover.has_value());
    CHECK(is_cover(edges, *cover));
    CHECK_LE(cover->size(), 2);
}

TEST(1, "duplicate edges do not change the answer") {
    const Edges edges{{0, 1}, {0, 1}, {1, 0}};
    const auto cover = vc_search(2, edges, 1);
    REQUIRE(cover.has_value());
    CHECK(is_cover(edges, *cover));
}

TEST(1, "random graphs match exhaustive search") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 11));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 30)));
        const int k = static_cast<int>(rng.uniform(0, n));
        const auto cover = vc_search(n, edges, k);
        INFO("round ", round);
        CHECK_EQ(cover.has_value(), brute_min_cover(n, edges) <= k);
        if (cover) {
            CHECK(is_cover(edges, *cover));
            CHECK_LE(static_cast<int>(cover->size()), k);
        }
    }
}

PERF(1, "moderate k stays exponential only in k", 1500) {
    Rng rng;
    const auto edges = planted(rng, 400, 16, 1500);
    const auto cover = vc_search(400, edges, 16);
    REQUIRE(cover.has_value());
    CHECK(is_cover(edges, *cover));
}

TEST(2, "a high degree vertex is forced") {
    Edges edges;
    for (int v = 1; v < 10; ++v) {
        edges.emplace_back(0, v);
    }
    const Kernel kernel = kernelize(10, edges, 2);
    REQUIRE(!kernel.no_instance);
    CHECK_EQ(kernel.forced, std::vector<int>{0});
    CHECK_EQ(kernel.k, 1);
    CHECK_EQ(kernel.n, 0);
}

TEST(2, "more forced vertices than budget means no") {
    Edges edges;
    for (int v = 2; v < 10; ++v) {
        edges.emplace_back(0, v);
        edges.emplace_back(1, v);
    }
    CHECK(kernelize(10, edges, 1).no_instance);
}

TEST(2, "too many edges after the degree rule means no") {
    // A 4-cycle has max degree 2 and needs a cover of size 2; with k=1 no vertex is forced
    // (degree 2 > 1 forces one, then k=0 and the remaining edges exceed 0).
    const Edges edges{{0, 1}, {1, 2}, {2, 3}, {3, 0}};
    CHECK(kernelize(4, edges, 1).no_instance);
}

TEST(2, "isolated vertices disappear and ids are compact") {
    const Edges edges{{5, 9}, {9, 5}};
    const Kernel kernel = kernelize(10, edges, 3);
    REQUIRE(!kernel.no_instance);
    CHECK_EQ(kernel.k, 3);
    CHECK_EQ(kernel.n, 2);
    CHECK_EQ(kernel.edges.size(), 1);
    CHECK_EQ(kernel.to_original, (std::vector<int>{5, 9}));
}

TEST(2, "kernels are equivalent to the input and respect the size bounds") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 11));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 40)));
        const int k = static_cast<int>(rng.uniform(0, n));
        const Kernel kernel = kernelize(n, edges, k);
        const bool answer = brute_min_cover(n, edges) <= k;
        INFO("round ", round, " n=", n, " k=", k);
        if (kernel.no_instance) {
            CHECK(!answer);
            continue;
        }
        CHECK_LE(kernel.edges.size(), static_cast<std::size_t>(kernel.k) * kernel.k);
        CHECK_LE(kernel.n, 2 * kernel.k * kernel.k);
        CHECK(kernel.k <= k);
        const std::set<std::pair<int, int>> distinct(kernel.edges.begin(), kernel.edges.end());
        CHECK_EQ(distinct.size(), kernel.edges.size());
        for (const auto& [u, v] : kernel.edges) {
            REQUIRE(u >= 0 && u < kernel.n);
            REQUIRE(v >= 0 && v < kernel.n);
            CHECK(u != v);
        }
        const bool kernel_answer = brute_min_cover(kernel.n, kernel.edges) <= kernel.k;
        CHECK_EQ(kernel_answer, answer);
        // Forced vertices plus a kernel cover cover the original graph.
        auto cover = kernel.forced;
        std::vector<int> inner;
        if (kernel_answer) {
            // Find a minimum cover of the kernel by brute force.
            int best_mask = -1;
            for (int mask = 0; mask < (1 << kernel.n); ++mask) {
                bool ok = __builtin_popcount(mask) <= kernel.k;
                for (const auto& [u, v] : kernel.edges) {
                    ok = ok && ((mask >> u & 1) || (mask >> v & 1));
                }
                if (ok) {
                    best_mask = mask;
                    break;
                }
            }
            REQUIRE(best_mask != -1);
            for (int v = 0; v < kernel.n; ++v) {
                if (best_mask >> v & 1) {
                    cover.push_back(kernel.to_original[v]);
                }
            }
            CHECK(is_cover(edges, cover));
            CHECK_LE(static_cast<int>(cover.size()), k);
        }
    }
}

TEST(3, "a large planted cover is found") {
    Rng rng;
    const auto edges = planted(rng, 200'000, 12, 600'000);
    const auto cover = vc_fpt(200'000, edges, 12);
    REQUIRE(cover.has_value());
    CHECK(is_cover(edges, *cover));
    CHECK_LE(cover->size(), 12);
}

TEST(3, "a large graph with too small k is rejected") {
    Rng rng;
    const auto edges = planted(rng, 50'000, 20, 200'000);
    CHECK(!vc_fpt(50'000, edges, 10).has_value());
}

TEST(3, "random graphs match exhaustive search") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 11));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 30)));
        const int k = static_cast<int>(rng.uniform(0, n));
        const auto cover = vc_fpt(n, edges, k);
        INFO("round ", round);
        CHECK_EQ(cover.has_value(), brute_min_cover(n, edges) <= k);
        if (cover) {
            CHECK(is_cover(edges, *cover));
            CHECK_LE(static_cast<int>(cover->size()), k);
        }
    }
}

PERF(3, "n in the millions with a tiny parameter", 2000) {
    Rng rng;
    const auto edges = planted(rng, 1'000'000, 16, 3'000'000);
    const auto cover = vc_fpt(1'000'000, edges, 16);
    REQUIRE(cover.has_value());
    CHECK_LE(cover->size(), 16);
}

TEST(4, "the minimum cover of an empty graph is empty") {
    CHECK(min_vertex_cover(4, Edges{}).empty());
}

TEST(4, "random graphs get a truly minimum cover") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 11));
        const auto edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 25)));
        const auto cover = min_vertex_cover(n, edges);
        INFO("round ", round);
        CHECK(is_cover(edges, cover));
        CHECK_EQ(static_cast<int>(cover.size()), brute_min_cover(n, edges));
    }
}

PERF(4, "small optimum on a huge graph", 2500) {
    Rng rng;
    const auto edges = planted(rng, 500'000, 15, 1'500'000);
    const auto cover = min_vertex_cover(500'000, edges);
    CHECK_LE(cover.size(), 15);
}
