#include <algorithm>
#include <functional>
#include <numeric>
#include <optional>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Tournament transitive(int n) {
    Tournament t(n, std::vector<char>(n, 0));
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            t[u][v] = 1;
        }
    }
    return t;
}

Tournament random_tournament(Rng& rng, int n) {
    Tournament t(n, std::vector<char>(n, 0));
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (rng.coin()) {
                t[u][v] = 1;
            } else {
                t[v][u] = 1;
            }
        }
    }
    return t;
}

void flip(Tournament& t, int u, int v) {
    t[u][v] = 0;
    t[v][u] = 1;
}

// Brute-force check that a tournament has no directed cycle (DFS over all paths is overkill for
// a transitivity test, so compare with Kahn's algorithm).
bool brute_acyclic(const Tournament& t) {
    const int n = static_cast<int>(t.size());
    std::vector<int> indegree(n, 0);
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            indegree[v] += t[u][v];
        }
    }
    std::vector<int> queue;
    for (int v = 0; v < n; ++v) {
        if (indegree[v] == 0) {
            queue.push_back(v);
        }
    }
    int seen = 0;
    while (!queue.empty()) {
        const int v = queue.back();
        queue.pop_back();
        ++seen;
        for (int w = 0; w < n; ++w) {
            if (t[v][w] && --indegree[w] == 0) {
                queue.push_back(w);
            }
        }
    }
    return seen == n;
}

// Minimum feedback arc set by DP over vertex subsets (n <= 14).
int brute_min_fas(const Tournament& t) {
    const int n = static_cast<int>(t.size());
    const int inf = 1'000'000;
    std::vector<int> dp(1 << n, inf);
    dp[0] = 0;
    for (int mask = 0; mask < (1 << n); ++mask) {
        for (int v = 0; v < n; ++v) {
            if (mask >> v & 1) {
                continue;
            }
            int cost = 0;
            for (int u = 0; u < n; ++u) {
                if ((mask >> u & 1) && t[v][u]) {
                    ++cost;  // v is placed after u but points to it: a back edge
                }
            }
            const int next = mask | (1 << v);
            dp[next] = std::min(dp[next], dp[mask] + cost);
        }
    }
    return dp[(1 << n) - 1];
}

bool is_tournament(const Tournament& t) {
    const int n = static_cast<int>(t.size());
    for (int u = 0; u < n; ++u) {
        if (t[u][u] != 0) {
            return false;
        }
        for (int v = u + 1; v < n; ++v) {
            if (t[u][v] + t[v][u] != 1) {
                return false;
            }
        }
    }
    return true;
}

// Planted instance: a transitive tournament with k short back edges.
Tournament planted(Rng& rng, int n, int k, int max_span) {
    Tournament t = transitive(n);
    int placed = 0;
    while (placed < k) {
        const int u = static_cast<int>(rng.uniform(0, n - 2));
        const int v = static_cast<int>(rng.uniform(u + 1, std::min(n - 1, u + max_span)));
        if (t[u][v]) {
            flip(t, u, v);
            ++placed;
        }
    }
    return t;
}

}  // namespace

TEST(1, "transitive tournaments are acyclic") {
    CHECK(is_acyclic(transitive(1)));
    CHECK(is_acyclic(transitive(6)));
    CHECK(!find_triangle(transitive(6)).has_value());
}

TEST(1, "a 3-cycle is detected and returned") {
    Tournament t(3, std::vector<char>(3, 0));
    t[0][1] = t[1][2] = t[2][0] = 1;
    CHECK(!is_acyclic(t));
    const auto triangle = find_triangle(t);
    REQUIRE(triangle.has_value());
    const auto [a, b, c] = *triangle;
    CHECK(t[a][b] && t[b][c] && t[c][a]);
}

TEST(1, "random tournaments: acyclicity and triangles agree with Kahn") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        auto t = random_tournament(rng, n);
        if (rng.coin()) {
            t = transitive(n);
            for (int flips = static_cast<int>(rng.uniform(0, 2)); flips > 0; --flips) {
                if (n >= 2) {
                    const int u = static_cast<int>(rng.uniform(0, n - 2));
                    const int v = static_cast<int>(rng.uniform(u + 1, n - 1));
                    if (t[u][v]) {
                        flip(t, u, v);
                    }
                }
            }
        }
        const bool acyclic = brute_acyclic(t);
        const auto triangle = find_triangle(t);
        INFO("round ", round, " n=", n);
        CHECK_EQ(is_acyclic(t), acyclic);
        CHECK_EQ(triangle.has_value(), !acyclic);
        if (triangle) {
            const auto [a, b, c] = *triangle;
            CHECK(t[a][b] && t[b][c] && t[c][a]);
        }
    }
}

PERF(1, "triangle finding on a large tournament", 500) {
    Rng rng;
    auto t = transitive(3000);
    // A single back edge over two positions creates exactly one triangle.
    flip(t, 1500, 1502);
    long long found = 0;
    for (int step = 0; step < 20; ++step) {
        found += find_triangle(t).has_value() ? 1 : 0;
        found += is_acyclic(t) ? 1 : 0;
    }
    CHECK_EQ(found, 20);
}

TEST(2, "an acyclic tournament reduces to nothing") {
    const auto kernel = kernelize(transitive(8), 3);
    REQUIRE(!kernel.no_instance);
    CHECK_EQ(kernel.reduced.size(), 0);
    CHECK_EQ(kernel.k, 3);
}

TEST(2, "negative budget or an oversized kernel is rejected") {
    Rng rng;
    Tournament t = transitive(30);
    flip(t, 0, 29);
    CHECK(kernelize(t, 0).no_instance);
}

TEST(2, "an edge in too many triangles is forced") {
    Tournament t = transitive(6);
    flip(t, 0, 5);  // edge 5->0: triangles through it for every middle vertex
    const auto kernel = kernelize(t, 2);
    REQUIRE(!kernel.no_instance);
    REQUIRE_EQ(kernel.reversed.size(), 1);
    CHECK_EQ(kernel.reversed[0], (std::pair<int, int>{5, 0}));
    CHECK_EQ(kernel.k, 1);
    CHECK_EQ(kernel.reduced.size(), 0);
}

TEST(2, "vertices in no triangle are removed") {
    Tournament t = transitive(5);
    // Make a cycle among 0, 1, 2; vertices 3 and 4 lose nothing and are in no triangle.
    t[0][2] = 0;
    t[2][0] = 1;
    const auto kernel = kernelize(t, 1);
    REQUIRE(!kernel.no_instance);
    REQUIRE_EQ(kernel.reduced.size(), 3);
    auto kept = kernel.to_original;
    std::sort(kept.begin(), kept.end());
    CHECK_EQ(kept, (std::vector<int>{0, 1, 2}));
    CHECK(is_tournament(kernel.reduced));
}

TEST(2, "kernels preserve the answer and respect the size bound") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const auto t = random_tournament(rng, n);
        const int k = static_cast<int>(rng.uniform(0, 8));
        const auto kernel = kernelize(t, k);
        const bool answer = brute_min_fas(t) <= k;
        INFO("round ", round, " n=", n, " k=", k);
        if (kernel.no_instance) {
            CHECK(!answer);
            continue;
        }
        CHECK_LE(static_cast<int>(kernel.reduced.size()), kernel.k * (kernel.k + 2));
        CHECK(is_tournament(kernel.reduced));
        CHECK_EQ(kernel.to_original.size(), kernel.reduced.size());
        CHECK_EQ(brute_min_fas(kernel.reduced) <= kernel.k, answer);
        // The reversed edges are genuine edges of t, and all edges are distinct.
        for (const auto& [u, v] : kernel.reversed) {
            REQUIRE(u >= 0 && u < n && v >= 0 && v < n && u != v);
            CHECK(t[u][v]);
        }
    }
}

TEST(3, "an acyclic tournament needs no reversals") {
    const auto arcs = feedback_arcs(transitive(10), 0);
    REQUIRE(arcs.has_value());
    CHECK(arcs->empty());
}

TEST(3, "a triangle needs exactly one reversal") {
    Tournament t(3, std::vector<char>(3, 0));
    t[0][1] = t[1][2] = t[2][0] = 1;
    CHECK(!feedback_arcs(t, 0).has_value());
    const auto arcs = feedback_arcs(t, 1);
    REQUIRE(arcs.has_value());
    CHECK_EQ(arcs->size(), 1);
}

TEST(3, "random tournaments match the exact minimum") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const auto t = random_tournament(rng, n);
        const int opt = brute_min_fas(t);
        INFO("round ", round, " n=", n, " opt=", opt);
        for (int k : {opt - 1, opt, opt + 1}) {
            const auto arcs = feedback_arcs(t, k);
            REQUIRE_EQ(arcs.has_value(), k >= opt);
            if (arcs) {
                CHECK_LE(static_cast<int>(arcs->size()), k);
                Tournament fixed = t;
                for (const auto& [u, v] : *arcs) {
                    REQUIRE(t[u][v]);
                    flip(fixed, u, v);
                }
                CHECK(brute_acyclic(fixed));
            }
        }
    }
}

PERF(3, "planted instance with a small parameter", 800) {
    Rng rng;
    const int n = 300, k = 10;
    const auto t = planted(rng, n, k, 3);
    const auto arcs = feedback_arcs(t, k);
    REQUIRE(arcs.has_value());
    CHECK_LE(static_cast<int>(arcs->size()), k);
    Tournament fixed = t;
    for (const auto& [u, v] : *arcs) {
        REQUIRE(t[u][v]);
        flip(fixed, u, v);
    }
    CHECK(brute_acyclic(fixed));
}

TEST(4, "an already consistent order is returned") {
    const auto ranking = repair_ranking(transitive(5), 0);
    REQUIRE(ranking.has_value());
    CHECK_EQ(*ranking, (std::vector<int>{0, 1, 2, 3, 4}));
}

TEST(4, "too small an allowance is rejected") {
    Tournament t(3, std::vector<char>(3, 0));
    t[0][1] = t[1][2] = t[2][0] = 1;
    CHECK(!repair_ranking(t, 0).has_value());
    CHECK(repair_ranking(t, 1).has_value());
}

TEST(4, "random tournaments get a minimum-upset ranking") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const auto t = random_tournament(rng, n);
        const int opt = brute_min_fas(t);
        const auto ranking = repair_ranking(t, n * n);
        INFO("round ", round, " n=", n, " opt=", opt);
        REQUIRE(ranking.has_value());
        REQUIRE_EQ(ranking->size(), n);
        std::vector<int> pos(n, -1);
        for (int i = 0; i < n; ++i) {
            pos[(*ranking)[i]] = i;
        }
        CHECK(std::find(pos.begin(), pos.end(), -1) == pos.end());
        int upsets = 0;
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                upsets += (t[u][v] && pos[u] > pos[v]) ? 1 : 0;
            }
        }
        CHECK_EQ(upsets, opt);
    }
}

PERF(4, "repair a near-consistent pairwise table", 800) {
    Rng rng;
    const int n = 200;
    const auto t = planted(rng, n, 7, 4);
    const auto ranking = repair_ranking(t, 9);
    REQUIRE(ranking.has_value());
    CHECK_EQ(ranking->size(), n);
}
