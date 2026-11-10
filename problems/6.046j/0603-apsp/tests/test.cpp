#include <algorithm>
#include <optional>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Matrix empty_matrix(int n) {
    Matrix m(n, std::vector<long long>(n, kInf));
    for (int i = 0; i < n; ++i) {
        m[i][i] = 0;
    }
    return m;
}

// O(V^2 E) reference: Bellman-Ford from every source, negative cycle => nullopt.
std::optional<Matrix> oracle(const Matrix& w) {
    const int n = static_cast<int>(w.size());
    Matrix out = empty_matrix(n);
    for (int s = 0; s < n; ++s) {
        auto& d = out[s];
        for (int round = 0; round < n; ++round) {
            for (int u = 0; u < n; ++u) {
                for (int v = 0; v < n; ++v) {
                    if (d[u] != kInf && w[u][v] != kInf && d[u] + w[u][v] < d[v]) {
                        d[v] = d[u] + w[u][v];
                    }
                }
            }
        }
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                if (d[u] != kInf && w[u][v] != kInf && d[u] + w[u][v] < d[v]) {
                    return std::nullopt;
                }
            }
        }
    }
    return out;
}

Matrix random_matrix(Rng& rng, int n, double density, int low, int high) {
    Matrix w = empty_matrix(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && rng.coin(density)) {
                w[i][j] = rng.uniform(low, high);
            }
        }
    }
    return w;
}

std::vector<Edge> edges_of(const Matrix& w) {
    std::vector<Edge> edges;
    for (int i = 0; i < static_cast<int>(w.size()); ++i) {
        for (int j = 0; j < static_cast<int>(w.size()); ++j) {
            if (i != j && w[i][j] != kInf) {
                edges.push_back({i, j, w[i][j]});
            }
        }
    }
    return edges;
}

}  // namespace

TEST(1, "min_plus treats infinity as no path") {
    const Matrix a{{1, 2}, {kInf, 3}};
    const Matrix b{{4, kInf}, {5, 6}};
    CHECK_EQ(min_plus(a, b), Matrix{{5, 8}, {8, 9}});
    CHECK_EQ(min_plus(b, a), Matrix{{5, 6}, {6, 7}});
}

TEST(1, "repeated squaring on the empty graph and on one vertex") {
    CHECK_EQ(apsp_repeated_squaring(Matrix{}), std::optional<Matrix>(Matrix{}));
    CHECK_EQ(apsp_repeated_squaring(Matrix{{0}}), std::optional<Matrix>(Matrix{{0}}));
}

TEST(1, "paths needing all n-1 edges are found") {
    const int n = 17;
    Matrix w = empty_matrix(n);
    for (int i = 0; i + 1 < n; ++i) {
        w[i][i + 1] = 1;
    }
    const auto d = apsp_repeated_squaring(w);
    REQUIRE(d.has_value());
    CHECK_EQ((*d)[0][n - 1], n - 1);
    CHECK_EQ((*d)[n - 1][0], kInf);
}

TEST(1, "negative edges without a cycle are fine, a negative cycle is reported") {
    const Matrix ok{{0, 4, kInf}, {kInf, 0, -3}, {2, kInf, 0}};
    const auto d = apsp_repeated_squaring(ok);
    REQUIRE(d.has_value());
    CHECK_EQ((*d)[0][2], 1);
    const Matrix bad{{0, 1, kInf}, {kInf, 0, -1}, {-1, kInf, 0}};
    CHECK(!apsp_repeated_squaring(bad).has_value());
}

TEST(1, "random graphs match Bellman-Ford") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 12));
        const Matrix w = random_matrix(rng, n, 0.4, -3, 20);
        INFO("round ", round);
        CHECK_EQ(apsp_repeated_squaring(w), oracle(w));
    }
}

TEST(2, "floyd reports distances and an explicit route") {
    const Matrix w{{0, 3, 8}, {kInf, 0, 2}, {kInf, kInf, 0}};
    FloydWarshall fw(w);
    CHECK(!fw.has_negative_cycle());
    CHECK_EQ(fw.dist(0, 2), 5);
    CHECK_EQ(fw.path(0, 2), std::vector<int>{0, 1, 2});
    CHECK_EQ(fw.path(1, 1), std::vector<int>{1});
    CHECK(fw.path(2, 0).empty());
    CHECK_EQ(fw.dist(2, 0), kInf);
}

TEST(2, "a negative cycle is reported and paths are empty") {
    FloydWarshall fw(Matrix{{0, 1}, {-3, 0}});
    CHECK(fw.has_negative_cycle());
    CHECK(fw.path(0, 1).empty());
}

TEST(2, "routes on random graphs have the promised length") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const Matrix w = random_matrix(rng, n, 0.5, -2, 15);
        const auto expected = oracle(w);
        FloydWarshall fw(w);
        INFO("round ", round);
        REQUIRE_EQ(fw.has_negative_cycle(), !expected.has_value());
        if (!expected) {
            continue;
        }
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                INFO("u=", u, " v=", v);
                CHECK_EQ(fw.dist(u, v), (*expected)[u][v]);
                const auto route = fw.path(u, v);
                if ((*expected)[u][v] == kInf) {
                    CHECK(route.empty());
                    continue;
                }
                REQUIRE(!route.empty());
                CHECK_EQ(route.front(), u);
                CHECK_EQ(route.back(), v);
                long long total = 0;
                for (std::size_t i = 0; i + 1 < route.size(); ++i) {
                    REQUIRE(w[route[i]][route[i + 1]] != kInf);
                    total += w[route[i]][route[i + 1]];
                }
                CHECK_EQ(total, (*expected)[u][v]);
            }
        }
    }
}

TEST(3, "johnson handles negative edges") {
    const std::vector<Edge> edges{{0, 1, 4}, {1, 2, -3}, {2, 0, 2}, {3, 0, -1}};
    const auto d = johnson(4, edges);
    REQUIRE(d.has_value());
    CHECK_EQ((*d)[0][2], 1);
    CHECK_EQ((*d)[3][1], 3);
    CHECK_EQ((*d)[0][3], kInf);
}

TEST(3, "johnson detects a negative cycle") {
    const std::vector<Edge> edges{{0, 1, 1}, {1, 2, -2}, {2, 0, 0}};
    CHECK(!johnson(3, edges).has_value());
}

TEST(3, "parallel edges use the lighter one") {
    const std::vector<Edge> edges{{0, 1, 5}, {0, 1, 2}};
    const auto d = johnson(2, edges);
    REQUIRE(d.has_value());
    CHECK_EQ((*d)[0][1], 2);
}

TEST(3, "johnson matches Bellman-Ford on random graphs") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 25));
        const Matrix w = random_matrix(rng, n, 0.15, -2, 30);
        INFO("round ", round);
        CHECK_EQ(johnson(n, edges_of(w)), oracle(w));
    }
}

PERF(3, "sparse graph with thousands of vertices", 3000) {
    Rng rng;
    const int n = 3000;
    std::vector<Edge> edges;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < 4; ++j) {
            const int to = static_cast<int>(rng.uniform(0, n - 1));
            if (to != i) {
                edges.push_back({i, to, rng.uniform(1, 1000)});
            }
        }
    }
    const auto d = johnson(n, edges);
    REQUIRE(d.has_value());
    CHECK_EQ((*d)[0][0], 0);
}

TEST(4, "no constraints is trivially solvable") {
    const auto x = solve_difference_constraints(3, std::vector<Constraint>{});
    REQUIRE(x.has_value());
    CHECK_EQ(x->size(), 3);
}

TEST(4, "tight chain of constraints") {
    const std::vector<Constraint> constraints{{1, 0, 3}, {0, 1, -3}, {2, 1, 2}, {1, 2, -2}};
    const auto x = solve_difference_constraints(3, constraints);
    REQUIRE(x.has_value());
    CHECK_EQ((*x)[1] - (*x)[0], 3);
    CHECK_EQ((*x)[2] - (*x)[1], 2);
}

TEST(4, "contradictory constraints have no solution") {
    const std::vector<Constraint> constraints{{1, 0, 2}, {0, 1, -3}};
    CHECK(!solve_difference_constraints(2, constraints).has_value());
}

TEST(4, "random systems are solved or correctly rejected") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        const int m = static_cast<int>(rng.uniform(0, 3 * n));
        std::vector<Constraint> constraints;
        Matrix w = empty_matrix(n);
        for (int i = 0; i < m; ++i) {
            const int a = static_cast<int>(rng.uniform(0, n - 1));
            const int b = static_cast<int>(rng.uniform(0, n - 1));
            if (a == b) {
                continue;
            }
            const long long bound = rng.uniform(-6, 12);
            constraints.push_back({a, b, bound});
            w[b][a] = std::min(w[b][a], bound);
        }
        INFO("round ", round);
        const auto x = solve_difference_constraints(n, constraints);
        REQUIRE_EQ(x.has_value(), oracle(w).has_value());
        if (x) {
            REQUIRE_EQ(x->size(), n);
            for (const auto& c : constraints) {
                CHECK_LE((*x)[c.later] - (*x)[c.earlier], c.bound);
            }
        }
    }
}
