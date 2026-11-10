#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Edges = std::vector<std::pair<int, int>>;

namespace {

Edges random_edges(Rng& rng, int n, double density) {
    Edges edges;
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (rng.coin(density)) {
                edges.emplace_back(u, v);
            }
        }
    }
    return edges;
}

int brute_vertex_cover(int n, const Edges& edges) {
    int best = n;
    for (int mask = 0; mask < (1 << n); ++mask) {
        if (__builtin_popcount(mask) >= best) {
            continue;
        }
        bool ok = true;
        for (const auto& [u, v] : edges) {
            ok = ok && ((mask >> u & 1) || (mask >> v & 1));
        }
        if (ok) {
            best = __builtin_popcount(mask);
        }
    }
    return best;
}

bool covers(const Edges& edges, const std::vector<int>& cover) {
    const std::set<int> c(cover.begin(), cover.end());
    for (const auto& [u, v] : edges) {
        if (!c.count(u) && !c.count(v)) {
            return false;
        }
    }
    return true;
}

int brute_set_cover(int universe, const std::vector<std::vector<int>>& sets) {
    const int m = static_cast<int>(sets.size());
    int best = m + 1;
    for (int mask = 0; mask < (1 << m); ++mask) {
        if (__builtin_popcount(mask) >= best) {
            continue;
        }
        std::set<int> got;
        for (int i = 0; i < m; ++i) {
            if (mask >> i & 1) {
                got.insert(sets[i].begin(), sets[i].end());
            }
        }
        if (static_cast<int>(got.size()) == universe) {
            best = __builtin_popcount(mask);
        }
    }
    return best;
}

long long brute_partition(const std::vector<long long>& w) {
    const long long total = std::accumulate(w.begin(), w.end(), 0LL);
    long long best = total;
    const int n = static_cast<int>(w.size());
    for (int mask = 0; mask < (1 << n); ++mask) {
        long long a = 0;
        for (int i = 0; i < n; ++i) {
            a += (mask >> i & 1) ? w[i] : 0;
        }
        best = std::min(best, std::max(a, total - a));
    }
    return best;
}

long long tour_cost(const std::vector<std::vector<long long>>& d, const std::vector<int>& tour) {
    long long cost = 0;
    for (std::size_t i = 0; i < tour.size(); ++i) {
        cost += d[tour[i]][tour[(i + 1) % tour.size()]];
    }
    return cost;
}

long long held_karp(const std::vector<std::vector<long long>>& d) {
    const int n = static_cast<int>(d.size());
    const long long inf = std::numeric_limits<long long>::max() / 4;
    std::vector<std::vector<long long>> dp(1 << n, std::vector<long long>(n, inf));
    dp[1][0] = 0;
    for (int mask = 1; mask < (1 << n); ++mask) {
        if (!(mask & 1)) {
            continue;
        }
        for (int v = 0; v < n; ++v) {
            if (dp[mask][v] == inf) {
                continue;
            }
            for (int w = 0; w < n; ++w) {
                if (!(mask >> w & 1)) {
                    const int next = mask | (1 << w);
                    dp[next][w] = std::min(dp[next][w], dp[mask][v] + d[v][w]);
                }
            }
        }
    }
    long long best = inf;
    for (int v = 0; v < n; ++v) {
        if (dp[(1 << n) - 1][v] != inf) {
            best = std::min(best, dp[(1 << n) - 1][v] + d[v][0]);
        }
    }
    return best;
}

std::vector<std::vector<long long>> random_metric(Rng& rng, int n) {
    std::vector<std::pair<long long, long long>> pts(n);
    for (auto& p : pts) {
        p = {rng.uniform(0, 100), rng.uniform(0, 100)};
    }
    std::vector<std::vector<long long>> d(n, std::vector<long long>(n, 0));
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            d[i][j] = std::llabs(pts[i].first - pts[j].first) +
                      std::llabs(pts[i].second - pts[j].second);
        }
    }
    return d;
}

}  // namespace

TEST(1, "no edges need no cover") {
    CHECK(approx_vertex_cover(4, Edges{}).empty());
}

TEST(1, "a perfect matching forces both endpoints") {
    const Edges edges{{0, 1}, {2, 3}, {4, 5}};
    CHECK_EQ(approx_vertex_cover(6, edges), (std::vector<int>{0, 1, 2, 3, 4, 5}));
}

TEST(1, "a star is covered by two vertices at most") {
    Edges edges;
    for (int v = 1; v < 50; ++v) {
        edges.emplace_back(0, v);
    }
    const auto cover = approx_vertex_cover(50, edges);
    CHECK(covers(edges, cover));
    CHECK_LE(cover.size(), 2);
}

TEST(1, "random graphs: valid cover within twice the optimum") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 11));
        const Edges edges = random_edges(rng, n, 0.4);
        const auto cover = approx_vertex_cover(n, edges);
        INFO("round ", round);
        CHECK(covers(edges, cover));
        CHECK(std::is_sorted(cover.begin(), cover.end()));
        CHECK(std::adjacent_find(cover.begin(), cover.end()) == cover.end());
        CHECK_LE(cover.size(), 2 * static_cast<std::size_t>(brute_vertex_cover(n, edges)));
    }
}

PERF(1, "linear on a very large graph", 500) {
    Rng rng;
    const int n = 2'000'000;
    Edges edges;
    edges.reserve(5'000'000);
    for (int i = 0; i < 5'000'000; ++i) {
        edges.emplace_back(rng.uniform(0, n - 1), rng.uniform(0, n - 1));
    }
    const auto cover = approx_vertex_cover(n, edges);
    CHECK_GT(cover.size(), 0);
}

TEST(2, "a single set covering everything") {
    CHECK_EQ(greedy_set_cover(3, {{0, 1, 2}}), std::vector<int>{0});
}

TEST(2, "greedy takes the largest set first") {
    const std::vector<std::vector<int>> sets{{0, 1, 2, 3}, {0, 1}, {2, 3, 4}, {4, 5}};
    const auto chosen = greedy_set_cover(6, sets);
    CHECK(std::find(chosen.begin(), chosen.end(), 0) != chosen.end());
    std::set<int> got;
    for (int i : chosen) {
        got.insert(sets[i].begin(), sets[i].end());
    }
    CHECK_EQ(got.size(), 6);
}

TEST(2, "greedy can be suboptimal on the classic trap") {
    // Greedy grabs the big middle set; the two side sets alone would suffice.
    const std::vector<std::vector<int>> sets{
        {0, 1, 2, 3, 4, 5},
        {0, 1, 2, 6},
        {3, 4, 5, 7},
    };
    const auto chosen = greedy_set_cover(8, sets);
    CHECK_EQ(chosen, std::vector<int>{0, 1, 2});
}

TEST(2, "random instances: valid, no wasted sets, within the logarithmic bound") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int universe = static_cast<int>(rng.uniform(1, 12));
        const int m = static_cast<int>(rng.uniform(1, 9));
        std::vector<std::vector<int>> sets(m);
        for (auto& s : sets) {
            for (int e = 0; e < universe; ++e) {
                if (rng.coin(0.4)) {
                    s.push_back(e);
                }
            }
        }
        // Make sure the union covers everything.
        for (int e = 0; e < universe; ++e) {
            sets[static_cast<std::size_t>(rng.uniform(0, m - 1))].push_back(e);
        }
        for (auto& s : sets) {
            std::sort(s.begin(), s.end());
            s.erase(std::unique(s.begin(), s.end()), s.end());
        }
        const auto chosen = greedy_set_cover(universe, sets);
        INFO("round ", round);
        std::set<int> got;
        int useful = 0;
        for (int i : chosen) {
            for (int e : sets[i]) {
                useful += got.insert(e).second ? 1 : 0;
            }
        }
        CHECK_EQ(got.size(), universe);
        CHECK_EQ(useful, universe);
        CHECK(std::is_sorted(chosen.begin(), chosen.end()));
        const int opt = brute_set_cover(universe, sets);
        CHECK_LE(static_cast<double>(chosen.size()),
                 opt * (std::log(static_cast<double>(universe)) + 1.0) + 1e-9);
    }
}

PERF(2, "lazy greedy on a large instance", 1500) {
    Rng rng;
    const int universe = 300'000;
    const int m = 60'000;
    std::vector<std::vector<int>> sets(m);
    for (auto& s : sets) {
        const int size = static_cast<int>(rng.uniform(1, 60));
        for (int i = 0; i < size; ++i) {
            s.push_back(static_cast<int>(rng.uniform(0, universe - 1)));
        }
    }
    for (int e = 0; e < universe; ++e) {
        sets[static_cast<std::size_t>(rng.uniform(0, m - 1))].push_back(e);
    }
    const auto chosen = greedy_set_cover(universe, sets);
    CHECK_LE(chosen.size(), m);
}

TEST(3, "no items splits trivially") {
    const auto split = approx_partition(std::vector<long long>{}, 0.5);
    CHECK_EQ(split.cost, 0);
}

TEST(3, "one huge item dominates") {
    const std::vector<long long> w{100, 1, 1};
    const auto split = approx_partition(w, 0.5);
    CHECK_LE(static_cast<double>(split.cost), 1.5 * 100);
}

TEST(3, "eps of one still gives a real partition") {
    const std::vector<long long> w{4, 3, 2, 1};
    const auto split = approx_partition(w, 1.0);
    CHECK_LE(split.cost, 2 * 5);
}

TEST(3, "random instances stay within (1+eps) of the optimum") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 13));
        std::vector<long long> w(n);
        for (auto& x : w) {
            x = rng.uniform(1, 60);
        }
        const double eps = 1.0 / static_cast<double>(rng.uniform(1, 5));
        const auto split = approx_partition(w, eps);
        INFO("round ", round, " eps ", eps);
        const std::set<int> a(split.side_a.begin(), split.side_a.end());
        CHECK_EQ(a.size(), split.side_a.size());
        CHECK(std::is_sorted(split.side_a.begin(), split.side_a.end()));
        long long wa = 0, wb = 0;
        for (int i = 0; i < n; ++i) {
            (a.count(i) ? wa : wb) += w[i];
        }
        CHECK_EQ(split.cost, std::max(wa, wb));
        CHECK_LE(static_cast<double>(split.cost),
                 (1.0 + eps) * static_cast<double>(brute_partition(w)) + 1e-9);
    }
}

PERF(3, "a million items with a small exhaustive phase", 1000) {
    Rng rng;
    std::vector<long long> w(1'000'000);
    for (auto& x : w) {
        x = rng.uniform(1, 1'000'000);
    }
    const auto split = approx_partition(w, 0.1);
    CHECK_GT(split.cost, 0);
}

TEST(4, "one and two cities") {
    CHECK_EQ(approx_tsp({{0}}), std::vector<int>{0});
    CHECK_EQ(approx_tsp({{0, 7}, {7, 0}}), std::vector<int>{0, 1});
}

TEST(4, "random metrics: valid tours within twice the optimum") {
    Rng rng;
    for (int round = 0; round < 80; ++round) {
        const int n = static_cast<int>(rng.uniform(3, 10));
        const auto d = random_metric(rng, n);
        const auto tour = approx_tsp(d);
        INFO("round ", round, " n=", n);
        REQUIRE_EQ(tour.size(), n);
        CHECK_EQ(tour[0], 0);
        std::vector<int> sorted_tour = tour;
        std::sort(sorted_tour.begin(), sorted_tour.end());
        std::vector<int> all(n);
        std::iota(all.begin(), all.end(), 0);
        CHECK_EQ(sorted_tour, all);
        CHECK_LE(tour_cost(d, tour), 2 * held_karp(d));
    }
}

PERF(4, "thousands of cities in O(n^2)", 1500) {
    Rng rng;
    const int n = 4000;
    const auto d = random_metric(rng, n);
    const auto tour = approx_tsp(d);
    CHECK_EQ(tour.size(), n);
}
