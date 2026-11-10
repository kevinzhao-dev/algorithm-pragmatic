#include <algorithm>
#include <map>
#include <numeric>
#include <optional>
#include <set>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Union-find used only to judge answers.
class Components {
public:
    explicit Components(int n) : parent_(static_cast<std::size_t>(n)) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }
    int find(int v) {
        while (parent_[index(v)] != v) {
            parent_[index(v)] = parent_[index(parent_[index(v)])];
            v = parent_[index(v)];
        }
        return v;
    }
    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) {
            return false;
        }
        parent_[index(a)] = b;
        return true;
    }

private:
    static std::size_t index(int v) { return static_cast<std::size_t>(v); }
    std::vector<int> parent_;
};

struct Facts {
    bool connected;
    bool acyclic;
};

Facts judge(int n, const std::vector<Edge>& edges) {
    Components components(n);
    int merges = 0;
    bool acyclic = true;
    for (const auto [u, v] : edges) {
        if (components.unite(u, v)) {
            ++merges;
        } else {
            acyclic = false;
        }
    }
    return {merges == n - 1, acyclic};
}

std::vector<Edge> path_graph(int n) {
    std::vector<Edge> edges;
    for (int v = 0; v + 1 < n; ++v) {
        edges.push_back({v, v + 1});
    }
    return edges;
}

// Random labeled tree on n vertices: each vertex connects to a random earlier vertex.
std::vector<Edge> random_tree(Rng& rng, int n) {
    auto labels = rng.permutation(static_cast<std::size_t>(n));
    std::vector<Edge> edges;
    for (int i = 1; i < n; ++i) {
        const auto parent = static_cast<std::size_t>(rng.uniform(0, i - 1));
        edges.push_back({labels[static_cast<std::size_t>(i)], labels[parent]});
    }
    rng.shuffle(edges);
    return edges;
}

// Plays the claimed peeling back on the graph.
void check_certificate(int n, const std::vector<Edge>& edges, const std::vector<Peel>& peels) {
    REQUIRE_EQ(peels.size(), static_cast<std::size_t>(n - 1));
    std::vector<int> degree(static_cast<std::size_t>(n), 0);
    std::map<std::pair<int, int>, int> remaining;
    const auto key = [](int a, int b) { return std::pair(std::min(a, b), std::max(a, b)); };
    for (const auto [u, v] : edges) {
        ++degree[static_cast<std::size_t>(u)];
        ++degree[static_cast<std::size_t>(v)];
        ++remaining[key(u, v)];
    }
    std::vector<char> gone(static_cast<std::size_t>(n), 0);
    for (std::size_t step = 0; step < peels.size(); ++step) {
        INFO("peel #", step);
        const auto [leaf, neighbor] = peels[step];
        REQUIRE(leaf >= 0 && leaf < n);
        REQUIRE(neighbor >= 0 && neighbor < n);
        REQUIRE(!gone[static_cast<std::size_t>(leaf)]);
        REQUIRE_EQ(degree[static_cast<std::size_t>(leaf)], 1);
        auto& count = remaining[key(leaf, neighbor)];
        REQUIRE(count > 0);
        --count;
        --degree[static_cast<std::size_t>(leaf)];
        --degree[static_cast<std::size_t>(neighbor)];
        gone[static_cast<std::size_t>(leaf)] = 1;
    }
    int alive = 0;
    for (std::size_t v = 0; v < gone.size(); ++v) {
        alive += gone[v] ? 0 : 1;
    }
    CHECK_EQ(alive, 1);
}

// In a unicyclic graph any simple cycle is the only cycle, so it suffices to check shape.
void check_cycle(const std::vector<Edge>& edges, const std::vector<int>& cycle) {
    REQUIRE(!cycle.empty());
    std::map<std::pair<int, int>, int> available;
    const auto key = [](int a, int b) { return std::pair(std::min(a, b), std::max(a, b)); };
    for (const auto [u, v] : edges) {
        ++available[key(u, v)];
    }
    std::set<int> seen;
    for (std::size_t i = 0; i < cycle.size(); ++i) {
        INFO("position ", i);
        REQUIRE(seen.insert(cycle[i]).second);
        auto& count = available[key(cycle[i], cycle[(i + 1) % cycle.size()])];
        REQUIRE(count > 0);
        --count;
    }
}

}  // namespace

TEST(1, "a single vertex is connected, acyclic and a tree") {
    CHECK(is_connected(1, {}));
    CHECK(is_acyclic(1, {}));
    CHECK(is_tree(1, {}));
}

TEST(1, "isolated vertices are not connected") {
    CHECK(!is_connected(2, {}));
    CHECK(is_acyclic(2, {}));
    CHECK(!is_tree(2, {}));
}

TEST(1, "a path is a tree") {
    const auto edges = path_graph(5);
    CHECK(is_connected(5, edges));
    CHECK(is_acyclic(5, edges));
    CHECK(is_tree(5, edges));
}

TEST(1, "a star is a tree") {
    const std::vector<Edge> edges = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    CHECK(is_tree(5, edges));
}

TEST(1, "a triangle is connected but has a cycle") {
    const std::vector<Edge> edges = {{0, 1}, {1, 2}, {2, 0}};
    CHECK(is_connected(3, edges));
    CHECK(!is_acyclic(3, edges));
    CHECK(!is_tree(3, edges));
}

TEST(1, "a forest is acyclic but not connected") {
    const std::vector<Edge> edges = {{0, 1}, {2, 3}};
    CHECK(!is_connected(4, edges));
    CHECK(is_acyclic(4, edges));
    CHECK(!is_tree(4, edges));
}

TEST(1, "a self-loop is a cycle") {
    const std::vector<Edge> edges = {{0, 0}};
    CHECK(is_connected(1, edges));
    CHECK(!is_acyclic(1, edges));
    CHECK(!is_tree(1, edges));
}

TEST(1, "parallel edges form a cycle") {
    const std::vector<Edge> edges = {{0, 1}, {1, 0}};
    CHECK(is_connected(2, edges));
    CHECK(!is_acyclic(2, edges));
    CHECK(!is_tree(2, edges));
}

TEST(1, "cycle and extra component: edge count alone does not decide") {
    // 3 vertices, 2 edges... but 4 vertices with |E| = |V| - 1 and a triangle plus an isolated one.
    const std::vector<Edge> edges = {{0, 1}, {1, 2}, {0, 2}};
    CHECK(!is_connected(4, edges));
    CHECK(!is_acyclic(4, edges));
    CHECK(!is_tree(4, edges));
}

TEST(1, "every simple graph up to six vertices matches union-find") {
    for (int n = 1; n <= 6; ++n) {
        std::vector<Edge> all;
        for (int u = 0; u < n; ++u) {
            for (int v = u + 1; v < n; ++v) {
                all.push_back({u, v});
            }
        }
        for (unsigned mask = 0; mask < (1U << all.size()); ++mask) {
            std::vector<Edge> edges;
            for (std::size_t i = 0; i < all.size(); ++i) {
                if ((mask >> i) & 1U) {
                    edges.push_back(all[i]);
                }
            }
            INFO("n=", n, " mask=", mask);
            const auto facts = judge(n, edges);
            REQUIRE_EQ(is_connected(n, edges), facts.connected);
            REQUIRE_EQ(is_acyclic(n, edges), facts.acyclic);
            REQUIRE_EQ(is_tree(n, edges), facts.connected && facts.acyclic);
            if (facts.connected && static_cast<int>(edges.size()) == n - 1) {
                REQUIRE(is_acyclic(n, edges));  // the theorem from the problem set
            }
        }
    }
}

TEST(1, "random multigraphs match union-find") {
    Rng rng;
    for (int round = 0; round < 3'000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 12));
        const int m = static_cast<int>(rng.uniform(0, 16));
        std::vector<Edge> edges;
        for (int e = 0; e < m; ++e) {
            edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                             static_cast<int>(rng.uniform(0, n - 1))});
        }
        INFO("round ", round);
        const auto facts = judge(n, edges);
        REQUIRE_EQ(is_connected(n, edges), facts.connected);
        REQUIRE_EQ(is_acyclic(n, edges), facts.acyclic);
        REQUIRE_EQ(is_tree(n, edges), facts.connected && facts.acyclic);
    }
}

TEST(1, "a path of a million vertices does not overflow the stack") {
    const int n = 1'000'000;
    const auto edges = path_graph(n);
    CHECK(is_connected(n, edges));
    CHECK(is_acyclic(n, edges));
    CHECK(is_tree(n, edges));
}

PERF(1, "linear time on large random trees", 3000) {
    Rng rng;
    const int n = 500'000;
    const auto edges = random_tree(rng, n);
    CHECK(is_connected(n, edges));
    CHECK(is_acyclic(n, edges));
    CHECK(is_tree(n, edges));
}

TEST(2, "a single vertex needs no peeling") {
    const auto peels = peel_order(1, {});
    REQUIRE(peels.has_value());
    CHECK(peels->empty());
}

TEST(2, "peeling a path follows valid leaves") {
    const auto edges = path_graph(6);
    const auto peels = peel_order(6, edges);
    REQUIRE(peels.has_value());
    check_certificate(6, edges, *peels);
}

TEST(2, "peeling a star always removes outer vertices") {
    const std::vector<Edge> edges = {{0, 1}, {0, 2}, {0, 3}, {0, 4}};
    const auto peels = peel_order(5, edges);
    REQUIRE(peels.has_value());
    check_certificate(5, edges, *peels);
}

TEST(2, "non-trees have no peeling") {
    const std::vector<Edge> triangle = {{0, 1}, {1, 2}, {2, 0}};
    CHECK(!peel_order(3, triangle).has_value());

    const std::vector<Edge> forest = {{0, 1}, {2, 3}};
    CHECK(!peel_order(4, forest).has_value());

    const std::vector<Edge> self_loop = {{0, 0}};
    CHECK(!peel_order(1, self_loop).has_value());

    const std::vector<Edge> parallel = {{0, 1}, {0, 1}, {1, 2}};
    CHECK(!peel_order(3, parallel).has_value());

    const std::vector<Edge> disconnected_with_cycle = {{0, 1}, {1, 2}, {2, 0}, {3, 4}};
    CHECK(!peel_order(5, disconnected_with_cycle).has_value());
}

TEST(2, "random trees yield valid certificates") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 60));
        const auto edges = random_tree(rng, n);
        INFO("round ", round, " n=", n);
        const auto peels = peel_order(n, edges);
        REQUIRE(peels.has_value());
        check_certificate(n, edges, *peels);
    }
}

TEST(2, "random graphs: a peeling exists exactly when the graph is a tree") {
    Rng rng;
    for (int round = 0; round < 3'000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        std::vector<Edge> edges;
        const int m = static_cast<int>(rng.uniform(0, n + 2));
        for (int e = 0; e < m; ++e) {
            edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                             static_cast<int>(rng.uniform(0, n - 1))});
        }
        INFO("round ", round);
        const auto facts = judge(n, edges);
        const auto peels = peel_order(n, edges);
        REQUIRE_EQ(peels.has_value(), facts.connected && facts.acyclic);
        if (peels) {
            check_certificate(n, edges, *peels);
        }
    }
}

PERF(2, "peeling half a million vertices is linear", 1500) {
    Rng rng;
    const int n = 500'000;
    const auto edges = random_tree(rng, n);
    const auto peels = peel_order(n, edges);
    REQUIRE(peels.has_value());
    REQUIRE_EQ(peels->size(), static_cast<std::size_t>(n - 1));
    std::vector<int> degree(static_cast<std::size_t>(n), 0);
    for (const auto [u, v] : edges) {
        ++degree[static_cast<std::size_t>(u)];
        ++degree[static_cast<std::size_t>(v)];
    }
    for (const auto [leaf, neighbor] : *peels) {
        REQUIRE_EQ(degree[static_cast<std::size_t>(leaf)], 1);
        --degree[static_cast<std::size_t>(leaf)];
        --degree[static_cast<std::size_t>(neighbor)];
    }
}

TEST(3, "a triangle with a pendant vertex") {
    const std::vector<Edge> edges = {{0, 1}, {1, 2}, {2, 0}, {2, 3}};
    const auto cycle = find_cycle(4, edges);
    CHECK_EQ(cycle.size(), 3);
    check_cycle(edges, cycle);
}

TEST(3, "a self-loop on a tree") {
    const std::vector<Edge> edges = {{0, 1}, {1, 2}, {1, 1}};
    const auto cycle = find_cycle(3, edges);
    CHECK_EQ(cycle, std::vector<int>{1});
    check_cycle(edges, cycle);
}

TEST(3, "a pair of parallel edges") {
    const std::vector<Edge> edges = {{0, 1}, {1, 2}, {1, 2}};
    const auto cycle = find_cycle(3, edges);
    CHECK_EQ(cycle.size(), 2);
    check_cycle(edges, cycle);
}

TEST(3, "the whole graph is one big cycle") {
    const int n = 7;
    auto edges = path_graph(n);
    edges.push_back({n - 1, 0});
    const auto cycle = find_cycle(n, edges);
    CHECK_EQ(cycle.size(), static_cast<std::size_t>(n));
    check_cycle(edges, cycle);
}

TEST(3, "a single vertex with a self-loop") {
    const std::vector<Edge> edges = {{0, 0}};
    CHECK_EQ(find_cycle(1, edges), std::vector<int>{0});
}

TEST(3, "random unicyclic graphs yield the unique cycle") {
    Rng rng;
    for (int round = 0; round < 2'000; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 40));
        auto edges = random_tree(rng, n);
        edges.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                         static_cast<int>(rng.uniform(0, n - 1))});
        rng.shuffle(edges);
        INFO("round ", round, " n=", n);
        check_cycle(edges, find_cycle(n, edges));
    }
}

TEST(3, "a million-vertex cycle does not overflow the stack") {
    const int n = 1'000'000;
    auto edges = path_graph(n);
    edges.push_back({n - 1, 0});
    CHECK_EQ(find_cycle(n, edges).size(), static_cast<std::size_t>(n));
}

PERF(3, "cycle finding is linear", 1500) {
    Rng rng;
    const int n = 500'000;
    auto edges = random_tree(rng, n);
    edges.push_back({0, n / 2});
    const auto cycle = find_cycle(n, edges);
    CHECK_GE(cycle.size(), 1);
}
