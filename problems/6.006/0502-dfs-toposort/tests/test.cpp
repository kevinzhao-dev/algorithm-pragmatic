#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <set>
#include <stdexcept>
#include <utility>
#include <variant>
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

EdgeList random_edges(Rng& rng, int n, int m) {
    EdgeList edges;
    for (int i = 0; i < m; ++i) {
        edges.emplace_back(static_cast<int>(rng.uniform(0, n - 1)),
                           static_cast<int>(rng.uniform(0, n - 1)));
    }
    return edges;
}

// Edges only go from earlier to later positions of a shuffled order, so the graph is acyclic.
EdgeList random_dag_edges(Rng& rng, int n, int m) {
    std::vector<int> order = rng.permutation(idx(n));
    EdgeList edges;
    for (int i = 0; i < m && n >= 2; ++i) {
        int a = static_cast<int>(rng.uniform(0, n - 1));
        int b = static_cast<int>(rng.uniform(0, n - 1));
        if (a == b) {
            continue;
        }
        if (a > b) {
            std::swap(a, b);
        }
        edges.emplace_back(order[idx(a)], order[idx(b)]);
    }
    return edges;
}

void model_visit(const AdjacencyList& g, int u, std::vector<char>& seen, DfsForest& out) {
    for (const int v : g[idx(u)]) {
        if (seen[idx(v)] == 0) {
            seen[idx(v)] = 1;
            out.parent[idx(v)] = u;
            model_visit(g, v, seen, out);
        }
    }
    out.finish_order.push_back(u);
}

// The recursive textbook DFS, which is the behaviour the solution has to reproduce iteratively.
DfsForest model_dfs(const AdjacencyList& g, const std::vector<int>& roots) {
    DfsForest out{std::vector<int>(g.size(), kNoParent), {}};
    std::vector<char> seen(g.size(), 0);
    for (const int root : roots) {
        if (seen[idx(root)] == 0) {
            seen[idx(root)] = 1;
            model_visit(g, root, seen, out);
        }
    }
    return out;
}

std::vector<int> iota_vector(int n) {
    std::vector<int> values(idx(n));
    for (int i = 0; i < n; ++i) {
        values[idx(i)] = i;
    }
    return values;
}

// Warshall closure: reach[u][v] says there is a path with at least one edge from u to v.
std::vector<std::vector<char>> reachability(int n, const EdgeList& edges) {
    std::vector<std::vector<char>> reach(idx(n), std::vector<char>(idx(n), 0));
    for (const auto& [u, v] : edges) {
        reach[idx(u)][idx(v)] = 1;
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

bool brute_has_cycle(int n, const EdgeList& edges) {
    const auto reach = reachability(n, edges);
    for (int v = 0; v < n; ++v) {
        if (reach[idx(v)][idx(v)] != 0) {
            return true;
        }
    }
    return false;
}

void check_order(const AdjacencyList& g, const std::vector<int>& order) {
    REQUIRE_EQ(order.size(), g.size());
    std::vector<int> position(g.size(), -1);
    for (std::size_t i = 0; i < order.size(); ++i) {
        REQUIRE(order[i] >= 0 && order[i] < static_cast<int>(g.size()));
        REQUIRE_EQ(position[idx(order[i])], -1);
        position[idx(order[i])] = static_cast<int>(i);
    }
    for (int u = 0; u < static_cast<int>(g.size()); ++u) {
        for (const int v : g[idx(u)]) {
            CHECK_LT(position[idx(u)], position[idx(v)]);
        }
    }
}

void check_cycle(const AdjacencyList& g, const std::vector<int>& cycle) {
    REQUIRE(!cycle.empty());
    std::set<int> distinct(cycle.begin(), cycle.end());
    CHECK_EQ(distinct.size(), cycle.size());
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        const int u = cycle[i];
        const int v = cycle[(i + 1) % cycle.size()];
        REQUIRE(u >= 0 && u < static_cast<int>(g.size()));
        const auto& adjacency = g[idx(u)];
        CHECK(std::find(adjacency.begin(), adjacency.end(), v) != adjacency.end());
    }
}

}  // namespace

TEST(1, "dfs from an isolated vertex finishes only that vertex") {
    const AdjacencyList g(3);
    const DfsForest out = dfs(g, 1);
    CHECK_EQ(out.finish_order, std::vector<int>{1});
    CHECK_EQ(out.parent, std::vector<int>{kNoParent, kNoParent, kNoParent});
}

TEST(1, "a bad source throws out_of_range") {
    const AdjacencyList g(2);
    CHECK_THROWS_AS(dfs(g, 2), std::out_of_range);
    CHECK_THROWS_AS(dfs(g, -1), std::out_of_range);
}

TEST(1, "a chain finishes from the far end back") {
    const AdjacencyList g = make_graph(4, {{0, 1}, {1, 2}, {2, 3}});
    const DfsForest out = dfs(g, 0);
    CHECK_EQ(out.finish_order, std::vector<int>{3, 2, 1, 0});
    CHECK_EQ(out.parent, std::vector<int>{kNoParent, 0, 1, 2});
}

TEST(1, "neighbours are tried in adjacency order") {
    const AdjacencyList g = make_graph(5, {{0, 2}, {0, 1}, {2, 3}, {1, 3}, {3, 4}});
    const DfsForest out = dfs(g, 0);
    CHECK_EQ(out.parent, std::vector<int>{kNoParent, 0, 0, 2, 3});
    CHECK_EQ(out.finish_order, std::vector<int>{4, 3, 2, 1, 0});
}

TEST(1, "a vertex is claimed by whoever reaches it first in the dfs, not by the shallower edge") {
    const AdjacencyList g = make_graph(4, {{0, 1}, {0, 2}, {1, 2}, {2, 3}});
    const DfsForest out = dfs(g, 0);
    CHECK_EQ(out.parent, std::vector<int>{kNoParent, 0, 1, 2});
}

TEST(1, "unreachable vertices are left out") {
    const AdjacencyList g = make_graph(4, {{0, 1}, {2, 0}, {2, 3}});
    const DfsForest out = dfs(g, 0);
    CHECK_EQ(out.finish_order, std::vector<int>{1, 0});
    CHECK_EQ(out.parent[idx(3)], kNoParent);
}

TEST(1, "self-loops and parallel edges are harmless") {
    const AdjacencyList g = make_graph(2, {{0, 0}, {0, 1}, {0, 1}, {1, 1}});
    const DfsForest out = dfs(g, 0);
    CHECK_EQ(out.finish_order, std::vector<int>{1, 0});
}

TEST(1, "random graphs match the recursive model exactly") {
    Rng rng;
    for (int round = 0; round < 1500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 25));
        const AdjacencyList g = make_graph(n, random_edges(rng, n, static_cast<int>(rng.uniform(0, 3 * n))));
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        INFO("round ", round);
        const DfsForest out = dfs(g, s);
        const DfsForest expected = model_dfs(g, {s});
        REQUIRE_EQ(out.parent, expected.parent);
        REQUIRE_EQ(out.finish_order, expected.finish_order);
    }
}

TEST(1, "a chain of half a million vertices does not overflow the stack") {
    const int n = 500'000;
    AdjacencyList g(idx(n));
    for (int i = 0; i + 1 < n; ++i) {
        g[idx(i)].push_back(i + 1);
    }
    const DfsForest out = dfs(g, 0);
    CHECK_EQ(out.finish_order.size(), n);
    CHECK_EQ(out.finish_order.front(), n - 1);
}

// A stack that is a std::vector scanned for membership is quadratic on the long chain.
PERF(1, "dfs is linear", 1500) {
    Rng rng;
    const int n = 500'000;
    AdjacencyList g(idx(n));
    for (int i = 0; i + 1 < n; ++i) {
        g[idx(i)].push_back(i + 1);
    }
    for (int i = 0; i < 1'000'000; ++i) {
        g[idx(static_cast<int>(rng.uniform(0, n - 1)))].push_back(static_cast<int>(rng.uniform(0, n - 1)));
    }
    long long total = 0;
    for (int s : {0, 1, 2}) {
        total += static_cast<long long>(dfs(g, s).finish_order.size());
    }
    CHECK_GT(total, 0);
}

TEST(2, "full_dfs reaches every vertex and restarts in id order") {
    const AdjacencyList g = make_graph(5, {{1, 0}, {3, 4}});
    const DfsForest out = full_dfs(g);
    CHECK_EQ(out.finish_order, std::vector<int>{0, 1, 2, 4, 3});
    CHECK_EQ(out.parent, std::vector<int>{kNoParent, kNoParent, kNoParent, kNoParent, 3});
}

TEST(2, "an empty graph is fine") {
    const AdjacencyList g;
    const DfsForest out = full_dfs(g);
    CHECK(out.finish_order.empty());
    CHECK(connected_components(g).empty());
}

TEST(2, "connected components are labelled by first appearance") {
    const AdjacencyList g = make_graph(6, {{0, 4}, {4, 0}, {1, 3}, {3, 1}, {3, 5}, {5, 3}});
    CHECK_EQ(connected_components(g), std::vector<int>{0, 1, 2, 1, 0, 1});
}

TEST(2, "isolated vertices are their own components") {
    const AdjacencyList g(4);
    CHECK_EQ(connected_components(g), std::vector<int>{0, 1, 2, 3});
}

TEST(2, "random graphs match the recursive model and a label-propagation brute force") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 25));
        const int m = static_cast<int>(rng.uniform(0, n));
        EdgeList half = random_edges(rng, n, m);
        EdgeList edges;
        for (const auto& [a, b] : half) {
            edges.emplace_back(a, b);
            edges.emplace_back(b, a);
        }
        const AdjacencyList g = make_graph(n, edges);
        INFO("round ", round);
        const DfsForest out = full_dfs(g);
        const DfsForest expected = model_dfs(g, iota_vector(n));
        REQUIRE_EQ(out.parent, expected.parent);
        REQUIRE_EQ(out.finish_order, expected.finish_order);

        std::vector<int> label = iota_vector(n);
        for (int pass = 0; pass < n; ++pass) {
            for (const auto& [a, b] : edges) {
                label[idx(a)] = label[idx(b)] = std::min(label[idx(a)], label[idx(b)]);
            }
        }
        const std::vector<int> ids = connected_components(g);
        std::vector<int> renumbered(idx(n), -1);
        int next = 0;
        for (int v = 0; v < n; ++v) {
            if (renumbered[idx(label[idx(v)])] == -1) {
                renumbered[idx(label[idx(v)])] = next++;
            }
            REQUIRE_EQ(ids[idx(v)], renumbered[idx(label[idx(v)])]);
        }
    }
}

PERF(2, "full_dfs and components are linear", 2000) {
    Rng rng;
    const int n = 500'000;
    AdjacencyList g(idx(n));
    for (int i = 0; i < 750'000; ++i) {
        const int a = static_cast<int>(rng.uniform(0, n - 1));
        const int b = static_cast<int>(rng.uniform(0, n - 1));
        g[idx(a)].push_back(b);
        g[idx(b)].push_back(a);
    }
    CHECK_EQ(full_dfs(g).finish_order.size(), n);
    CHECK_EQ(connected_components(g).size(), n);
}

TEST(3, "the empty graph has an empty order") {
    const auto result = topological_sort({});
    const auto* order = std::get_if<TopologicalOrder>(&result);
    REQUIRE(order != nullptr);
    CHECK(order->vertices.empty());
}

TEST(3, "a self-loop is a cycle of length one") {
    const AdjacencyList g = make_graph(2, {{0, 1}, {1, 1}});
    const auto result = topological_sort(g);
    const auto* cycle = std::get_if<DirectedCycle>(&result);
    REQUIRE(cycle != nullptr);
    CHECK_EQ(cycle->vertices, std::vector<int>{1});
}

TEST(3, "a diamond sorts with all edges forward") {
    const AdjacencyList g = make_graph(4, {{3, 1}, {3, 2}, {1, 0}, {2, 0}});
    const auto result = topological_sort(g);
    const auto* order = std::get_if<TopologicalOrder>(&result);
    REQUIRE(order != nullptr);
    check_order(g, order->vertices);
}

TEST(3, "a two-cycle is reported") {
    const AdjacencyList g = make_graph(2, {{0, 1}, {1, 0}});
    const auto result = topological_sort(g);
    const auto* cycle = std::get_if<DirectedCycle>(&result);
    REQUIRE(cycle != nullptr);
    check_cycle(g, cycle->vertices);
    CHECK_EQ(cycle->vertices.size(), 2);
}

TEST(3, "a tail leading into a cycle is not part of the returned cycle") {
    const AdjacencyList g = make_graph(5, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 2}});
    const auto result = topological_sort(g);
    const auto* cycle = std::get_if<DirectedCycle>(&result);
    REQUIRE(cycle != nullptr);
    check_cycle(g, cycle->vertices);
    CHECK_EQ(cycle->vertices.size(), 3);
}

TEST(3, "a cycle that only a later root can reach is still found") {
    const AdjacencyList g = make_graph(5, {{0, 1}, {3, 4}, {4, 3}});
    const auto result = topological_sort(g);
    const auto* cycle = std::get_if<DirectedCycle>(&result);
    REQUIRE(cycle != nullptr);
    check_cycle(g, cycle->vertices);
}

TEST(3, "a finished vertex reached again is not a cycle") {
    const AdjacencyList g = make_graph(4, {{0, 1}, {1, 2}, {0, 3}, {3, 2}});
    CHECK(std::holds_alternative<TopologicalOrder>(topological_sort(g)));
}

TEST(3, "random graphs agree with a transitive closure") {
    Rng rng;
    int acyclic = 0;
    int cyclic = 0;
    for (int round = 0; round < 2000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 14));
        const int m = static_cast<int>(rng.uniform(0, 2 * n));
        const EdgeList edges = rng.coin() ? random_dag_edges(rng, n, m) : random_edges(rng, n, m);
        INFO("round ", round, " n=", n, " edges=", edges);
        const AdjacencyList g = make_graph(n, edges);
        const auto result = topological_sort(g);
        if (brute_has_cycle(n, edges)) {
            ++cyclic;
            const auto* cycle = std::get_if<DirectedCycle>(&result);
            REQUIRE(cycle != nullptr);
            check_cycle(g, cycle->vertices);
        } else {
            ++acyclic;
            const auto* order = std::get_if<TopologicalOrder>(&result);
            REQUIRE(order != nullptr);
            check_order(g, order->vertices);
        }
    }
    CHECK_GT(acyclic, 200);
    CHECK_GT(cyclic, 200);
}

// Keeping the current path in a list and scanning it for membership is quadratic here.
PERF(3, "toposort is linear on a very deep graph", 1200) {
    const int n = 1'500'000;
    AdjacencyList g(idx(n));
    for (int i = 0; i + 1 < n; ++i) {
        g[idx(i)].push_back(i + 1);
    }
    for (int i = 0; i + 2 < n; ++i) {
        g[idx(i)].push_back(i + 2);
    }
    const auto sorted = topological_sort(g);
    CHECK(std::holds_alternative<TopologicalOrder>(sorted));
    g[idx(n - 1)].push_back(n - 3);
    const auto cyclic = topological_sort(g);
    const auto* cycle = std::get_if<DirectedCycle>(&cyclic);
    REQUIRE(cycle != nullptr);
    CHECK_EQ(cycle->vertices.size(), 3);
}

TEST(4, "a cycle in the edges is reported through CycleError") {
    const EdgeList edges = {{0, 1}, {1, 2}, {2, 0}, {2, 3}};
    const AdjacencyList g = make_graph(4, edges);
    try {
        BuildGraph build(4, edges);
        CHECK(false);
    } catch (const CycleError& error) {
        CHECK_EQ(error.cycle().size(), 3);
        check_cycle(g, error.cycle());
    }
}

TEST(4, "unknown targets throw out_of_range") {
    const EdgeList bad = {{0, 3}};
    CHECK_THROWS_AS(BuildGraph(3, bad), std::out_of_range);
    const EdgeList good = {{0, 1}};
    BuildGraph build(2, good);
    const std::vector<int> changed = {2};
    CHECK_THROWS_AS(build.rebuild_order(changed), std::out_of_range);
}

TEST(4, "nothing changed means nothing to rebuild") {
    const EdgeList edges = {{0, 1}};
    BuildGraph build(2, edges);
    CHECK(build.rebuild_order({}).empty());
}

TEST(4, "only the changed target and its downstream are rebuilt, upstream first") {
    const EdgeList edges = {{0, 1}, {1, 2}, {3, 2}, {2, 4}};
    BuildGraph build(5, edges);
    const std::vector<int> changed = {1};
    CHECK_EQ(build.rebuild_order(changed), std::vector<int>{1, 2, 4});
}

TEST(4, "duplicates and overlap in changed are harmless") {
    const EdgeList edges = {{0, 1}, {1, 2}};
    BuildGraph build(4, edges);
    const std::vector<int> changed = {2, 1, 2, 1, 3};
    std::vector<int> order = build.rebuild_order(changed);
    REQUIRE_EQ(order.size(), 3);
    const auto position = [&](int t) {
        return std::find(order.begin(), order.end(), t) - order.begin();
    };
    CHECK_LT(position(1), position(2));
    CHECK_LT(position(3), 3);
}

TEST(4, "a later query is not affected by an earlier one") {
    const EdgeList edges = {{0, 1}, {1, 2}};
    BuildGraph build(3, edges);
    const std::vector<int> first = {1};
    const std::vector<int> second = {0};
    CHECK_EQ(build.rebuild_order(first), std::vector<int>{1, 2});
    CHECK_EQ(build.rebuild_order(second), std::vector<int>{0, 1, 2});
    CHECK_EQ(build.rebuild_order(first), std::vector<int>{1, 2});
}

TEST(4, "random DAGs match a transitive closure") {
    Rng rng;
    for (int round = 0; round < 800; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 18));
        const int m = static_cast<int>(rng.uniform(0, 3 * n));
        const EdgeList edges = random_dag_edges(rng, n, m);
        INFO("round ", round, " n=", n, " edges=", edges);
        const auto reach = reachability(n, edges);
        BuildGraph build(n, edges);
        for (int query = 0; query < 12; ++query) {
            std::vector<int> changed;
            const int count = static_cast<int>(rng.uniform(0, 4));
            for (int i = 0; i < count; ++i) {
                changed.push_back(static_cast<int>(rng.uniform(0, n - 1)));
            }
            std::set<int> expected(changed.begin(), changed.end());
            for (const int c : changed) {
                for (int v = 0; v < n; ++v) {
                    if (reach[idx(c)][idx(v)] != 0) {
                        expected.insert(v);
                    }
                }
            }
            const std::vector<int> order = build.rebuild_order(changed);
            std::set<int> got(order.begin(), order.end());
            REQUIRE_EQ(got.size(), order.size());
            REQUIRE(got == expected);
            std::vector<int> position(idx(n), -1);
            for (std::size_t i = 0; i < order.size(); ++i) {
                position[idx(order[i])] = static_cast<int>(i);
            }
            for (const auto& [a, b] : edges) {
                if (position[idx(a)] != -1) {
                    REQUIRE(position[idx(b)] != -1);
                    REQUIRE(position[idx(a)] < position[idx(b)]);
                }
            }
        }
    }
}

TEST(4, "a very long downstream chain does not overflow the stack") {
    const int n = 500'000;
    EdgeList edges;
    edges.reserve(idx(n));
    for (int i = 0; i + 1 < n; ++i) {
        edges.emplace_back(i, i + 1);
    }
    BuildGraph build(n, edges);
    const std::vector<int> changed = {0};
    const std::vector<int> order = build.rebuild_order(changed);
    REQUIRE_EQ(order.size(), n);
    CHECK_EQ(order.front(), 0);
    CHECK_EQ(order.back(), n - 1);
}

// Clearing a visited array of two million entries on every query costs minutes in total.
PERF(4, "a query only pays for the affected subgraph", 3000) {
    Rng rng;
    const int group = 8;
    const int groups = 250'000;
    const int n = group * groups;
    EdgeList edges;
    edges.reserve(idx(2 * n));
    for (int g = 0; g < groups; ++g) {
        const int base = g * group;
        for (int i = 0; i + 1 < group; ++i) {
            edges.emplace_back(base + i, base + i + 1);
            if (i + 2 < group) {
                edges.emplace_back(base + i, base + i + 2);
            }
        }
    }
    BuildGraph build(n, edges);
    long long touched = 0;
    for (int query = 0; query < 800'000; ++query) {
        const std::vector<int> changed = {static_cast<int>(rng.uniform(0, n - 1))};
        touched += static_cast<long long>(build.rebuild_order(changed).size());
    }
    CHECK_GT(touched, 0);
}
