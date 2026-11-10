#include <queue>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::int64_t total_of(const Network& network, const std::vector<int>& chosen) {
    std::int64_t total = 0;
    for (int v : chosen) {
        total += network.value[static_cast<std::size_t>(v)];
    }
    return total;
}

void check_independent(const Network& network, const std::vector<int>& chosen) {
    CHECK(std::ranges::is_sorted(chosen));
    std::vector<char> picked(network.size(), 0);
    for (int v : chosen) {
        REQUIRE(v >= 0 && v < static_cast<int>(network.size()));
        CHECK(!picked[static_cast<std::size_t>(v)]);
        picked[static_cast<std::size_t>(v)] = 1;
    }
    for (const auto& [a, b] : network.edges) {
        CHECK(!(picked[static_cast<std::size_t>(a)] && picked[static_cast<std::size_t>(b)]));
    }
}

// Exhaustive search over all subsets; only for small inputs.
std::int64_t brute_best(const Network& network) {
    const std::size_t n = network.size();
    std::int64_t best = 0;
    for (std::uint64_t mask = 0; mask < (std::uint64_t{1} << n); ++mask) {
        bool independent = true;
        for (const auto& [a, b] : network.edges) {
            independent = independent && !(((mask >> a) & 1) && ((mask >> b) & 1));
        }
        if (independent) {
            std::int64_t total = 0;
            for (std::size_t v = 0; v < n; ++v) {
                if ((mask >> v) & 1) {
                    total += network.value[v];
                }
            }
            best = std::max(best, total);
        }
    }
    return best;
}

// A tree whose parent links satisfy parent[i] < i, so one reverse pass is enough.
Network random_tree(Rng& rng, std::size_t n, std::int64_t max_value) {
    Network tree;
    tree.value.resize(n);
    for (auto& value : tree.value) {
        value = rng.uniform(0, max_value);
    }
    for (std::size_t i = 1; i < n; ++i) {
        tree.edges.emplace_back(static_cast<int>(rng.uniform(0, static_cast<std::int64_t>(i) - 1)),
                                static_cast<int>(i));
    }
    return tree;
}

std::int64_t tree_optimum(const Network& tree) {
    const std::size_t n = tree.size();
    std::vector<std::int64_t> with(n, 0);
    std::vector<std::int64_t> without(n, 0);
    std::vector<int> parent(n, -1);
    for (const auto& [a, b] : tree.edges) {
        parent[static_cast<std::size_t>(b)] = a;
    }
    for (std::size_t v = n; v-- > 0;) {
        with[v] += tree.value[v];
        if (parent[v] != -1) {
            const auto p = static_cast<std::size_t>(parent[v]);
            with[p] += without[v];
            without[p] += std::max(with[v], without[v]);
        }
    }
    return n == 0 ? 0 : std::max(with[0], without[0]);
}

Network path(std::size_t n, std::int64_t value) {
    Network network;
    network.value.assign(n, value);
    for (std::size_t i = 1; i < n; ++i) {
        network.edges.emplace_back(static_cast<int>(i - 1), static_cast<int>(i));
    }
    return network;
}

bool is_tree(const Network& network) {
    const std::size_t n = network.size();
    if (n == 0 || network.edges.size() != n - 1) {
        return false;
    }
    std::vector<std::vector<int>> adjacent(n);
    for (const auto& [a, b] : network.edges) {
        adjacent[static_cast<std::size_t>(a)].push_back(b);
        adjacent[static_cast<std::size_t>(b)].push_back(a);
    }
    std::vector<char> seen(n, 0);
    std::vector<int> stack{0};
    seen[0] = 1;
    std::size_t reached = 0;
    while (!stack.empty()) {
        const int v = stack.back();
        stack.pop_back();
        ++reached;
        for (int u : adjacent[static_cast<std::size_t>(v)]) {
            if (!seen[static_cast<std::size_t>(u)]) {
                seen[static_cast<std::size_t>(u)] = 1;
                stack.push_back(u);
            }
        }
    }
    return reached == n;
}

// The cycle and random graphs defeat plain enumeration but not a branching search.
Network hard_graph(Rng& rng, std::size_t n, double edge_probability) {
    Network network;
    network.value.resize(n);
    for (auto& value : network.value) {
        value = rng.uniform(1, 1000);
    }
    for (std::size_t a = 0; a < n; ++a) {
        for (std::size_t b = a + 1; b < n; ++b) {
            if (rng.coin(edge_probability)) {
                network.edges.emplace_back(static_cast<int>(a), static_cast<int>(b));
            }
        }
    }
    return network;
}

}  // namespace

TEST(1, "greedy takes the most valuable vertex and blocks its neighbors") {
    const Network network{{1, 5, 1, 4, 3}, {{0, 1}, {1, 2}, {2, 3}, {3, 4}}};
    CHECK_EQ(greedy_by_value(network), (std::vector<int>{1, 3}));
}

TEST(1, "ties are broken by the lower vertex number") {
    const Network network{{7, 7, 7}, {{0, 1}, {1, 2}}};
    CHECK_EQ(greedy_by_value(network), (std::vector<int>{0, 2}));
}

TEST(1, "an edgeless network takes everything") {
    const Network network{{1, 2, 3}, {}};
    CHECK_EQ(greedy_by_value(network), (std::vector<int>{0, 1, 2}));
}

TEST(1, "no vertices means nothing is chosen") {
    CHECK(greedy_by_value(Network{}).empty());
}

TEST(1, "the counterexample is a small tree that beats the greedy") {
    const Network network = greedy_counterexample();
    CHECK(is_tree(network));
    REQUIRE(network.size() <= 12);
    for (std::int64_t value : network.value) {
        CHECK_GE(value, 0);
    }
    const auto chosen = greedy_by_value(network);
    check_independent(network, chosen);
    CHECK_LT(total_of(network, chosen), brute_best(network));
}

PERF(1, "greedy on a large tree", 3000) {
    Rng rng;
    const Network tree = random_tree(rng, 1'000'000, 1'000'000);
    check_independent(tree, greedy_by_value(tree));
}

TEST(2, "the empty tree and a single vertex") {
    CHECK(max_value_placement(Network{}).empty());
    const Network one{{5}, {}};
    CHECK_EQ(max_value_placement(one), std::vector<int>{0});
}

TEST(2, "a path where the two ends beat the middle") {
    const Network network{{5, 6, 5}, {{0, 1}, {1, 2}}};
    CHECK_EQ(max_value_placement(network), (std::vector<int>{0, 2}));
}

TEST(2, "a star where the center beats all the leaves") {
    const Network network{{100, 3, 3, 3}, {{0, 1}, {0, 2}, {0, 3}}};
    CHECK_EQ(max_value_placement(network), std::vector<int>{0});
}

TEST(2, "zero-valued vertices do not break the bookkeeping") {
    const Network network{{0, 0, 0}, {{0, 1}, {1, 2}}};
    const auto chosen = max_value_placement(network);
    check_independent(network, chosen);
    CHECK_EQ(total_of(network, chosen), 0);
}

TEST(2, "vertices need not be numbered from the root") {
    const Network network{{1, 1, 9, 9, 1}, {{4, 0}, {0, 1}, {1, 2}, {2, 3}}};
    const auto chosen = max_value_placement(network);
    check_independent(network, chosen);
    CHECK_EQ(total_of(network, chosen), 11);
}

TEST(2, "random trees match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        const Network tree = random_tree(rng, static_cast<std::size_t>(rng.uniform(1, 14)), 20);
        const auto chosen = max_value_placement(tree);
        check_independent(tree, chosen);
        REQUIRE_EQ(total_of(tree, chosen), brute_best(tree));
    }
}

TEST(2, "large random trees reach the optimum") {
    Rng rng;
    for (int trial = 0; trial < 20; ++trial) {
        INFO("trial ", trial);
        const Network tree = random_tree(rng, 20'000, 1'000'000);
        const auto chosen = max_value_placement(tree);
        check_independent(tree, chosen);
        CHECK_EQ(total_of(tree, chosen), tree_optimum(tree));
    }
}

TEST(2, "a million-vertex path does not overflow the stack") {
    const Network network = path(1'000'000, 1);
    const auto chosen = max_value_placement(network);
    CHECK_EQ(chosen.size(), 500'000);
}

PERF(2, "tree DP on two million vertices", 3000) {
    Rng rng;
    const Network tree = random_tree(rng, 2'000'000, 1'000'000'000);
    const auto chosen = max_value_placement(tree);
    CHECK_EQ(total_of(tree, chosen), tree_optimum(tree));
}

TEST(3, "single vertices and edges") {
    CHECK(max_count_placement(Network{}).empty());
    CHECK_EQ(max_count_placement(Network{{7}, {}}), std::vector<int>{0});
    CHECK_EQ(max_count_placement(Network{{1, 1}, {{0, 1}}}).size(), 1);
}

TEST(3, "a star keeps every leaf") {
    Network star;
    star.value.assign(11, 1);
    for (int leaf = 1; leaf <= 10; ++leaf) {
        star.edges.emplace_back(0, leaf);
    }
    const auto chosen = max_count_placement(star);
    check_independent(star, chosen);
    CHECK_EQ(chosen.size(), 10);
}

TEST(3, "the count is independent of the values") {
    Network tree{{0, 99, 0, 99}, {{0, 1}, {1, 2}, {2, 3}}};
    const auto chosen = max_count_placement(tree);
    check_independent(tree, chosen);
    CHECK_EQ(chosen.size(), 2);
}

TEST(3, "random trees reach the maximum cardinality") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        Network tree = random_tree(rng, static_cast<std::size_t>(rng.uniform(1, 14)), 0);
        tree.value.assign(tree.size(), 1);
        const auto chosen = max_count_placement(tree);
        check_independent(tree, chosen);
        REQUIRE_EQ(static_cast<std::int64_t>(chosen.size()), brute_best(tree));
    }
}

TEST(3, "large random trees reach the maximum cardinality") {
    Rng rng;
    for (int trial = 0; trial < 20; ++trial) {
        INFO("trial ", trial);
        Network tree = random_tree(rng, 20'000, 0);
        tree.value.assign(tree.size(), 1);
        const auto chosen = max_count_placement(tree);
        check_independent(tree, chosen);
        CHECK_EQ(static_cast<std::int64_t>(chosen.size()), tree_optimum(tree));
    }
}

TEST(3, "a million-vertex path does not overflow the stack") {
    const Network network = path(1'000'000, 1);
    CHECK_EQ(max_count_placement(network).size(), 500'000);
}

PERF(3, "cardinality greedy on two million vertices", 2500) {
    Rng rng;
    Network tree = random_tree(rng, 2'000'000, 0);
    tree.value.assign(tree.size(), 1);
    CHECK_EQ(static_cast<std::int64_t>(max_count_placement(tree).size()), tree_optimum(tree));
}

TEST(4, "the empty graph") {
    CHECK(max_value_placement_general(Network{}).empty());
}

TEST(4, "a triangle takes its most valuable vertex") {
    const Network network{{4, 9, 2}, {{0, 1}, {1, 2}, {2, 0}}};
    CHECK_EQ(max_value_placement_general(network), std::vector<int>{1});
}

TEST(4, "a five-cycle is not bipartite") {
    const Network network{{3, 3, 3, 3, 3}, {{0, 1}, {1, 2}, {2, 3}, {3, 4}, {4, 0}}};
    const auto chosen = max_value_placement_general(network);
    check_independent(network, chosen);
    CHECK_EQ(total_of(network, chosen), 6);
}

TEST(4, "isolated vertices are all taken") {
    const Network network{{1, 2, 3, 4}, {}};
    const auto chosen = max_value_placement_general(network);
    CHECK_EQ(total_of(network, chosen), 10);
}

TEST(4, "random small graphs match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        INFO("trial ", trial);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 13));
        const Network graph = hard_graph(rng, n, 0.15 * static_cast<double>(rng.uniform(1, 5)));
        const auto chosen = max_value_placement_general(graph);
        check_independent(graph, chosen);
        REQUIRE_EQ(total_of(graph, chosen), brute_best(graph));
    }
}

TEST(4, "a tree is solved the same as the tree algorithm") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        INFO("trial ", trial);
        const Network tree = random_tree(rng, static_cast<std::size_t>(rng.uniform(1, 24)), 50);
        const auto chosen = max_value_placement_general(tree);
        check_independent(tree, chosen);
        REQUIRE_EQ(total_of(tree, chosen), tree_optimum(tree));
    }
}

PERF(4, "sparse graphs on forty vertices", 2000) {
    Rng rng;
    for (int trial = 0; trial < 6; ++trial) {
        const Network graph = hard_graph(rng, 40, 0.08);
        const auto chosen = max_value_placement_general(graph);
        check_independent(graph, chosen);
    }
}

PERF(4, "a forty-vertex cycle", 2500) {
    Network cycle = path(40, 1);
    cycle.edges.emplace_back(39, 0);
    const auto chosen = max_value_placement_general(cycle);
    check_independent(cycle, chosen);
    CHECK_EQ(chosen.size(), 20);
}
