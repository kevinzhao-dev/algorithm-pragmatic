#include <algorithm>
#include <cstddef>
#include <queue>
#include <set>
#include <stdexcept>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using EdgeList = std::vector<std::pair<int, int>>;

constexpr std::size_t idx(int value) { return static_cast<std::size_t>(value); }

AdjacencyList make_graph(int n, const EdgeList& edges) {
    AdjacencyList g(idx(n));
    for (const auto& [u, v] : edges) {
        g[idx(u)].push_back(v);
    }
    return g;
}

// A random simple digraph: no self-loops, no duplicate edges.
EdgeList random_simple_edges(Rng& rng, int n, int m) {
    std::set<std::pair<int, int>> chosen;
    for (int i = 0; i < m && n >= 2; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a != b) {
            chosen.emplace(a, b);
        }
    }
    return {chosen.begin(), chosen.end()};
}

SearchTrace model_bfs(const AdjacencyList& sorted, int s) {
    SearchTrace out{{}, std::vector<int>(sorted.size(), kNoParent)};
    std::vector<char> seen(sorted.size(), 0);
    std::queue<int> q;
    seen[idx(s)] = 1;
    q.push(s);
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        out.visit_order.push_back(u);
        for (const int v : sorted[idx(u)]) {
            if (seen[idx(v)] == 0) {
                seen[idx(v)] = 1;
                out.parent[idx(v)] = u;
                q.push(v);
            }
        }
    }
    return out;
}

void model_dfs_visit(const AdjacencyList& sorted, int u, std::vector<char>& seen, SearchTrace& out) {
    out.visit_order.push_back(u);
    seen[idx(u)] = 1;
    for (const int v : sorted[idx(u)]) {
        if (seen[idx(v)] == 0) {
            out.parent[idx(v)] = u;
            model_dfs_visit(sorted, v, seen, out);
        }
    }
}

AdjacencyList sorted_lists(AdjacencyList g) {
    for (auto& adj : g) {
        std::sort(adj.begin(), adj.end());
    }
    return g;
}

SearchTrace model_dfs(const AdjacencyList& sorted, int s) {
    SearchTrace out{{}, std::vector<int>(sorted.size(), kNoParent)};
    std::vector<char> seen(sorted.size(), 0);
    model_dfs_visit(sorted, s, seen, out);
    return out;
}

bool is_acyclic_without(int n, const EdgeList& edges, std::pair<int, int> removed) {
    std::vector<std::vector<char>> reach(idx(n), std::vector<char>(idx(n), 0));
    for (const auto& e : edges) {
        if (e != removed) {
            reach[idx(e.first)][idx(e.second)] = 1;
        }
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
    for (int v = 0; v < n; ++v) {
        if (reach[idx(v)][idx(v)] != 0) {
            return false;
        }
    }
    return true;
}

bool is_acyclic(int n, const EdgeList& edges) { return is_acyclic_without(n, edges, {-1, -1}); }

void check_fix(int n, const EdgeList& edges, const EdgeFix& fix) {
    REQUIRE_EQ(fix.order.size(), n);
    std::vector<int> position(idx(n), -1);
    for (std::size_t i = 0; i < fix.order.size(); ++i) {
        REQUIRE(fix.order[i] >= 0 && fix.order[i] < n);
        REQUIRE_EQ(position[idx(fix.order[i])], -1);
        position[idx(fix.order[i])] = static_cast<int>(i);
    }
    for (const auto& e : edges) {
        if (e != std::pair{fix.from, fix.to}) {
            CHECK_LT(position[idx(e.first)], position[idx(e.second)]);
        }
    }
}

std::vector<std::pair<int, int>> fix_edges(const std::vector<EdgeFix>& fixes) {
    std::vector<std::pair<int, int>> out;
    for (const auto& fix : fixes) {
        out.emplace_back(fix.from, fix.to);
    }
    return out;
}

}  // namespace

TEST(1, "a matrix becomes sorted adjacency lists") {
    const AdjacencyMatrix matrix = {{0, 1, 1}, {0, 0, 0}, {1, 0, 0}};
    CHECK_EQ(to_adjacency_list(matrix), AdjacencyList{{1, 2}, {}, {0}});
}

TEST(1, "the empty graph converts both ways") {
    CHECK(to_adjacency_list({}).empty());
    CHECK(to_adjacency_matrix({}).empty());
}

TEST(1, "adjacency lists become matrices regardless of list order") {
    const AdjacencyList list = {{2, 1}, {}, {0}};
    CHECK_EQ(to_adjacency_matrix(list), AdjacencyMatrix{{0, 1, 1}, {0, 0, 0}, {1, 0, 0}});
}

TEST(1, "malformed matrices are rejected") {
    const AdjacencyMatrix not_square = {{0, 1}};
    CHECK_THROWS_AS(to_adjacency_list(not_square), std::invalid_argument);
    const AdjacencyMatrix jagged = {{0, 1}, {1}};
    CHECK_THROWS_AS(to_adjacency_list(jagged), std::invalid_argument);
    const AdjacencyMatrix bad_value = {{0, 2}, {0, 0}};
    CHECK_THROWS_AS(to_adjacency_list(bad_value), std::invalid_argument);
}

TEST(1, "malformed lists are rejected") {
    const AdjacencyList out_of_range = {{5}};
    CHECK_THROWS_AS(to_adjacency_matrix(out_of_range), std::invalid_argument);
    const AdjacencyList negative = {{-1}};
    CHECK_THROWS_AS(to_adjacency_matrix(negative), std::invalid_argument);
    const AdjacencyList repeated = {{1, 1}, {}};
    CHECK_THROWS_AS(to_adjacency_matrix(repeated), std::invalid_argument);
}

TEST(1, "random graphs round-trip") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        AdjacencyMatrix matrix(idx(n), std::vector<int>(idx(n)));
        for (auto& row : matrix) {
            for (int& cell : row) {
                cell = rng.coin(0.3) ? 1 : 0;
            }
        }
        INFO("round ", round);
        const AdjacencyList list = to_adjacency_list(matrix);
        for (const auto& adj : list) {
            REQUIRE(std::is_sorted(adj.begin(), adj.end()));
        }
        REQUIRE_EQ(to_adjacency_matrix(list), matrix);
    }
}

TEST(2, "bfs and dfs visit the same vertices in different orders") {
    const AdjacencyList g = {{3, 1}, {4}, {0}, {1, 5}, {5}, {}, {}};
    const SearchTrace b = bfs_trace(g, 0);
    CHECK_EQ(b.visit_order, std::vector<int>{0, 1, 3, 4, 5});
    CHECK_EQ(b.parent, std::vector<int>{kNoParent, 0, kNoParent, 0, 1, 3, kNoParent});
    const SearchTrace d = dfs_trace(g, 0);
    CHECK_EQ(d.visit_order, std::vector<int>{0, 1, 4, 5, 3});
    CHECK_EQ(d.parent, std::vector<int>{kNoParent, 0, kNoParent, 0, 1, 4, kNoParent});
}

TEST(2, "traces only contain what the source can reach") {
    const AdjacencyList g = {{}, {0}, {1}};
    CHECK_EQ(bfs_trace(g, 0).visit_order, std::vector<int>{0});
    CHECK_EQ(dfs_trace(g, 0).visit_order, std::vector<int>{0});
    CHECK_EQ(bfs_trace(g, 2).visit_order, std::vector<int>{2, 1, 0});
}

TEST(2, "dfs goes deep before visiting the next sibling") {
    const AdjacencyList g = {{1, 2}, {3}, {}, {}};
    CHECK_EQ(dfs_trace(g, 0).visit_order, std::vector<int>{0, 1, 3, 2});
}

TEST(2, "a sibling already claimed through a deeper branch keeps that parent") {
    const AdjacencyList g = {{1, 2}, {2}, {}};
    CHECK_EQ(dfs_trace(g, 0).parent, std::vector<int>{kNoParent, 0, 1});
    CHECK_EQ(bfs_trace(g, 0).parent, std::vector<int>{kNoParent, 0, 0});
}

TEST(2, "an unknown source throws out_of_range") {
    const AdjacencyList g(2);
    CHECK_THROWS_AS(bfs_trace(g, 2), std::out_of_range);
    CHECK_THROWS_AS(dfs_trace(g, -1), std::out_of_range);
}

TEST(2, "unsorted lists are still processed in ascending order") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 16));
        EdgeList edges = random_simple_edges(rng, n, static_cast<int>(rng.uniform(0, 3 * n)));
        rng.shuffle(edges);
        const AdjacencyList g = make_graph(n, edges);
        const AdjacencyList sorted = sorted_lists(g);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round, " edges=", edges, " s=", s);
        const SearchTrace bfs_expected = model_bfs(sorted, s);
        const SearchTrace bfs_got = bfs_trace(g, s);
        REQUIRE_EQ(bfs_got.visit_order, bfs_expected.visit_order);
        REQUIRE_EQ(bfs_got.parent, bfs_expected.parent);
        const SearchTrace dfs_expected = model_dfs(sorted, s);
        const SearchTrace dfs_got = dfs_trace(g, s);
        REQUIRE_EQ(dfs_got.visit_order, dfs_expected.visit_order);
        REQUIRE_EQ(dfs_got.parent, dfs_expected.parent);
    }
}

TEST(3, "an acyclic graph has nothing to fix") {
    const AdjacencyList g = {{1, 2}, {2}, {}};
    CHECK(single_edge_fixes(g).empty());
    CHECK(single_edge_fixes({}).empty());
}

TEST(3, "every edge of the only cycle is a fix") {
    const EdgeList edges = {{0, 1}, {1, 2}, {2, 0}, {2, 3}};
    const AdjacencyList g = make_graph(4, edges);
    const auto fixes = single_edge_fixes(g);
    CHECK_EQ(fix_edges(fixes), (std::vector<std::pair<int, int>>{{0, 1}, {1, 2}, {2, 0}}));
    for (const auto& fix : fixes) {
        check_fix(4, edges, fix);
    }
}

TEST(3, "two vertex-disjoint cycles cannot be fixed by one edge") {
    const AdjacencyList g = make_graph(4, {{0, 1}, {1, 0}, {2, 3}, {3, 2}});
    CHECK(single_edge_fixes(g).empty());
}

TEST(3, "only the edge shared by every cycle qualifies") {
    // Two cycles through the edge 0 -> 1, plus an unrelated forward edge.
    const EdgeList edges = {{0, 1}, {1, 2}, {2, 0}, {1, 3}, {3, 0}, {2, 4}};
    const AdjacencyList g = make_graph(5, edges);
    const auto fixes = single_edge_fixes(g);
    CHECK_EQ(fix_edges(fixes), (std::vector<std::pair<int, int>>{{0, 1}}));
    check_fix(5, edges, fixes.at(0));
}

TEST(3, "a two-vertex cycle has two fixes with different orders") {
    const EdgeList edges = {{0, 1}, {1, 0}};
    const auto fixes = single_edge_fixes(make_graph(2, edges));
    CHECK_EQ(fix_edges(fixes), (std::vector<std::pair<int, int>>{{0, 1}, {1, 0}}));
    CHECK_EQ(fixes.at(0).order, std::vector<int>{1, 0});
    CHECK_EQ(fixes.at(1).order, std::vector<int>{0, 1});
}

TEST(3, "random graphs match trying every edge") {
    Rng rng;
    int with_fix = 0;
    for (int round = 0; round < 600; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const int m = static_cast<int>(rng.uniform(0, 2 * n));
        const EdgeList edges = random_simple_edges(rng, n, m);
        INFO("round ", round, " n=", n, " edges=", edges);
        const auto fixes = single_edge_fixes(make_graph(n, edges));
        std::vector<std::pair<int, int>> expected;
        if (!is_acyclic(n, edges)) {
            for (const auto& e : edges) {
                if (is_acyclic_without(n, edges, e)) {
                    expected.push_back(e);
                }
            }
        }
        std::sort(expected.begin(), expected.end());
        REQUIRE_EQ(fix_edges(fixes), expected);
        with_fix += expected.empty() ? 0 : 1;
        for (const auto& fix : fixes) {
            check_fix(n, edges, fix);
        }
    }
    CHECK_GT(with_fix, 100);
}

// Testing all 250,000 edges separately costs about 250,000 full sorts instead of 500.
PERF(3, "only edges on one cycle are tested", 2000) {
    Rng rng;
    const int cycle_length = 500;
    const int noise = 1000;
    const int n = cycle_length + noise;
    std::vector<int> label = rng.permutation(idx(n));
    AdjacencyList g(idx(n));
    const auto add = [&](int a, int b) { g[idx(label[idx(a)])].push_back(label[idx(b)]); };
    for (int i = 0; i < cycle_length; ++i) {
        add(i, (i + 1) % cycle_length);
    }
    std::set<std::pair<int, int>> used;
    while (used.size() < 250'000) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a < b && b >= cycle_length && used.emplace(a, b).second) {
            add(a, b);
        }
    }
    const auto fixes = single_edge_fixes(g);
    CHECK_EQ(fixes.size(), cycle_length);
}
