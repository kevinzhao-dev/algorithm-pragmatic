#include <algorithm>
#include <cstdint>
#include <optional>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// A random DAG on shuffled labels so vertex numbers are not a topological order.
Graph random_dag(Rng& rng, int vertices, double density) {
    const std::vector<int> labels = rng.permutation(static_cast<std::size_t>(vertices));
    Graph graph(static_cast<std::size_t>(vertices));
    for (int i = 0; i < vertices; ++i) {
        for (int j = i + 1; j < vertices; ++j) {
            if (rng.coin(density)) {
                graph[static_cast<std::size_t>(labels[static_cast<std::size_t>(i)])].push_back(
                    {labels[static_cast<std::size_t>(j)], static_cast<int>(rng.uniform(-6, 6))});
            }
        }
    }
    return graph;
}

// Walks every path from `from` to `target`, adding one to counts[residue] at each arrival.
void enumerate_paths(const Graph& graph, int from, int target, int modulus, int residue,
                     std::vector<std::uint64_t>& counts) {
    if (from == target) {
        ++counts[static_cast<std::size_t>(residue)];
    }
    for (const Edge& edge : graph[static_cast<std::size_t>(from)]) {
        enumerate_paths(graph, edge.to, target, modulus,
                        ((residue + edge.weight % modulus) % modulus + modulus) % modulus, counts);
    }
}

std::vector<std::uint64_t> brute_counts(const Graph& graph, int source, int target, int modulus) {
    std::vector<std::uint64_t> counts(static_cast<std::size_t>(modulus), 0);
    enumerate_paths(graph, source, target, modulus, 0, counts);
    return counts;
}

// Each diamond doubles the number of paths while keeping the graph tiny.
Graph diamond_chain(int diamonds, int odd_side_weight) {
    Graph graph(static_cast<std::size_t>(3 * diamonds + 1));
    for (int d = 0; d < diamonds; ++d) {
        const auto left = static_cast<std::size_t>(3 * d);
        const int top = 3 * d + 1;
        const int bottom = 3 * d + 2;
        const int right = 3 * d + 3;
        graph[left].push_back({top, 0});
        graph[left].push_back({bottom, odd_side_weight});
        graph[static_cast<std::size_t>(top)].push_back({right, 0});
        graph[static_cast<std::size_t>(bottom)].push_back({right, 0});
    }
    return graph;
}

Graph long_chain(int vertices, int weight) {
    Graph graph(static_cast<std::size_t>(vertices));
    for (int v = 0; v + 1 < vertices; ++v) {
        graph[static_cast<std::size_t>(v)].push_back({v + 1, weight});
    }
    return graph;
}

Graph large_layered_dag(Rng& rng, int vertices, int fanout) {
    Graph graph(static_cast<std::size_t>(vertices));
    for (int v = 0; v < vertices; ++v) {
        for (int f = 0; f < fanout; ++f) {
            const std::int64_t step = rng.uniform(1, 64);
            if (v + step < vertices) {
                graph[static_cast<std::size_t>(v)].push_back(
                    {v + static_cast<int>(step), static_cast<int>(rng.uniform(-9, 9))});
            }
        }
    }
    return graph;
}

}  // namespace

TEST(1, "a vertex reaches itself by the empty path") {
    const Graph graph(1);
    CHECK_EQ(count_paths(graph, 0, 0), 1);
}

TEST(1, "an unreachable target has no paths") {
    Graph graph(3);
    graph[1].push_back({2, 1});
    CHECK_EQ(count_paths(graph, 0, 2), 0);
}

TEST(1, "parallel edges are distinct paths") {
    Graph graph(2);
    graph[0].push_back({1, 1});
    graph[0].push_back({1, 2});
    CHECK_EQ(count_paths(graph, 0, 1), 2);
}

TEST(1, "edges pointing the wrong way are not walked") {
    Graph graph(2);
    graph[1].push_back({0, 1});
    CHECK_EQ(count_paths(graph, 0, 1), 0);
}

TEST(1, "exponentially many paths are counted, not listed") {
    const Graph graph = diamond_chain(62, 1);
    CHECK_EQ(count_paths(graph, 0, 3 * 62), std::uint64_t{1} << 62);
}

TEST(1, "a cycle anywhere in the graph is rejected") {
    Graph graph(4);
    graph[0].push_back({1, 1});
    graph[2].push_back({3, 1});
    graph[3].push_back({2, 1});  // not reachable from vertex 0
    CHECK_THROWS_AS(count_paths(graph, 0, 1), std::invalid_argument);
}

TEST(1, "a self-loop is a cycle") {
    Graph graph(2);
    graph[1].push_back({1, 1});
    CHECK_THROWS_AS(count_paths(graph, 0, 1), std::invalid_argument);
}

TEST(1, "out-of-range vertices are rejected") {
    const Graph graph(2);
    CHECK_THROWS_AS(count_paths(graph, -1, 0), std::out_of_range);
    CHECK_THROWS_AS(count_paths(graph, 0, 2), std::out_of_range);
    CHECK_THROWS_AS(count_paths(Graph{}, 0, 0), std::out_of_range);
}

TEST(1, "a very long chain does not overflow the stack") {
    const Graph graph = long_chain(400'000, 1);
    CHECK_EQ(count_paths(graph, 0, 399'999), 1);
}

TEST(1, "random DAGs match path enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const Graph graph = random_dag(rng, n, 0.45);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        INFO("trial ", trial, " s ", s, " t ", t);
        REQUIRE_EQ(count_paths(graph, s, t), brute_counts(graph, s, t, 1)[0]);
    }
}

PERF(1, "three hundred thousand vertices and over a million edges", 800) {
    Rng rng;
    const Graph graph = large_layered_dag(rng, 300'000, 4);
    CHECK_GE(count_paths(graph, 0, 299'999), 0);
}

TEST(2, "the empty path has even weight") {
    const Graph graph(1);
    const auto counts = count_paths_by_parity(graph, 0, 0);
    CHECK_EQ(counts.even, 1);
    CHECK_EQ(counts.odd, 0);
}

TEST(2, "odd edges flip the parity") {
    Graph graph(3);
    graph[0].push_back({1, 3});
    graph[1].push_back({2, 4});
    const auto counts = count_paths_by_parity(graph, 0, 2);
    CHECK_EQ(counts.even, 0);
    CHECK_EQ(counts.odd, 1);
}

TEST(2, "negative odd weights are odd") {
    Graph graph(2);
    graph[0].push_back({1, -3});
    const auto counts = count_paths_by_parity(graph, 0, 1);
    CHECK_EQ(counts.even, 0);
    CHECK_EQ(counts.odd, 1);
}

TEST(2, "two odd edges make an even path") {
    Graph graph(3);
    graph[0].push_back({1, -1});
    graph[1].push_back({2, 5});
    const auto counts = count_paths_by_parity(graph, 0, 2);
    CHECK_EQ(counts.even, 1);
    CHECK_EQ(counts.odd, 0);
}

TEST(2, "a diamond chain splits half and half") {
    // Each diamond contributes either 0 or the odd weight, independently.
    const Graph graph = diamond_chain(61, 1);
    const auto counts = count_paths_by_parity(graph, 0, 3 * 61);
    CHECK_EQ(counts.even, std::uint64_t{1} << 60);
    CHECK_EQ(counts.odd, std::uint64_t{1} << 60);
}

TEST(2, "a long chain is still handled iteratively") {
    const Graph graph = long_chain(300'001, 1);
    const auto counts = count_paths_by_parity(graph, 0, 300'000);
    CHECK_EQ(counts.even, 1);  // 300000 odd edges
    CHECK_EQ(counts.odd, 0);
}

TEST(2, "random DAGs match path enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const Graph graph = random_dag(rng, n, 0.45);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        INFO("trial ", trial, " s ", s, " t ", t);
        const auto brute = brute_counts(graph, s, t, 2);
        const auto counts = count_paths_by_parity(graph, s, t);
        REQUIRE_EQ(counts.even, brute[0]);
        REQUIRE_EQ(counts.odd, brute[1]);
    }
}

PERF(2, "parity on a large graph", 800) {
    Rng rng;
    const Graph graph = large_layered_dag(rng, 300'000, 4);
    const auto counts = count_paths_by_parity(graph, 0, 299'999);
    CHECK_EQ(counts.even + counts.odd, count_paths(graph, 0, 299'999));
}

TEST(3, "modulus one lumps everything together") {
    Graph graph(2);
    graph[0].push_back({1, 7});
    graph[0].push_back({1, -2});
    CHECK_EQ(count_paths_by_residue(graph, 0, 1, 1), std::vector<std::uint64_t>{2});
}

TEST(3, "negative weights land on normalized residues") {
    Graph graph(2);
    graph[0].push_back({1, -1});
    CHECK_EQ(count_paths_by_residue(graph, 0, 1, 3), std::vector<std::uint64_t>{0, 0, 1});
}

TEST(3, "a non-positive modulus is rejected") {
    const Graph graph(1);
    CHECK_THROWS_AS(count_paths_by_residue(graph, 0, 0, 0), std::invalid_argument);
}

TEST(3, "random DAGs and moduli match path enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const Graph graph = random_dag(rng, n, 0.45);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        const int modulus = static_cast<int>(rng.uniform(1, 7));
        INFO("trial ", trial, " modulus ", modulus);
        REQUIRE_EQ(count_paths_by_residue(graph, s, t, modulus), brute_counts(graph, s, t, modulus));
    }
}

PERF(3, "residues on a large graph", 1200) {
    Rng rng;
    const Graph graph = large_layered_dag(rng, 200'000, 4);
    CHECK_EQ(count_paths_by_residue(graph, 0, 199'999, 7).size(), 7);
}

TEST(4, "the empty path answers residue zero at the source") {
    const Graph graph(1);
    CHECK_EQ(find_path_with_residue(graph, 0, 0, 2, 0), std::optional<std::vector<int>>({0}));
    CHECK_EQ(find_path_with_residue(graph, 0, 0, 2, 1), std::optional<std::vector<int>>());
}

TEST(4, "a specific path is reported in order") {
    Graph graph(3);
    graph[0].push_back({1, 1});
    graph[1].push_back({2, 2});
    CHECK_EQ(find_path_with_residue(graph, 0, 2, 3, 0), std::optional<std::vector<int>>({0, 1, 2}));
    CHECK_EQ(find_path_with_residue(graph, 0, 2, 2, 0), std::optional<std::vector<int>>());
}

TEST(4, "random searches agree with path enumeration and return real paths") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const int n = static_cast<int>(rng.uniform(1, 9));
        const Graph graph = random_dag(rng, n, 0.45);
        const int s = static_cast<int>(rng.uniform(0, n - 1));
        const int t = static_cast<int>(rng.uniform(0, n - 1));
        const int modulus = static_cast<int>(rng.uniform(1, 5));
        const int residue = static_cast<int>(rng.uniform(0, modulus - 1));
        INFO("trial ", trial, " modulus ", modulus, " residue ", residue);
        const auto found = find_path_with_residue(graph, s, t, modulus, residue);
        const auto brute = brute_counts(graph, s, t, modulus);
        if (brute[static_cast<std::size_t>(residue)] == 0) {
            REQUIRE(!found.has_value());
            continue;
        }
        REQUIRE(found.has_value());
        REQUIRE(!found->empty());
        REQUIRE_EQ(found->front(), s);
        REQUIRE_EQ(found->back(), t);
        int weight = 0;
        for (std::size_t i = 0; i + 1 < found->size(); ++i) {
            const auto& edges = graph[static_cast<std::size_t>((*found)[i])];
            const auto edge = std::ranges::find_if(
                edges, [&](const Edge& e) { return e.to == (*found)[i + 1]; });
            REQUIRE(edge != edges.end());
            weight += edge->weight;
        }
        REQUIRE_EQ(((weight % modulus) + modulus) % modulus, residue);
    }
}
