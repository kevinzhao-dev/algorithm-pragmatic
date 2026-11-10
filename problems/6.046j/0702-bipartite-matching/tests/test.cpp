#include <algorithm>
#include <functional>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Edges = std::vector<std::pair<int, int>>;

namespace {

Edges random_edges(Rng& rng, int left, int right, int m) {
    Edges edges;
    for (int i = 0; i < m; ++i) {
        edges.emplace_back(rng.uniform(0, left - 1), rng.uniform(0, right - 1));
    }
    return edges;
}

// Exhaustive maximum matching by recursion over left vertices (left <= 9).
int brute_matching(int left, int right, const Edges& edges) {
    std::vector<std::vector<int>> adj(left);
    for (const auto& [l, r] : edges) {
        adj[l].push_back(r);
    }
    std::vector<char> used(right, 0);
    const std::function<int(int)> solve = [&](int l) -> int {
        if (l == left) {
            return 0;
        }
        int best = solve(l + 1);
        for (int r : adj[l]) {
            if (!used[r]) {
                used[r] = 1;
                best = std::max(best, 1 + solve(l + 1));
                used[r] = 0;
            }
        }
        return best;
    };
    return solve(0);
}

void check_matching(const Matching& m, int left, int right, const Edges& edges) {
    REQUIRE_EQ(m.match_left.size(), left);
    REQUIRE_EQ(m.match_right.size(), right);
    int count = 0;
    const std::set<std::pair<int, int>> all(edges.begin(), edges.end());
    for (int l = 0; l < left; ++l) {
        const int r = m.match_left[l];
        if (r != -1) {
            ++count;
            REQUIRE(r >= 0 && r < right);
            CHECK_EQ(m.match_right[r], l);
            CHECK(all.count({l, r}) == 1);
        }
    }
    CHECK_EQ(count, m.size);
    for (int r = 0; r < right; ++r) {
        if (m.match_right[r] != -1) {
            CHECK_EQ(m.match_left[m.match_right[r]], r);
        }
    }
}

}  // namespace

TEST(1, "no edges match nothing") {
    const Matching m = max_matching(3, 2, Edges{});
    CHECK_EQ(m.size, 0);
    CHECK_EQ(m.match_left, std::vector<int>{-1, -1, -1});
    CHECK_EQ(m.match_right, std::vector<int>{-1, -1});
}

TEST(1, "augmenting through an earlier match") {
    const Edges edges{{0, 0}, {0, 1}, {1, 0}};
    const Matching m = max_matching(2, 2, edges);
    CHECK_EQ(m.size, 2);
    CHECK_EQ(m.match_left[0], 1);
    CHECK_EQ(m.match_left[1], 0);
}

TEST(1, "duplicate edges do not double count") {
    const Edges edges{{0, 0}, {0, 0}, {1, 0}};
    const Matching m = max_matching(2, 1, edges);
    CHECK_EQ(m.size, 1);
    check_matching(m, 2, 1, edges);
}

TEST(1, "random graphs match exhaustive search") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int left = static_cast<int>(rng.uniform(1, 8));
        const int right = static_cast<int>(rng.uniform(1, 8));
        const auto edges = random_edges(rng, left, right, static_cast<int>(rng.uniform(0, 30)));
        INFO("round ", round);
        const Matching m = max_matching(left, right, edges);
        check_matching(m, left, right, edges);
        CHECK_EQ(m.size, brute_matching(left, right, edges));
    }
}

TEST(2, "cover of no edges is empty") {
    const auto cover = min_vertex_cover(4, 4, Edges{});
    CHECK(cover.left.empty());
    CHECK(cover.right.empty());
}

TEST(2, "a star needs only its center") {
    Edges edges;
    for (int r = 0; r < 5; ++r) {
        edges.emplace_back(0, r);
    }
    const auto cover = min_vertex_cover(1, 5, edges);
    CHECK_EQ(cover.left, std::vector<int>{0});
    CHECK(cover.right.empty());
}

TEST(2, "random graphs: cover is valid and matches matching size") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int left = static_cast<int>(rng.uniform(1, 9));
        const int right = static_cast<int>(rng.uniform(1, 9));
        const auto edges = random_edges(rng, left, right, static_cast<int>(rng.uniform(0, 40)));
        const auto cover = min_vertex_cover(left, right, edges);
        INFO("round ", round);
        std::vector<char> in_left(left, 0), in_right(right, 0);
        for (int l : cover.left) {
            REQUIRE(l >= 0 && l < left);
            in_left[l] = 1;
        }
        for (int r : cover.right) {
            REQUIRE(r >= 0 && r < right);
            in_right[r] = 1;
        }
        CHECK(std::is_sorted(cover.left.begin(), cover.left.end()));
        CHECK(std::is_sorted(cover.right.begin(), cover.right.end()));
        for (const auto& [l, r] : edges) {
            CHECK(in_left[l] || in_right[r]);
        }
        CHECK_EQ(static_cast<int>(cover.left.size() + cover.right.size()),
                 brute_matching(left, right, edges));
    }
}

TEST(3, "hopcroft-karp agrees with exhaustive search") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int left = static_cast<int>(rng.uniform(1, 8));
        const int right = static_cast<int>(rng.uniform(1, 8));
        const auto edges = random_edges(rng, left, right, static_cast<int>(rng.uniform(0, 30)));
        INFO("round ", round);
        const Matching m = hopcroft_karp(left, right, edges);
        check_matching(m, left, right, edges);
        CHECK_EQ(m.size, brute_matching(left, right, edges));
    }
}

TEST(3, "large sparse random graphs still give valid matchings") {
    Rng rng;
    const auto edges = random_edges(rng, 500, 500, 2500);
    const Matching m = hopcroft_karp(500, 500, edges);
    check_matching(m, 500, 500, edges);
    CHECK_GT(m.size, 400);
}

PERF(3, "dense core defeats simple augmenting search", 600) {
    // A complete bipartite core with far more left vertices than right: simple augmenting
    // search re-scans the whole core once per unmatched vertex.
    const int left = 2500;
    const int right = 1200;
    Edges edges;
    edges.reserve(static_cast<std::size_t>(left) * right);
    for (int l = 0; l < left; ++l) {
        for (int r = 0; r < right; ++r) {
            edges.emplace_back(l, r);
        }
    }
    const Matching m = hopcroft_karp(left, right, edges);
    CHECK_EQ(m.size, right);
}

TEST(4, "perfect matching gives no violator") {
    const Edges edges{{0, 0}, {1, 1}};
    CHECK(hall_violator(2, 2, edges).empty());
}

TEST(4, "two applicants fighting over one job") {
    const Edges edges{{0, 0}, {1, 0}, {2, 1}};
    const auto violator = hall_violator(3, 2, edges);
    CHECK_EQ(violator, std::vector<int>{0, 1});
}

TEST(4, "a vertex with no neighbors is a violator by itself") {
    const Edges edges{{0, 0}};
    const auto violator = hall_violator(2, 1, edges);
    REQUIRE(!violator.empty());
}

TEST(4, "random graphs: violator exists exactly when no perfect left matching") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int left = static_cast<int>(rng.uniform(1, 8));
        const int right = static_cast<int>(rng.uniform(1, 8));
        const auto edges = random_edges(rng, left, right, static_cast<int>(rng.uniform(0, 20)));
        INFO("round ", round);
        const auto violator = hall_violator(left, right, edges);
        const bool perfect = brute_matching(left, right, edges) == left;
        CHECK_EQ(violator.empty(), perfect);
        if (!violator.empty()) {
            CHECK(std::is_sorted(violator.begin(), violator.end()));
            std::set<int> neighbors;
            const std::set<int> members(violator.begin(), violator.end());
            CHECK_EQ(members.size(), violator.size());
            for (const auto& [l, r] : edges) {
                if (members.count(l)) {
                    neighbors.insert(r);
                }
            }
            CHECK_LT(neighbors.size(), violator.size());
        }
    }
}
