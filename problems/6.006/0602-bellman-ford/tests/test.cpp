#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

constexpr std::int64_t INF = kUnreachable;
constexpr std::int64_t NEG = kNegativeInfinity;

std::vector<Edge> random_edges(Rng& rng, int n, int m, std::int64_t low, std::int64_t high) {
    std::vector<Edge> edges;
    for (int i = 0; i < m; ++i) {
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                         static_cast<int>(rng.uniform(0, n - 1)), rng.uniform(low, high)});
    }
    return edges;
}

// Non-negative base weights rewritten through potentials: negative edges, but no negative cycle.
std::vector<Edge> random_potential_edges(Rng& rng, int n, int m, std::int64_t max_weight) {
    std::vector<std::int64_t> potential(idx(n));
    for (auto& h : potential) {
        h = rng.uniform(-40, 40);
    }
    std::vector<Edge> edges;
    for (int i = 0; i < m; ++i) {
        const int u = static_cast<int>(rng.uniform(0, n - 1));
        const int v = static_cast<int>(rng.uniform(0, n - 1));
        edges.push_back({u, v, rng.uniform(0, max_weight) + potential[idx(u)] - potential[idx(v)]});
    }
    return edges;
}

// reach[a][b]: a path with zero or more edges exists from a to b.
std::vector<std::vector<char>> closure(int n, const std::vector<Edge>& edges) {
    std::vector<std::vector<char>> reach(idx(n), std::vector<char>(idx(n), 0));
    for (int v = 0; v < n; ++v) {
        reach[idx(v)][idx(v)] = 1;
    }
    for (const Edge& e : edges) {
        reach[idx(e.from)][idx(e.to)] = 1;
    }
    for (int k = 0; k < n; ++k) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                if (reach[idx(i)][idx(k)] != 0 && reach[idx(k)][idx(j)] != 0) {
                    reach[idx(i)][idx(j)] = 1;
                }
            }
        }
    }
    return reach;
}

std::map<std::pair<int, int>, std::int64_t> lightest_edges(const std::vector<Edge>& edges) {
    std::map<std::pair<int, int>, std::int64_t> lightest;
    for (const Edge& e : edges) {
        const auto [slot, inserted] = lightest.try_emplace({e.from, e.to}, e.weight);
        if (!inserted) {
            slot->second = std::min(slot->second, e.weight);
        }
    }
    return lightest;
}

// Enumerates every simple cycle (including self-loops), reporting its minimum possible weight.
void enumerate_cycles(int start, int at, std::int64_t weight, std::vector<int>& path,
                      const std::map<std::pair<int, int>, std::int64_t>& lightest,
                      const std::vector<std::vector<int>>& successors, std::set<int>& on_path,
                      std::vector<std::vector<int>>& negative) {
    for (const int next : successors[idx(at)]) {
        const std::int64_t w = weight + lightest.at({at, next});
        if (next == start) {
            if (w < 0) {
                negative.push_back(path);
            }
        } else if (next > start && on_path.count(next) == 0) {
            on_path.insert(next);
            path.push_back(next);
            enumerate_cycles(start, next, w, path, lightest, successors, on_path, negative);
            path.pop_back();
            on_path.erase(next);
        }
    }
}

std::vector<std::vector<int>> negative_simple_cycles(int n, const std::vector<Edge>& edges) {
    const auto lightest = lightest_edges(edges);
    std::vector<std::vector<int>> successors(idx(n));
    for (const auto& [key, weight] : lightest) {
        successors[idx(key.first)].push_back(key.second);
    }
    std::vector<std::vector<int>> negative;
    for (int start = 0; start < n; ++start) {
        std::vector<int> path{start};
        std::set<int> on_path{start};
        enumerate_cycles(start, start, 0, path, lightest, successors, on_path, negative);
    }
    return negative;
}

// Exact DP over walks of at most n-1 edges, plus closure-based -infinity marking.
std::vector<std::int64_t> brute_distances(int n, const std::vector<Edge>& edges, int source) {
    std::vector<std::int64_t> dist(idx(n), INF);
    dist[idx(source)] = 0;
    for (int round = 1; round < n; ++round) {
        std::vector<std::int64_t> next = dist;
        for (const Edge& e : edges) {
            if (dist[idx(e.from)] != INF) {
                next[idx(e.to)] = std::min(next[idx(e.to)], dist[idx(e.from)] + e.weight);
            }
        }
        dist = std::move(next);
    }
    const auto reach = closure(n, edges);
    for (const auto& cycle : negative_simple_cycles(n, edges)) {
        if (reach[idx(source)][idx(cycle.front())] == 0) {
            continue;
        }
        for (int v = 0; v < n; ++v) {
            if (reach[idx(cycle.front())][idx(v)] != 0) {
                dist[idx(v)] = NEG;
            }
        }
    }
    return dist;
}

void check_cycle(int n, const std::vector<Edge>& edges, int source, const std::vector<int>& cycle) {
    REQUIRE(!cycle.empty());
    const std::set<int> distinct(cycle.begin(), cycle.end());
    CHECK_EQ(distinct.size(), cycle.size());
    const auto lightest = lightest_edges(edges);
    const auto reach = closure(n, edges);
    CHECK_NE(reach[idx(source)][idx(cycle.front())], 0);
    std::int64_t total = 0;
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        const int u = cycle[i];
        const int v = cycle[(i + 1) % cycle.size()];
        REQUIRE(lightest.count({u, v}) != 0);
        total += lightest.at({u, v});
    }
    CHECK_LT(total, 0);
}

void check_arbitrage(int n, const std::vector<Quote>& quotes, const std::vector<int>& cycle) {
    REQUIRE(!cycle.empty());
    const std::set<int> distinct(cycle.begin(), cycle.end());
    CHECK_EQ(distinct.size(), cycle.size());
    double product = 1.0;
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        const int a = cycle[i];
        const int b = cycle[(i + 1) % cycle.size()];
        REQUIRE(a >= 0 && a < n);
        double best = -1.0;
        for (const Quote& q : quotes) {
            if (q.from == a && q.to == b) {
                best = std::max(best, q.rate);
            }
        }
        REQUIRE(best > 0.0);
        product *= best;
    }
    CHECK_GT(product, 1.0 + 1e-9);
}

}  // namespace

TEST(1, "a single vertex is at distance zero") {
    CHECK_EQ(bellman_ford(1, {}, 0), std::vector<std::int64_t>{0});
}

TEST(1, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(bellman_ford(2, {}, 2), std::out_of_range);
    CHECK_THROWS_AS(bellman_ford(2, {}, -1), std::out_of_range);
}

TEST(1, "negative edges without a cycle are handled") {
    const std::vector<Edge> edges = {{0, 1, 5}, {1, 2, -3}, {0, 2, 4}};
    CHECK_EQ(bellman_ford(3, edges, 0), std::vector<std::int64_t>{0, 5, 2});
}

TEST(1, "unreachable vertices stay at kUnreachable") {
    const std::vector<Edge> edges = {{0, 1, 1}, {2, 0, -5}};
    CHECK_EQ(bellman_ford(3, edges, 0), std::vector<std::int64_t>{0, 1, INF});
}

TEST(1, "edges out of unreachable vertices are skipped without overflow") {
    const std::vector<Edge> edges = {{1, 2, -1000}, {0, 3, 7}};
    CHECK_EQ(bellman_ford(4, edges, 0), std::vector<std::int64_t>{0, INF, INF, 7});
}

TEST(1, "parallel edges and non-negative self-loops are harmless") {
    const std::vector<Edge> edges = {{0, 1, 9}, {0, 1, 2}, {1, 1, 0}, {1, 1, 5}};
    CHECK_EQ(bellman_ford(2, edges, 0), std::vector<std::int64_t>{0, 2});
}

TEST(1, "a positive cycle is not a negative cycle") {
    const std::vector<Edge> edges = {{0, 1, 3}, {1, 2, -2}, {2, 1, 3}};
    CHECK_EQ(bellman_ford(3, edges, 0), std::vector<std::int64_t>{0, 3, 1});
}

TEST(1, "edge order does not change the answer") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        std::vector<Edge> edges = random_potential_edges(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), 20);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round);
        REQUIRE_EQ(bellman_ford(n, edges, s), brute_distances(n, edges, s));
        rng.shuffle(edges);
        REQUIRE_EQ(bellman_ford(n, edges, s), brute_distances(n, edges, s));
    }
}

// Edges are listed back to front, so an in-place sweep extends the path by one hop per round.
PERF(1, "V rounds over a long chain in hostile order", 1200) {
    const int n = 4000;
    std::vector<Edge> edges;
    for (int v = n - 1; v >= 1; --v) {
        edges.push_back({v - 1, v, -1});
        edges.push_back({v - 1, v, 7});
    }
    for (int v = 0; v + 2 < n; ++v) {
        edges.push_back({v, v + 2, 5});
    }
    const auto dist = bellman_ford(n, edges, 0);
    CHECK_EQ(dist[idx(n - 1)], -(n - 1));
}

TEST(2, "a negative cycle makes everything after it negative infinity") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 2, -3}, {2, 1, 1}, {2, 3, 4}};
    CHECK_EQ(bellman_ford(5, edges, 0), std::vector<std::int64_t>{0, NEG, NEG, NEG, INF});
}

TEST(2, "vertices before the negative cycle keep finite distances") {
    const std::vector<Edge> edges = {{0, 1, 2}, {1, 2, 1}, {2, 3, -2}, {3, 2, 1}};
    CHECK_EQ(bellman_ford(4, edges, 0), std::vector<std::int64_t>{0, 2, NEG, NEG});
}

TEST(2, "a negative self-loop is a negative cycle") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 1, -1}, {1, 2, 5}};
    CHECK_EQ(bellman_ford(3, edges, 0), std::vector<std::int64_t>{0, NEG, NEG});
}

TEST(2, "a negative cycle the source cannot reach changes nothing") {
    const std::vector<Edge> edges = {{0, 1, 2}, {2, 3, -5}, {3, 2, 1}};
    CHECK_EQ(bellman_ford(4, edges, 0), std::vector<std::int64_t>{0, 2, INF, INF});
}

TEST(2, "a vertex can be infected through a later edge only") {
    // Vertex 3 is reached from the source first, then again from the cycle.
    const std::vector<Edge> edges = {{0, 3, 1}, {0, 1, 1}, {1, 2, -3}, {2, 1, 1}, {2, 3, 1}};
    CHECK_EQ(bellman_ford(4, edges, 0), std::vector<std::int64_t>{0, NEG, NEG, NEG});
}

TEST(2, "random graphs with possible negative cycles match enumeration") {
    Rng rng;
    int with_cycle = 0;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        const std::vector<Edge> edges = rng.coin() ? random_edges(rng, n, static_cast<int>(rng.uniform(0, 2 * n)), -5, 8)
                                                   : random_edges(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), -3, 3);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        const auto expected = brute_distances(n, edges, s);
        REQUIRE_EQ(bellman_ford(n, edges, s), expected);
        with_cycle += std::find(expected.begin(), expected.end(), NEG) != expected.end() ? 1 : 0;
    }
    CHECK_GT(with_cycle, 100);
}

TEST(3, "a graph without negative cycles returns nullopt") {
    const std::vector<Edge> edges = {{0, 1, 5}, {1, 2, -3}, {2, 0, 2}};
    CHECK_EQ(find_negative_cycle(3, edges, 0), std::optional<std::vector<int>>());
}

TEST(3, "a triangle of total weight -1 is found") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 2, 1}, {2, 0, -3}};
    const auto cycle = find_negative_cycle(3, edges, 0);
    REQUIRE(cycle.has_value());
    check_cycle(3, edges, 0, *cycle);
    CHECK_EQ(cycle->size(), 3);
}

TEST(3, "a negative self-loop is a one-vertex cycle") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 1, -2}};
    CHECK_EQ(find_negative_cycle(2, edges, 0), std::optional(std::vector<int>{1}));
}

TEST(3, "a cycle the source cannot reach is ignored") {
    const std::vector<Edge> edges = {{0, 1, 1}, {2, 3, -2}, {3, 2, 1}};
    CHECK_EQ(find_negative_cycle(4, edges, 0), std::optional<std::vector<int>>());
}

TEST(3, "a tail leading into the cycle is not returned") {
    const std::vector<Edge> edges = {{0, 1, 1}, {1, 2, 1}, {2, 3, -3}, {3, 2, 1}};
    const auto cycle = find_negative_cycle(4, edges, 0);
    REQUIRE(cycle.has_value());
    check_cycle(4, edges, 0, *cycle);
    CHECK_EQ(cycle->size(), 2);
}

TEST(3, "a bad source throws out_of_range") {
    CHECK_THROWS_AS(find_negative_cycle(2, {}, 5), std::out_of_range);
}

TEST(3, "random graphs agree with enumeration of simple cycles") {
    Rng rng;
    int found = 0;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        const std::vector<Edge> edges = random_edges(rng, n, static_cast<int>(rng.uniform(0, 3 * n)), -4, 6);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " n=", n, " s=", s);
        const auto reach = closure(n, edges);
        bool expected = false;
        for (const auto& candidate : negative_simple_cycles(n, edges)) {
            expected = expected || reach[idx(s)][idx(candidate.front())] != 0;
        }
        const auto cycle = find_negative_cycle(n, edges, s);
        REQUIRE_EQ(cycle.has_value(), expected);
        if (cycle) {
            check_cycle(n, edges, s, *cycle);
            ++found;
        }
    }
    CHECK_GT(found, 150);
}

PERF(3, "cycle extraction after V hostile rounds", 1200) {
    const int n = 4000;
    std::vector<Edge> edges;
    for (int v = n - 1; v >= 1; --v) {
        edges.push_back({v - 1, v, 1});
    }
    edges.push_back({n - 1, n - 3, -5});
    const auto cycle = find_negative_cycle(n, edges, 0);
    REQUIRE(cycle.has_value());
    CHECK_EQ(cycle->size(), 3);
}

TEST(4, "no quotes means no arbitrage") {
    CHECK_EQ(find_arbitrage(3, {}), std::optional<std::vector<int>>());
}

TEST(4, "non-positive rates are rejected") {
    const std::vector<Quote> quotes = {{0, 1, 0.0}};
    CHECK_THROWS_AS(find_arbitrage(2, quotes), std::invalid_argument);
    const std::vector<Quote> negative = {{0, 1, -1.5}};
    CHECK_THROWS_AS(find_arbitrage(2, negative), std::invalid_argument);
}

TEST(4, "a profitable triangle is reported") {
    const std::vector<Quote> quotes = {{0, 1, 2.0}, {1, 2, 2.0}, {2, 0, 0.3}};
    const auto cycle = find_arbitrage(3, quotes);
    REQUIRE(cycle.has_value());
    check_arbitrage(3, quotes, *cycle);
}

TEST(4, "losing every round trip is not arbitrage") {
    const std::vector<Quote> quotes = {{0, 1, 2.0}, {1, 0, 0.4}, {1, 2, 0.5}, {2, 1, 1.5}};
    CHECK_EQ(find_arbitrage(3, quotes), std::optional<std::vector<int>>());
}

TEST(4, "opportunities in a disconnected market are still found") {
    const std::vector<Quote> quotes = {{0, 1, 2.0}, {1, 0, 0.4}, {2, 3, 2.0}, {3, 2, 0.6}};
    const auto cycle = find_arbitrage(4, quotes);
    REQUIRE(cycle.has_value());
    check_arbitrage(4, quotes, *cycle);
}

TEST(4, "perfectly consistent rates are not arbitrage despite rounding") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int n = static_cast<int>(rng.uniform(3, 30));
        std::vector<double> value(idx(n));
        for (double& v : value) {
            v = std::exp(rng.real() * 12.0 - 6.0);
        }
        std::vector<Quote> quotes;
        for (int a = 0; a < n; ++a) {
            for (int b = 0; b < n; ++b) {
                if (a != b && rng.coin(0.7)) {
                    quotes.push_back({a, b, value[idx(a)] / value[idx(b)]});
                }
            }
        }
        rng.shuffle(quotes);
        INFO("round ", round, " n=", n);
        CHECK_EQ(find_arbitrage(n, quotes), std::optional<std::vector<int>>());
    }
}

TEST(4, "a single mispriced quote in a large table is detected") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        const int n = static_cast<int>(rng.uniform(4, 40));
        std::vector<double> value(idx(n));
        for (double& v : value) {
            v = std::exp(rng.real() * 8.0 - 4.0);
        }
        std::vector<Quote> quotes;
        for (int a = 0; a < n; ++a) {
            for (int b = 0; b < n; ++b) {
                if (a != b) {
                    quotes.push_back({a, b, value[idx(a)] / value[idx(b)]});
                }
            }
        }
        const std::size_t mispriced = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(quotes.size()) - 1));
        quotes[mispriced].rate *= 1.001;
        rng.shuffle(quotes);
        INFO("round ", round, " n=", n);
        const auto cycle = find_arbitrage(n, quotes);
        REQUIRE(cycle.has_value());
        check_arbitrage(n, quotes, *cycle);
    }
}

TEST(4, "random small markets match enumeration") {
    Rng rng;
    int found = 0;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 6));
        std::vector<Quote> quotes;
        const int count = static_cast<int>(rng.uniform(0, 2 * n));
        for (int i = 0; i < count; ++i) {
            quotes.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                              static_cast<int>(rng.uniform(0, n - 1)),
                              std::exp((rng.real() - 0.5) * 2.0)});
        }
        // Brute force: best rate per ordered pair, then every ordered simple cycle.
        std::map<std::pair<int, int>, double> best;
        for (const Quote& q : quotes) {
            auto& slot = best[{q.from, q.to}];
            slot = std::max(slot, q.rate);
        }
        std::vector<int> order(idx(n));
        for (int i = 0; i < n; ++i) {
            order[idx(i)] = i;
        }
        double best_product = 0.0;
        for (std::uint32_t mask = 1; mask < (1u << n); ++mask) {
            std::vector<int> members;
            for (int v = 0; v < n; ++v) {
                if ((mask >> v) & 1u) {
                    members.push_back(v);
                }
            }
            std::sort(members.begin(), members.end());
            do {
                double product = 1.0;
                for (std::size_t i = 0; i < members.size() && product > 0.0; ++i) {
                    const auto it = best.find({members[i], members[(i + 1) % members.size()]});
                    product = it == best.end() ? 0.0 : product * it->second;
                }
                best_product = std::max(best_product, product);
            } while (std::next_permutation(members.begin(), members.end()));
        }
        // Margins are checked so rounding cannot decide the expected answer.
        if (best_product > 1.0 - 1e-6 && best_product < 1.0 + 1e-6) {
            continue;
        }
        INFO("round ", round, " n=", n, " best=", best_product);
        const auto cycle = find_arbitrage(n, quotes);
        REQUIRE_EQ(cycle.has_value(), best_product > 1.0);
        if (cycle) {
            check_arbitrage(n, quotes, *cycle);
            ++found;
        }
    }
    CHECK_GT(found, 100);
}
