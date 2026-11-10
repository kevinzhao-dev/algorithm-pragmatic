#include <algorithm>
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

Matrix random_matrix(Rng& rng, int n, double density, int high) {
    Matrix w = empty_matrix(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (i != j && rng.coin(density)) {
                w[i][j] = rng.uniform(0, high);
            }
        }
    }
    return w;
}

Matrix floyd(const Matrix& w) {
    Matrix d = w;
    const int n = static_cast<int>(w.size());
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (d[i][k] != kInf && d[k][j] != kInf) {
                    d[i][j] = std::min(d[i][j], d[i][k] + d[k][j]);
                }
            }
        }
    }
    return d;
}

Matrix hop_oracle(const Matrix& w, int h) {
    const int n = static_cast<int>(w.size());
    Matrix cur = empty_matrix(n);
    for (int b = 0; b < h; ++b) {
        Matrix next = cur;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int k = 0; k < n; ++k) {
                    if (cur[i][k] != kInf && w[k][j] != kInf) {
                        next[i][j] = std::min(next[i][j], cur[i][k] + w[k][j]);
                    }
                }
            }
        }
        cur = std::move(next);
    }
    return cur;
}

void check_matches(const DynamicApsp& g, const Matrix& w) {
    const Matrix expected = floyd(w);
    const int n = static_cast<int>(w.size());
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            CHECK_EQ(g.dist(u, v), expected[u][v]);
            const auto route = g.path(u, v);
            if (expected[u][v] == kInf) {
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
            CHECK_EQ(total, expected[u][v]);
        }
    }
}

long long diff_count(const Matrix& a, const Matrix& b) {
    long long diff = 0;
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t j = 0; j < a.size(); ++j) {
            diff += a[i][j] != b[i][j] ? 1 : 0;
        }
    }
    return diff;
}

}  // namespace

TEST(1, "static distances and routes") {
    Matrix w = empty_matrix(4);
    w[0][1] = 3;
    w[1][2] = 4;
    w[0][2] = 10;
    w[2][3] = 1;
    DynamicApsp g(w);
    check_matches(g, w);
    CHECK_EQ(g.dist(0, 3), 8);
    CHECK_EQ(g.path(0, 3), std::vector<int>{0, 1, 2, 3});
    CHECK_EQ(g.dist(3, 0), kInf);
    CHECK(g.path(3, 0).empty());
    CHECK_EQ(g.path(2, 2), std::vector<int>{2});
}

TEST(1, "no-op update returns zero") {
    Matrix w = empty_matrix(3);
    w[0][1] = 5;
    DynamicApsp g(w);
    CHECK_EQ(g.update(0, 1, 5), 0);
    CHECK_EQ(g.update(2, 2, 0), 0);
    check_matches(g, w);
}

TEST(1, "decrease creates a shortcut and counts changed pairs") {
    Matrix w = empty_matrix(3);
    w[0][1] = 100;
    w[1][2] = 1;
    DynamicApsp g(w);
    // Pairs (0,1), (0,2) change.
    CHECK_EQ(g.update(0, 1, 2), 2);
    w[0][1] = 2;
    check_matches(g, w);
    CHECK_EQ(g.path(0, 2), std::vector<int>{0, 1, 2});
}

TEST(1, "adding a brand new edge is a decrease") {
    Matrix w = empty_matrix(3);
    w[1][2] = 1;
    DynamicApsp g(w);
    CHECK_EQ(g.update(0, 1, 7), 2);
    w[0][1] = 7;
    check_matches(g, w);
    CHECK_EQ(g.dist(0, 2), 8);
}

TEST(1, "increase breaks shortest paths and they get rerouted") {
    Matrix w = empty_matrix(3);
    w[0][2] = 1;
    w[0][1] = 3;
    w[1][2] = 3;
    DynamicApsp g(w);
    CHECK_EQ(g.update(0, 2, 50), 1);
    w[0][2] = 50;
    check_matches(g, w);
    CHECK_EQ(g.path(0, 2), std::vector<int>{0, 1, 2});
}

TEST(1, "deleting an edge disconnects") {
    Matrix w = empty_matrix(2);
    w[0][1] = 2;
    DynamicApsp g(w);
    CHECK_EQ(g.update(0, 1, kInf), 1);
    CHECK_EQ(g.dist(0, 1), kInf);
    CHECK(g.path(0, 1).empty());
}

TEST(1, "random update streams stay exact and count correctly") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 10));
        Matrix w = random_matrix(rng, n, 0.4, 12);
        DynamicApsp g(w);
        for (int step = 0; step < 40; ++step) {
            const int i = static_cast<int>(rng.uniform(0, n - 1));
            const int j = static_cast<int>(rng.uniform(0, n - 1));
            long long r = rng.uniform(0, 12);
            if (rng.coin(0.2)) {
                r = kInf;
            }
            INFO("round ", round, " step ", step);
            const Matrix before = floyd(w);
            const long long changed = g.update(i, j, r);
            if (i != j) {
                w[i][j] = r;
            }
            CHECK_EQ(changed, diff_count(before, floyd(w)));
            check_matches(g, w);
        }
    }
}

PERF(1, "edge decreases are quadratic per update", 2500) {
    Rng rng;
    const int n = 500;
    Matrix w = empty_matrix(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < 6; ++j) {
            const int to = static_cast<int>(rng.uniform(0, n - 1));
            if (to != i) {
                w[i][to] = rng.uniform(1'000'000, 2'000'000);
            }
        }
    }
    DynamicApsp g(w);
    long long total = 0;
    for (int step = 0; step < 1500; ++step) {
        const int i = static_cast<int>(rng.uniform(0, n - 1));
        const int j = static_cast<int>(rng.uniform(0, n - 1));
        total += g.update(i, j, rng.uniform(0, 500'000));
    }
    CHECK_GT(total, 0);
}

TEST(2, "a single update can change quadratically many distances") {
    for (int n : {4, 5, 9, 32}) {
        Scenario s = worst_update_scenario(n);
        REQUIRE_EQ(s.w.size(), n);
        Matrix after = s.w;
        after[s.i][s.j] = s.r;
        DynamicApsp g(s.w);
        const long long changed = g.update(s.i, s.j, s.r);
        INFO("n=", n);
        CHECK_GE(changed, n * n / 4);
        CHECK_EQ(changed, diff_count(floyd(s.w), floyd(after)));
        check_matches(g, after);
    }
}

TEST(3, "zero hops only reaches yourself") {
    Matrix w = empty_matrix(3);
    w[0][1] = 2;
    const Matrix expected = empty_matrix(3);
    CHECK_EQ(bounded_hop_floyd(w, 0), expected);
}

TEST(3, "the hop limit forces a heavier route") {
    Matrix w = empty_matrix(4);
    w[0][1] = 1;
    w[1][2] = 1;
    w[2][3] = 1;
    w[0][3] = 100;
    CHECK_EQ(bounded_hop_floyd(w, 2)[0][3], 100);
    CHECK_EQ(bounded_hop_floyd(w, 3)[0][3], 3);
    CHECK_EQ(bounded_hop_floyd(w, 1)[0][2], kInf);
}

TEST(3, "random graphs match the hop-by-hop oracle") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const int h = static_cast<int>(rng.uniform(0, n - 1));
        const Matrix w = random_matrix(rng, n, 0.5, 9);
        INFO("round ", round, " n=", n, " h=", h);
        CHECK_EQ(bounded_hop_floyd(w, h), hop_oracle(w, h));
    }
}

TEST(4, "doubling agrees with floyd and the oracle") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const int h = static_cast<int>(rng.uniform(0, n - 1));
        const Matrix w = random_matrix(rng, n, 0.4, 9);
        INFO("round ", round, " n=", n, " h=", h);
        CHECK_EQ(bounded_hop_doubling(w, h), hop_oracle(w, h));
    }
}

TEST(4, "h equal to one is just the weight matrix") {
    Matrix w = empty_matrix(3);
    w[0][2] = 4;
    w[2][1] = 5;
    CHECK_EQ(bounded_hop_doubling(w, 1), w);
}

PERF(4, "doubling on a few hundred vertices", 800) {
    Rng rng;
    const int n = 300;
    const Matrix w = random_matrix(rng, n, 0.2, 1000);
    CHECK_EQ(bounded_hop_doubling(w, n - 1)[0][0], 0);
}

TEST(5, "construction answers every hop bound") {
    Matrix w = empty_matrix(4);
    w[0][1] = 1;
    w[1][2] = 1;
    w[2][3] = 1;
    w[0][3] = 100;
    DynamicBoundedApsp g(w, 3);
    CHECK_EQ(g.dist(0, 3), 3);
    CHECK_EQ(g.dist_at_most(0, 3, 2), 100);
    CHECK_EQ(g.dist_at_most(0, 3, 0), kInf);
    CHECK_EQ(g.dist_at_most(2, 2, 0), 0);
}

TEST(5, "decrease and increase both update every level") {
    Matrix w = empty_matrix(4);
    w[0][1] = 5;
    w[1][2] = 5;
    w[2][3] = 5;
    DynamicBoundedApsp g(w, 3);
    g.update(1, 2, 1);
    CHECK_EQ(g.dist(0, 3), 11);
    CHECK_EQ(g.dist_at_most(0, 2, 2), 6);
    g.update(1, 2, 7);
    CHECK_EQ(g.dist(0, 3), 17);
    CHECK_EQ(g.dist_at_most(0, 2, 2), 12);
    g.update(0, 1, kInf);
    CHECK_EQ(g.dist(0, 3), kInf);
}

TEST(5, "random update streams stay exact on every level") {
    Rng rng;
    for (int round = 0; round < 40; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 9));
        const int h = static_cast<int>(rng.uniform(1, n));
        Matrix w = random_matrix(rng, n, 0.4, 10);
        DynamicBoundedApsp g(w, h);
        for (int step = 0; step < 30; ++step) {
            const int i = static_cast<int>(rng.uniform(0, n - 1));
            const int j = static_cast<int>(rng.uniform(0, n - 1));
            const long long r = rng.coin(0.2) ? kInf : rng.uniform(0, 10);
            g.update(i, j, r);
            if (i != j) {
                w[i][j] = r;
            }
            const int k = static_cast<int>(rng.uniform(0, h));
            const Matrix expected = hop_oracle(w, k);
            INFO("round ", round, " step ", step, " k=", k);
            for (int u = 0; u < n; ++u) {
                for (int v = 0; v < n; ++v) {
                    CHECK_EQ(g.dist_at_most(u, v, k), expected[u][v]);
                }
            }
        }
    }
}

PERF(5, "edge decreases avoid a full rebuild", 1000) {
    Rng rng;
    const int n = 200;
    const int h = 5;
    Matrix w = empty_matrix(n);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < 8; ++j) {
            const int to = static_cast<int>(rng.uniform(0, n - 1));
            if (to != i) {
                w[i][to] = rng.uniform(1'000'000, 2'000'000);
            }
        }
    }
    DynamicBoundedApsp g(w, h);
    for (int step = 0; step < 800; ++step) {
        const int i = static_cast<int>(rng.uniform(0, n - 1));
        const int j = static_cast<int>(rng.uniform(0, n - 1));
        g.update(i, j, rng.uniform(0, 500'000));
    }
    CHECK_GE(g.dist(0, n - 1), 0);
}
