#include <algorithm>
#include <functional>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Values = std::vector<std::int64_t>;
using Prizes = std::vector<Prize>;
using Tree = std::vector<TreePrize>;
using Chosen = std::vector<int>;

namespace {

void check_indices(const Chosen& chosen, std::size_t n, std::size_t m) {
    CHECK_LE(chosen.size(), m);
    for (std::size_t i = 0; i < chosen.size(); ++i) {
        REQUIRE(chosen[i] >= 0 && static_cast<std::size_t>(chosen[i]) < n);
        if (i > 0) {
            CHECK_LT(chosen[i - 1], chosen[i]);
        }
    }
}

std::int64_t total_of(const Values& values, const Chosen& chosen) {
    std::int64_t total = 0;
    for (int index : chosen) {
        total += values[static_cast<std::size_t>(index)];
    }
    return total;
}

std::int64_t total_of(const Prizes& prizes, const Chosen& chosen) {
    std::int64_t total = 0;
    for (int index : chosen) {
        total += prizes[static_cast<std::size_t>(index)].value;
    }
    return total;
}

std::int64_t total_of(const Tree& tree, const Chosen& chosen) {
    std::int64_t total = 0;
    for (int index : chosen) {
        total += tree[static_cast<std::size_t>(index)].value;
    }
    return total;
}

bool a_before_b(const Prizes& prizes, const Chosen& chosen) {
    bool seen_b = false;
    for (int index : chosen) {
        if (prizes[static_cast<std::size_t>(index)].type == 'B') {
            seen_b = true;
        } else if (seen_b) {
            return false;
        }
    }
    return true;
}

bool non_decreasing(const Values& values, const Chosen& chosen) {
    for (std::size_t i = 1; i < chosen.size(); ++i) {
        if (values[static_cast<std::size_t>(chosen[i])] < values[static_cast<std::size_t>(chosen[i - 1])]) {
            return false;
        }
    }
    return true;
}

// A selection is valid when every chosen non-root node has its parent chosen too.
bool rooted(const Tree& tree, int root, const Chosen& chosen) {
    if (chosen.empty()) {
        return true;
    }
    std::vector<int> parent(tree.size(), -1);
    for (std::size_t u = 0; u < tree.size(); ++u) {
        for (int child : {tree[u].left, tree[u].right}) {
            if (child != -1) {
                parent[static_cast<std::size_t>(child)] = static_cast<int>(u);
            }
        }
    }
    std::vector<char> picked(tree.size(), 0);
    for (int index : chosen) {
        picked[static_cast<std::size_t>(index)] = 1;
    }
    if (!picked[static_cast<std::size_t>(root)]) {
        return false;
    }
    for (int index : chosen) {
        if (index != root && !picked[static_cast<std::size_t>(parent[static_cast<std::size_t>(index)])]) {
            return false;
        }
    }
    return true;
}

// Tries every subset of indices; `value` totals a subset and `valid` accepts or rejects it.
std::int64_t brute(std::size_t n, std::size_t m, const std::function<bool(const Chosen&)>& valid,
                   const std::function<std::int64_t(const Chosen&)>& value) {
    std::int64_t best = 0;
    for (std::uint64_t mask = 0; mask < (std::uint64_t{1} << n); ++mask) {
        Chosen chosen;
        for (std::size_t i = 0; i < n; ++i) {
            if ((mask >> i) & 1) {
                chosen.push_back(static_cast<int>(i));
            }
        }
        if (chosen.size() <= m && valid(chosen)) {
            best = std::max(best, value(chosen));
        }
    }
    return best;
}

Values random_values(Rng& rng, std::size_t n, std::int64_t max_value) {
    Values values(n);
    for (auto& value : values) {
        value = rng.uniform(0, max_value);
    }
    return values;
}

Prizes random_prizes(Rng& rng, std::size_t n, std::int64_t max_value) {
    Prizes prizes(n);
    for (Prize& prize : prizes) {
        prize = {rng.uniform(0, max_value), rng.coin() ? 'A' : 'B'};
    }
    return prizes;
}

// Hangs each new node on a random free child slot, then shuffles the node numbers.
Tree random_tree(Rng& rng, std::size_t n, std::int64_t max_value, int* root) {
    const std::vector<int> label = rng.permutation(n);
    Tree tree(n, TreePrize{0, -1, -1});
    for (TreePrize& node : tree) {
        node.value = rng.uniform(0, max_value);
    }
    std::vector<int*> free_slots;
    const auto add_slots = [&](int id) {
        free_slots.push_back(&tree[static_cast<std::size_t>(id)].left);
        free_slots.push_back(&tree[static_cast<std::size_t>(id)].right);
    };
    *root = label[0];
    add_slots(label[0]);
    for (std::size_t i = 1; i < n; ++i) {
        const auto pick = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(free_slots.size()) - 1));
        *free_slots[pick] = label[i];
        free_slots[pick] = free_slots.back();
        free_slots.pop_back();
        add_slots(label[i]);
    }
    return tree;
}

// Independent of any DP: for a split point, every A before it pairs with every B after it.
std::int64_t split_oracle(const Prizes& prizes, std::size_t m) {
    std::int64_t best = 0;
    for (std::size_t split = 0; split <= prizes.size(); ++split) {
        std::vector<std::int64_t> usable;
        for (std::size_t i = 0; i < prizes.size(); ++i) {
            if ((i < split) == (prizes[i].type == 'A')) {
                usable.push_back(prizes[i].value);
            }
        }
        std::ranges::sort(usable, std::ranges::greater{});
        std::int64_t total = 0;
        for (std::size_t i = 0; i < std::min(m, usable.size()); ++i) {
            total += usable[i];
        }
        best = std::max(best, total);
    }
    return best;
}

std::int64_t chain_oracle(const Values& values, std::size_t m) {
    const std::size_t n = values.size();
    std::vector<std::vector<std::int64_t>> best(m + 1, std::vector<std::int64_t>(n, -1));
    std::int64_t answer = 0;
    for (std::size_t length = 1; length <= m; ++length) {
        for (std::size_t i = 0; i < n; ++i) {
            if (length == 1) {
                best[1][i] = values[i];
            }
            for (std::size_t j = 0; j < i && length > 1; ++j) {
                if (values[j] <= values[i] && best[length - 1][j] >= 0) {
                    best[length][i] = std::max(best[length][i], best[length - 1][j] + values[i]);
                }
            }
            answer = std::max(answer, best[length][i]);
        }
    }
    return answer;
}

std::vector<std::int64_t> subtree_oracle(const Tree& tree, int node, std::size_t m) {
    if (node == -1) {
        return std::vector<std::int64_t>(m + 1, 0);
    }
    const auto left = subtree_oracle(tree, tree[static_cast<std::size_t>(node)].left, m);
    const auto right = subtree_oracle(tree, tree[static_cast<std::size_t>(node)].right, m);
    std::vector<std::int64_t> best(m + 1, 0);
    for (std::size_t k = 1; k <= m; ++k) {
        for (std::size_t j = 0; j < k; ++j) {
            best[k] = std::max(best[k], tree[static_cast<std::size_t>(node)].value + left[j] + right[k - 1 - j]);
        }
    }
    return best;
}

}  // namespace

TEST(1, "an empty budget takes nothing") {
    CHECK(pick_top(Values{3, 1}, 0).empty());
}

TEST(1, "no prizes means nothing to take") {
    CHECK(pick_top(Values{}, 3).empty());
}

TEST(1, "the largest values win and stay in their original order") {
    CHECK_EQ(pick_top(Values{3, 9, 1, 7}, 2), (Chosen{1, 3}));
}

TEST(1, "a budget larger than the list takes everything") {
    CHECK_EQ(pick_top(Values{4, 1}, 5), (Chosen{0, 1}));
}

TEST(1, "ties may be broken either way") {
    const Values values{5, 5, 5};
    const Chosen chosen = pick_top(values, 2);
    check_indices(chosen, values.size(), 2);
    CHECK_EQ(total_of(values, chosen), 10);
}

TEST(1, "negative values are rejected") {
    CHECK_THROWS_AS(pick_top(Values{1, -1}, 1), std::invalid_argument);
}

TEST(1, "random inputs match sorting") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        INFO("trial ", trial);
        const Values values = random_values(rng, static_cast<std::size_t>(rng.uniform(0, 60)), 30);
        const auto m = static_cast<std::size_t>(rng.uniform(0, 70));
        const Chosen chosen = pick_top(values, m);
        check_indices(chosen, values.size(), m);
        Values sorted = values;
        std::ranges::sort(sorted, std::ranges::greater{});
        std::int64_t expected = 0;
        for (std::size_t i = 0; i < std::min(m, sorted.size()); ++i) {
            expected += sorted[i];
        }
        REQUIRE_EQ(total_of(values, chosen), expected);
    }
}

PERF(1, "picking half of five million prizes", 2500) {
    Rng rng;
    const Values values = random_values(rng, 5'000'000, 1'000'000'000);
    const Chosen chosen = pick_top(values, 2'500'000);
    CHECK_EQ(chosen.size(), 2'500'000);
}

TEST(2, "an empty budget or list takes nothing") {
    CHECK(pick_a_then_b(Prizes{{5, 'A'}}, 0).empty());
    CHECK(pick_a_then_b(Prizes{}, 3).empty());
}

TEST(2, "invalid prizes are rejected") {
    CHECK_THROWS_AS(pick_a_then_b(Prizes{{1, 'C'}}, 1), std::invalid_argument);
    CHECK_THROWS_AS(pick_a_then_b(Prizes{{-1, 'A'}}, 1), std::invalid_argument);
}

TEST(2, "a single type is an unconstrained choice") {
    const Prizes all_a{{1, 'A'}, {2, 'A'}, {3, 'A'}};
    CHECK_EQ(total_of(all_a, pick_a_then_b(all_a, 2)), 5);
    const Prizes all_b{{1, 'B'}, {2, 'B'}, {3, 'B'}};
    CHECK_EQ(total_of(all_b, pick_a_then_b(all_b, 2)), 5);
}

TEST(2, "an A followed by a B may both be taken") {
    const Prizes prizes{{10, 'A'}, {10, 'B'}};
    CHECK_EQ(total_of(prizes, pick_a_then_b(prizes, 2)), 20);
}

TEST(2, "a B followed by an A may not both be taken") {
    const Prizes prizes{{10, 'B'}, {10, 'A'}};
    const Chosen chosen = pick_a_then_b(prizes, 2);
    CHECK(a_before_b(prizes, chosen));
    CHECK_EQ(total_of(prizes, chosen), 10);
}

TEST(2, "the best split can skip an A in the middle") {
    const Prizes prizes{{5, 'A'}, {9, 'B'}, {4, 'A'}, {3, 'B'}};
    const Chosen chosen = pick_a_then_b(prizes, 3);
    check_indices(chosen, prizes.size(), 3);
    CHECK(a_before_b(prizes, chosen));
    CHECK_EQ(total_of(prizes, chosen), 17);
}

TEST(2, "the budget limits the combined count") {
    const Prizes prizes{{5, 'A'}, {9, 'B'}, {4, 'A'}, {3, 'B'}};
    CHECK_EQ(total_of(prizes, pick_a_then_b(prizes, 1)), 9);
}

TEST(2, "random inputs match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        INFO("trial ", trial);
        const Prizes prizes = random_prizes(rng, static_cast<std::size_t>(rng.uniform(1, 11)), 20);
        const auto m = static_cast<std::size_t>(rng.uniform(0, 12));
        const Chosen chosen = pick_a_then_b(prizes, m);
        check_indices(chosen, prizes.size(), m);
        CHECK(a_before_b(prizes, chosen));
        const auto expected = brute(prizes.size(), m,
                                    [&](const Chosen& c) { return a_before_b(prizes, c); },
                                    [&](const Chosen& c) { return total_of(prizes, c); });
        REQUIRE_EQ(total_of(prizes, chosen), expected);
    }
}

TEST(2, "larger inputs match an independent split search") {
    Rng rng;
    for (int trial = 0; trial < 40; ++trial) {
        INFO("trial ", trial);
        const Prizes prizes = random_prizes(rng, 300, 1'000'000);
        const auto m = static_cast<std::size_t>(rng.uniform(1, 60));
        const Chosen chosen = pick_a_then_b(prizes, m);
        check_indices(chosen, prizes.size(), m);
        CHECK(a_before_b(prizes, chosen));
        CHECK_EQ(total_of(prizes, chosen), split_oracle(prizes, m));
    }
}

PERF(2, "ten thousand prizes with a budget of two thousand", 1500) {
    Rng rng;
    const Prizes prizes = random_prizes(rng, 10'000, 1'000'000);
    const Chosen chosen = pick_a_then_b(prizes, 2'000);
    CHECK(a_before_b(prizes, chosen));
}

TEST(3, "an empty budget or list takes nothing") {
    CHECK(pick_non_decreasing(Values{5}, 0).empty());
    CHECK(pick_non_decreasing(Values{}, 3).empty());
}

TEST(3, "negative values are rejected") {
    CHECK_THROWS_AS(pick_non_decreasing(Values{-1}, 1), std::invalid_argument);
}

TEST(3, "an increasing run is taken whole") {
    const Values values{1, 2, 3};
    CHECK_EQ(pick_non_decreasing(values, 3), (Chosen{0, 1, 2}));
}

TEST(3, "a strictly decreasing run takes only its largest element") {
    const Values values{9, 7, 5};
    CHECK_EQ(total_of(values, pick_non_decreasing(values, 3)), 9);
}

TEST(3, "equal values count as non-decreasing") {
    const Values values{4, 4, 4};
    CHECK_EQ(total_of(values, pick_non_decreasing(values, 2)), 8);
    CHECK_EQ(total_of(values, pick_non_decreasing(values, 3)), 12);
}

TEST(3, "the budget caps the length of the chain") {
    const Values values{1, 2, 3, 4};
    CHECK_EQ(pick_non_decreasing(values, 2), (Chosen{2, 3}));
}

TEST(3, "a large early value can beat a long small chain") {
    const Values values{10, 1, 2, 3};
    CHECK_EQ(total_of(values, pick_non_decreasing(values, 4)), 10);
}

TEST(3, "the best chain can skip elements") {
    const Values values{5, 1, 6, 2, 7};
    CHECK_EQ(total_of(values, pick_non_decreasing(values, 2)), 13);
    CHECK_EQ(total_of(values, pick_non_decreasing(values, 3)), 18);
}

TEST(3, "large values do not overflow") {
    Values values(2000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = 1'000'000'000'000 + static_cast<std::int64_t>(i);
    }
    CHECK_EQ(pick_non_decreasing(values, 2000).size(), 2000);
}

TEST(3, "random inputs match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        INFO("trial ", trial);
        const Values values = random_values(rng, static_cast<std::size_t>(rng.uniform(1, 11)), rng.coin() ? 5 : 40);
        const auto m = static_cast<std::size_t>(rng.uniform(0, 12));
        const Chosen chosen = pick_non_decreasing(values, m);
        check_indices(chosen, values.size(), m);
        CHECK(non_decreasing(values, chosen));
        const auto expected = brute(values.size(), m,
                                    [&](const Chosen& c) { return non_decreasing(values, c); },
                                    [&](const Chosen& c) { return total_of(values, c); });
        REQUIRE_EQ(total_of(values, chosen), expected);
    }
}

TEST(3, "larger inputs match a direct dynamic program") {
    Rng rng;
    for (int trial = 0; trial < 25; ++trial) {
        INFO("trial ", trial);
        const Values values = random_values(rng, 300, rng.coin() ? 10 : 1'000'000);
        const auto m = static_cast<std::size_t>(rng.uniform(1, 30));
        const Chosen chosen = pick_non_decreasing(values, m);
        check_indices(chosen, values.size(), m);
        CHECK(non_decreasing(values, chosen));
        CHECK_EQ(total_of(values, chosen), chain_oracle(values, m));
    }
}

TEST(4, "an empty tree or budget takes nothing") {
    CHECK(pick_rooted_subtree(Tree{}, -1, 3).empty());
    CHECK(pick_rooted_subtree(Tree{{5, -1, -1}}, 0, 0).empty());
}

TEST(4, "an invalid root or a negative value is rejected") {
    CHECK_THROWS_AS(pick_rooted_subtree(Tree{{1, -1, -1}}, 1, 1), std::invalid_argument);
    CHECK_THROWS_AS(pick_rooted_subtree(Tree{{1, -1, -1}}, -1, 1), std::invalid_argument);
    CHECK_THROWS_AS(pick_rooted_subtree(Tree{{-1, -1, -1}}, 0, 1), std::invalid_argument);
}

TEST(4, "a single node") {
    CHECK_EQ(pick_rooted_subtree(Tree{{5, -1, -1}}, 0, 3), Chosen{0});
}

TEST(4, "a child cannot be taken without its parent") {
    const Tree tree{{0, 1, -1}, {100, -1, -1}};
    CHECK_EQ(total_of(tree, pick_rooted_subtree(tree, 0, 1)), 0);
    CHECK_EQ(total_of(tree, pick_rooted_subtree(tree, 0, 2)), 100);
}

TEST(4, "the budget is divided between the two children") {
    const Tree tree{{1, 1, 2}, {10, 3, -1}, {20, -1, -1}, {100, -1, -1}};
    const Chosen chosen = pick_rooted_subtree(tree, 0, 3);
    CHECK(rooted(tree, 0, chosen));
    CHECK_EQ(total_of(tree, chosen), 111);
    CHECK_EQ(total_of(tree, pick_rooted_subtree(tree, 0, 4)), 131);
}

TEST(4, "the root need not be node zero") {
    const Tree tree{{5, -1, -1}, {7, -1, -1}, {1, 0, 1}};
    CHECK_EQ(pick_rooted_subtree(tree, 2, 2), (Chosen{1, 2}));
}

TEST(4, "random trees match an exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        INFO("trial ", trial);
        int root = 0;
        const Tree tree = random_tree(rng, static_cast<std::size_t>(rng.uniform(1, 11)), 20, &root);
        const auto m = static_cast<std::size_t>(rng.uniform(0, 12));
        const Chosen chosen = pick_rooted_subtree(tree, root, m);
        check_indices(chosen, tree.size(), m);
        CHECK(rooted(tree, root, chosen));
        const auto expected = brute(tree.size(), m,
                                    [&](const Chosen& c) { return rooted(tree, root, c); },
                                    [&](const Chosen& c) { return total_of(tree, c); });
        REQUIRE_EQ(total_of(tree, chosen), expected);
    }
}

TEST(4, "larger trees match a direct dynamic program") {
    Rng rng;
    for (int trial = 0; trial < 25; ++trial) {
        INFO("trial ", trial);
        int root = 0;
        const Tree tree = random_tree(rng, 400, 1'000'000, &root);
        const auto m = static_cast<std::size_t>(rng.uniform(1, 30));
        const Chosen chosen = pick_rooted_subtree(tree, root, m);
        check_indices(chosen, tree.size(), m);
        CHECK(rooted(tree, root, chosen));
        CHECK_EQ(total_of(tree, chosen), subtree_oracle(tree, root, m)[m]);
    }
}

TEST(4, "a very deep path does not overflow the stack") {
    const int n = 500'000;
    Tree tree(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        tree[static_cast<std::size_t>(i)] = {1, i + 1 < n ? i + 1 : -1, -1};
    }
    CHECK_EQ(pick_rooted_subtree(tree, 0, 2), (Chosen{0, 1}));
}

PERF(5, "long non-decreasing chains", 3000) {
    Rng rng;
    const Values values = random_values(rng, 20'000, 1'000'000);
    const Chosen chosen = pick_non_decreasing(values, 150);
    CHECK(non_decreasing(values, chosen));
}

PERF(5, "a large balanced tree", 3000) {
    const int n = 200'000;
    Tree tree(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        tree[static_cast<std::size_t>(i)] = {i % 1000, 2 * i + 1 < n ? 2 * i + 1 : -1, 2 * i + 2 < n ? 2 * i + 2 : -1};
    }
    CHECK_EQ(pick_rooted_subtree(tree, 0, 400).size(), 400);
}

PERF(5, "a long path with a wide budget", 3000) {
    const int n = 100'000;
    Tree tree(static_cast<std::size_t>(n));
    for (int i = 0; i < n; ++i) {
        tree[static_cast<std::size_t>(i)] = {1, i + 1 < n ? i + 1 : -1, -1};
    }
    CHECK_EQ(pick_rooted_subtree(tree, 0, 200).size(), 200);
}
