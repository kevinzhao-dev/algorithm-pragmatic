#include <algorithm>
#include <functional>
#include <numeric>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Graph random_graph(Rng& rng, int n, double density) {
    Graph g;
    g.n = n;
    for (int u = 0; u < n; ++u) {
        for (int v = u + 1; v < n; ++v) {
            if (rng.coin(density)) {
                g.edges.emplace_back(u, v);
            }
        }
    }
    return g;
}

// Exhaustive searches over vertex subsets (n <= 12): smallest / largest with a property.
std::optional<std::vector<int>> find_subset(int n, int size,
                                            const std::function<bool(const std::vector<int>&)>& ok) {
    for (int mask = 0; mask < (1 << n); ++mask) {
        if (__builtin_popcount(mask) != size) {
            continue;
        }
        std::vector<int> subset;
        for (int v = 0; v < n; ++v) {
            if (mask >> v & 1) {
                subset.push_back(v);
            }
        }
        if (ok(subset)) {
            return subset;
        }
    }
    return std::nullopt;
}

bool brute_clique(const Graph& g, const std::vector<int>& s) {
    for (std::size_t i = 0; i < s.size(); ++i) {
        for (std::size_t j = i + 1; j < s.size(); ++j) {
            bool adjacent = false;
            for (const auto& [u, v] : g.edges) {
                adjacent = adjacent || (u == s[i] && v == s[j]) || (u == s[j] && v == s[i]);
            }
            if (!adjacent) {
                return false;
            }
        }
    }
    return true;
}

bool brute_is(const Graph& g, const std::vector<int>& s) {
    const std::set<int> in(s.begin(), s.end());
    for (const auto& [u, v] : g.edges) {
        if (in.count(u) && in.count(v)) {
            return false;
        }
    }
    return true;
}

bool brute_vc(const Graph& g, const std::vector<int>& s) {
    const std::set<int> in(s.begin(), s.end());
    for (const auto& [u, v] : g.edges) {
        if (!in.count(u) && !in.count(v)) {
            return false;
        }
    }
    return true;
}

std::optional<std::vector<int>> brute_set_cover(const SetCoverInstance& inst) {
    const int m = static_cast<int>(inst.sets.size());
    for (int mask = 0; mask < (1 << m); ++mask) {
        if (__builtin_popcount(mask) != inst.k) {
            continue;
        }
        std::set<int> covered;
        std::vector<int> chosen;
        for (int i = 0; i < m; ++i) {
            if (mask >> i & 1) {
                chosen.push_back(i);
                covered.insert(inst.sets[i].begin(), inst.sets[i].end());
            }
        }
        if (static_cast<int>(covered.size()) == inst.universe) {
            return chosen;
        }
    }
    return std::nullopt;
}

}  // namespace

TEST(1, "empty subsets are valid certificates") {
    const Graph g{3, {{0, 1}}};
    const std::vector<int> none;
    CHECK(is_clique(g, none));
    CHECK(is_independent_set(g, none));
    CHECK(!is_vertex_cover(g, none));
    CHECK(is_vertex_cover(Graph{3, {}}, none));
    CHECK(is_set_cover(0, {{}}, none));
    CHECK(!is_set_cover(1, {{0}}, none));
}

TEST(1, "verifiers on a triangle with a pendant vertex") {
    const Graph g{4, {{0, 1}, {1, 2}, {0, 2}, {2, 3}}};
    const std::vector<int> triangle{0, 1, 2};
    CHECK(is_clique(g, triangle));
    CHECK(!is_independent_set(g, triangle));
    CHECK(!is_independent_set(g, std::vector<int>{0, 2, 3}));
    CHECK(is_vertex_cover(g, std::vector<int>{1, 2}));
    CHECK(!is_vertex_cover(g, std::vector<int>{0, 3}));
}

TEST(1, "set cover checks every element") {
    const std::vector<std::vector<int>> sets{{0, 1}, {1, 2}, {3}};
    CHECK(is_set_cover(4, sets, std::vector<int>{0, 1, 2}));
    CHECK(!is_set_cover(4, sets, std::vector<int>{0, 1}));
}

TEST(1, "random certificates agree with direct checks") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 8));
        const Graph g = random_graph(rng, n, 0.5);
        std::vector<int> s;
        for (int v = 0; v < n; ++v) {
            if (rng.coin(0.5)) {
                s.push_back(v);
            }
        }
        INFO("round ", round);
        CHECK_EQ(is_clique(g, s), brute_clique(g, s));
        CHECK_EQ(is_independent_set(g, s), brute_is(g, s));
        CHECK_EQ(is_vertex_cover(g, s), brute_vc(g, s));
    }
}

TEST(2, "clique maps to independent set on the complement graph") {
    const CliqueInstance src{Graph{3, {{0, 1}}}, 2};
    const IsInstance dst = clique_to_is(src);
    CHECK_EQ(dst.k, 2);
    CHECK_EQ(dst.g.n, 3);
    CHECK_EQ(dst.g.edges.size(), 2);
}

TEST(2, "random instances: reductions preserve the answer and lifts return witnesses") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 7));
        const int k = static_cast<int>(rng.uniform(0, n));
        const CliqueInstance clique{random_graph(rng, n, 0.5), k};
        INFO("round ", round, " n=", n, " k=", k);

        const IsInstance is = clique_to_is(clique);
        REQUIRE_EQ(is.g.n, n);
        REQUIRE_EQ(is.k, k);
        const auto clique_answer =
            find_subset(n, k, [&](const auto& s) { return brute_clique(clique.g, s); });
        const auto is_answer =
            find_subset(n, k, [&](const auto& s) { return brute_is(is.g, s); });
        CHECK_EQ(clique_answer.has_value(), is_answer.has_value());
        if (is_answer) {
            const auto witness = lift_is_to_clique(clique, *is_answer);
            CHECK(brute_clique(clique.g, witness));
            CHECK_EQ(witness.size(), k);
        }

        const IsInstance is_src{random_graph(rng, n, 0.5), k};
        const VcInstance vc = is_to_vc(is_src);
        REQUIRE_EQ(vc.g.n, n);
        const auto is_yes =
            find_subset(n, k, [&](const auto& s) { return brute_is(is_src.g, s); });
        const auto vc_yes =
            find_subset(n, vc.k, [&](const auto& s) { return brute_vc(vc.g, s); });
        CHECK_EQ(is_yes.has_value(), vc_yes.has_value());
        if (vc_yes) {
            const auto witness = lift_vc_to_is(is_src, *vc_yes);
            CHECK(brute_is(is_src.g, witness));
            CHECK_EQ(static_cast<int>(witness.size()), k);
        }

        const VcInstance vc_src{random_graph(rng, n, 0.4), k};
        const SetCoverInstance sc = vc_to_set_cover(vc_src);
        const auto vc_answer =
            find_subset(n, k, [&](const auto& s) { return brute_vc(vc_src.g, s); });
        const auto sc_answer = brute_set_cover(sc);
        CHECK_EQ(vc_answer.has_value(), sc_answer.has_value());
        if (sc_answer) {
            const auto witness = lift_set_cover_to_vc(vc_src, sc, *sc_answer);
            CHECK(brute_vc(vc_src.g, witness));
            CHECK_LE(static_cast<int>(witness.size()), k);
        }
    }
}

namespace {

std::optional<std::vector<int>> brute_partition(const std::vector<long long>& a) {
    const long long total = std::accumulate(a.begin(), a.end(), 0LL);
    if (total % 2 != 0) {
        return std::nullopt;
    }
    const int n = static_cast<int>(a.size());
    for (int mask = 0; mask < (1 << n); ++mask) {
        long long sum = 0;
        std::vector<int> side;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                sum += a[i];
                side.push_back(i);
            }
        }
        if (2 * sum == total) {
            return side;
        }
    }
    return std::nullopt;
}

bool has_subset_sum(const SubsetSumInstance& inst) {
    const int n = static_cast<int>(inst.a.size());
    for (int mask = 0; mask < (1 << n); ++mask) {
        long long sum = 0;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                sum += inst.a[i];
            }
        }
        if (sum == inst.t) {
            return true;
        }
    }
    return false;
}

}  // namespace

TEST(3, "worked example: a yes-instance maps to a balanced partition") {
    const SubsetSumInstance src{{3, 5, 7}, 8};
    const PartitionInstance dst = subset_sum_to_partition(src);
    const auto side = brute_partition(dst.a);
    REQUIRE(side.has_value());
    const auto subset = lift_partition_to_subset_sum(src, dst, *side);
    long long sum = 0;
    for (int i : subset) {
        REQUIRE(i >= 0 && i < 3);
        sum += src.a[i];
    }
    CHECK_EQ(sum, 8);
}

TEST(3, "no-instances stay no-instances") {
    const SubsetSumInstance src{{2, 4}, 3};
    CHECK(!brute_partition(subset_sum_to_partition(src).a).has_value());
}

TEST(3, "random instances preserve the answer") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 10));
        SubsetSumInstance src;
        for (int i = 0; i < n; ++i) {
            src.a.push_back(rng.uniform(0, 30));
        }
        const long long total = std::accumulate(src.a.begin(), src.a.end(), 0LL);
        src.t = rng.uniform(0, total);
        INFO("round ", round);
        const PartitionInstance dst = subset_sum_to_partition(src);
        const auto side = brute_partition(dst.a);
        CHECK_EQ(side.has_value(), has_subset_sum(src));
        if (side) {
            const auto subset = lift_partition_to_subset_sum(src, dst, *side);
            long long sum = 0;
            std::set<int> seen;
            for (int i : subset) {
                REQUIRE(i >= 0 && i < n);
                CHECK(seen.insert(i).second);
                sum += src.a[i];
            }
            CHECK_EQ(sum, src.t);
        }
    }
}

namespace {

std::optional<std::vector<int>> brute_ham_path(const Digraph& g) {
    std::vector<int> order(static_cast<std::size_t>(g.n));
    std::iota(order.begin(), order.end(), 0);
    do {
        bool ok = true;
        for (std::size_t i = 0; ok && i + 1 < order.size(); ++i) {
            ok = std::find(g.edges.begin(), g.edges.end(),
                           std::make_pair(order[i], order[i + 1])) != g.edges.end();
        }
        if (ok) {
            return order;
        }
    } while (std::next_permutation(order.begin(), order.end()));
    return std::nullopt;
}

bool brute_ham_cycle(const Digraph& g) {
    if (g.n < 2) {
        return false;
    }
    std::vector<int> order(static_cast<std::size_t>(g.n));
    std::iota(order.begin(), order.end(), 0);
    do {
        if (order[0] != 0) {
            break;
        }
        bool ok = true;
        for (std::size_t i = 0; ok && i < order.size(); ++i) {
            const int a = order[i];
            const int b = order[(i + 1) % order.size()];
            ok = std::find(g.edges.begin(), g.edges.end(), std::make_pair(a, b)) != g.edges.end();
        }
        if (ok) {
            return true;
        }
    } while (std::next_permutation(order.begin(), order.end()));
    return false;
}

}  // namespace

TEST(4, "a cycle becomes a path after splitting a vertex") {
    const Digraph g{3, {{0, 1}, {1, 2}, {2, 0}}};
    const Digraph dst = ham_cycle_to_path(g);
    const auto path = brute_ham_path(dst);
    REQUIRE(path.has_value());
    const auto cycle = lift_path_to_cycle(g, dst, *path);
    REQUIRE_EQ(cycle.size(), 3);
    for (std::size_t i = 0; i < 3; ++i) {
        CHECK(std::find(g.edges.begin(), g.edges.end(),
                        std::make_pair(cycle[i], cycle[(i + 1) % 3])) != g.edges.end());
    }
}

TEST(4, "random digraphs preserve the answer") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(2, 6));
        Digraph g{n, {}};
        for (int u = 0; u < n; ++u) {
            for (int v = 0; v < n; ++v) {
                if (u != v && rng.coin(0.5)) {
                    g.edges.emplace_back(u, v);
                }
            }
        }
        INFO("round ", round);
        const Digraph dst = ham_cycle_to_path(g);
        const auto path = brute_ham_path(dst);
        CHECK_EQ(path.has_value(), brute_ham_cycle(g));
        if (path) {
            const auto cycle = lift_path_to_cycle(g, dst, *path);
            REQUIRE_EQ(cycle.size(), n);
            std::vector<int> sorted_cycle = cycle;
            std::sort(sorted_cycle.begin(), sorted_cycle.end());
            std::vector<int> all(n);
            std::iota(all.begin(), all.end(), 0);
            CHECK_EQ(sorted_cycle, all);
            for (std::size_t i = 0; i < cycle.size(); ++i) {
                CHECK(std::find(g.edges.begin(), g.edges.end(),
                                std::make_pair(cycle[i], cycle[(i + 1) % cycle.size()])) !=
                      g.edges.end());
            }
        }
    }
}

namespace {

std::optional<std::vector<int>> brute_3dm(const ThreeDm& t) {
    const int m = static_cast<int>(t.triples.size());
    for (int mask = 0; mask < (1 << m); ++mask) {
        if (__builtin_popcount(mask) != t.n) {
            continue;
        }
        std::set<int> xs, ys, zs;
        std::vector<int> chosen;
        bool ok = true;
        for (int i = 0; i < m && ok; ++i) {
            if (mask >> i & 1) {
                chosen.push_back(i);
                ok = xs.insert(t.triples[i][0]).second && ys.insert(t.triples[i][1]).second &&
                     zs.insert(t.triples[i][2]).second;
            }
        }
        if (ok) {
            return chosen;
        }
    }
    return std::nullopt;
}

std::optional<std::vector<int>> brute_subset_sum(const SubsetSumInstance& inst) {
    const int n = static_cast<int>(inst.a.size());
    for (int mask = 0; mask < (1 << n); ++mask) {
        long long sum = 0;
        std::vector<int> chosen;
        for (int i = 0; i < n; ++i) {
            if (mask >> i & 1) {
                sum += inst.a[i];
                chosen.push_back(i);
            }
        }
        if (sum == inst.t) {
            return chosen;
        }
    }
    return std::nullopt;
}

}  // namespace

TEST(5, "a perfect 3D matching maps to an exact sum") {
    const ThreeDm t{2, {{0, 0, 0}, {1, 1, 1}, {0, 1, 1}}};
    const auto dst = three_dm_to_subset_sum(t);
    const auto subset = brute_subset_sum(dst);
    REQUIRE(subset.has_value());
    CHECK_EQ(lift_subset_sum_to_3dm(t, dst, *subset), std::vector<int>{0, 1});
}

TEST(5, "no perfect matching means no subset") {
    const ThreeDm t{2, {{0, 0, 0}, {0, 1, 1}}};
    CHECK(!brute_subset_sum(three_dm_to_subset_sum(t)).has_value());
}

TEST(5, "random instances preserve the answer") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const int n = static_cast<int>(rng.uniform(1, 3));
        const int m = static_cast<int>(rng.uniform(1, 10));
        ThreeDm t{n, {}};
        for (int i = 0; i < m; ++i) {
            t.triples.push_back({static_cast<int>(rng.uniform(0, n - 1)),
                                 static_cast<int>(rng.uniform(0, n - 1)),
                                 static_cast<int>(rng.uniform(0, n - 1))});
        }
        INFO("round ", round);
        const auto dst = three_dm_to_subset_sum(t);
        REQUIRE_EQ(dst.a.size(), t.triples.size());
        const auto subset = brute_subset_sum(dst);
        const auto matching = brute_3dm(t);
        CHECK_EQ(subset.has_value(), matching.has_value());
        if (subset) {
            const auto witness = lift_subset_sum_to_3dm(t, dst, *subset);
            REQUIRE_EQ(witness.size(), n);
            std::set<int> xs, ys, zs;
            for (int i : witness) {
                CHECK(xs.insert(t.triples[i][0]).second);
                CHECK(ys.insert(t.triples[i][1]).second);
                CHECK(zs.insert(t.triples[i][2]).second);
            }
        }
    }
}
