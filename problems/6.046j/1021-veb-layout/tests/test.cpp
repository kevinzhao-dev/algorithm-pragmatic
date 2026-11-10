#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::vector<long long> even_keys(std::size_t n) {
    std::vector<long long> keys(n);
    for (std::size_t i = 0; i < n; ++i) {
        keys[i] = static_cast<long long>(i) * 2;
    }
    return keys;
}

void check_bst(const std::vector<VebNode>& nodes, const std::vector<long long>& keys) {
    REQUIRE_EQ(nodes.size(), keys.size());
    if (nodes.empty()) {
        return;
    }
    std::vector<long long> inorder;
    std::vector<int> seen(nodes.size(), 0);
    int max_depth = 0;
    const std::function<void(int, int)> walk = [&](int v, int depth) {
        REQUIRE(v >= 0 && v < static_cast<int>(nodes.size()));
        REQUIRE(seen[v] == 0);
        seen[v] = 1;
        max_depth = std::max(max_depth, depth);
        if (nodes[v].left != -1) {
            walk(nodes[v].left, depth + 1);
        }
        inorder.push_back(nodes[v].key);
        if (nodes[v].right != -1) {
            walk(nodes[v].right, depth + 1);
        }
    };
    walk(0, 1);
    CHECK_EQ(inorder, keys);
    int ceil_lg = 0;
    while ((1ULL << ceil_lg) < keys.size() + 1) {
        ++ceil_lg;
    }
    CHECK_LE(max_depth, ceil_lg);
}

// Independent reference for search_blocks.
int reference_blocks(const std::vector<VebNode>& nodes, long long key, std::size_t block_bytes) {
    std::set<std::size_t> blocks;
    int v = nodes.empty() ? -1 : 0;
    while (v != -1) {
        blocks.insert(static_cast<std::size_t>(v) * sizeof(VebNode) / block_bytes);
        if (nodes[v].key == key) {
            break;
        }
        v = key < nodes[v].key ? nodes[v].left : nodes[v].right;
    }
    return static_cast<int>(blocks.size());
}

double mean_blocks(const std::vector<VebNode>& nodes, std::size_t block_bytes, int queries,
                   long long max_key) {
    Rng rng;
    double total = 0;
    for (int q = 0; q < queries; ++q) {
        total += reference_blocks(nodes, rng.uniform(-1, max_key), block_bytes);
    }
    return total / queries;
}

}  // namespace

TEST(1, "no keys and one key") {
    CHECK(build_veb_layout(std::vector<long long>{}).empty());
    const std::vector<long long> one{42};
    const auto nodes = build_veb_layout(one);
    check_bst(nodes, one);
    CHECK_EQ(nodes[0].key, 42);
    CHECK_EQ(nodes[0].left, -1);
    CHECK_EQ(nodes[0].right, -1);
}

TEST(1, "every key appears exactly once in a balanced tree") {
    for (std::size_t n : {2u, 3u, 7u, 8u, 15u, 16u, 31u, 100u, 1023u, 1024u}) {
        INFO("n=", n);
        const auto keys = even_keys(n);
        check_bst(build_veb_layout(keys), keys);
    }
}

TEST(1, "the top half of the tree comes first in memory") {
    for (int h : {2, 3, 4, 8, 12, 16}) {
        const std::size_t n = (1ULL << h) - 1;
        const auto keys = even_keys(n);
        const auto nodes = build_veb_layout(keys);
        check_bst(nodes, keys);
        const int top_levels = h / 2;
        const std::size_t prefix = (1ULL << top_levels) - 1;
        // Mark every node at depth < top_levels; they must exactly fill the first positions.
        std::vector<char> shallow(n, 0);
        const std::function<void(int, int)> walk = [&](int v, int depth) {
            if (v == -1 || depth >= top_levels) {
                return;
            }
            shallow[v] = 1;
            walk(nodes[v].left, depth + 1);
            walk(nodes[v].right, depth + 1);
        };
        walk(0, 0);
        INFO("h=", h);
        std::size_t count = 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (shallow[i]) {
                CHECK_LT(i, prefix);
                ++count;
            }
        }
        CHECK_EQ(count, prefix);
    }
}

TEST(1, "every recursive subtree occupies a contiguous range") {
    const int h = 12;
    const std::size_t n = (1ULL << h) - 1;
    const auto keys = even_keys(n);
    const auto nodes = build_veb_layout(keys);
    // At every node, the subtree at height h/2 below the top half occupies adjacent slots:
    // here we check the bottom subtrees of the top split are contiguous and consecutive.
    const int top_levels = h / 2;
    std::vector<int> roots;  // roots of the bottom subtrees, left to right
    const std::function<void(int, int)> gather = [&](int v, int depth) {
        if (depth == top_levels) {
            roots.push_back(v);
            return;
        }
        gather(nodes[v].left, depth + 1);
        gather(nodes[v].right, depth + 1);
    };
    gather(0, 0);
    std::size_t next = (1ULL << top_levels) - 1;
    for (int root : roots) {
        // Collect the subtree's positions.
        std::vector<int> pos;
        const std::function<void(int)> walk = [&](int v) {
            if (v == -1) {
                return;
            }
            pos.push_back(v);
            walk(nodes[v].left);
            walk(nodes[v].right);
        };
        walk(root);
        std::sort(pos.begin(), pos.end());
        REQUIRE_EQ(pos.size(), (1ULL << (h - top_levels)) - 1);
        CHECK_EQ(static_cast<std::size_t>(pos.front()), next);
        CHECK_EQ(static_cast<std::size_t>(pos.back()), next + pos.size() - 1);
        next += pos.size();
    }
    CHECK_EQ(next, n);
}

TEST(2, "search_blocks counts the distinct blocks on the search path") {
    Rng rng;
    const auto keys = even_keys(5000);
    const auto layouts = {build_veb_layout(keys), build_bfs_layout(keys)};
    for (const auto& nodes : layouts) {
        for (std::size_t block : {16u, 64u, 512u, 4096u}) {
            for (int q = 0; q < 200; ++q) {
                const long long key = rng.uniform(-1, 10'002);
                INFO("block ", block, " key ", key);
                CHECK_EQ(search_blocks(nodes, key, block), reference_blocks(nodes, key, block));
            }
        }
    }
}

TEST(2, "an empty layout touches no blocks") {
    CHECK_EQ(search_blocks(std::vector<VebNode>{}, 3, 64), 0);
}

TEST(2, "bfs layout stores the root first and follows the same tree") {
    const auto keys = even_keys(1000);
    const auto bfs = build_bfs_layout(keys);
    check_bst(bfs, keys);
    // Level order: children positions are at least as large as the parent's.
    for (std::size_t i = 0; i < bfs.size(); ++i) {
        if (bfs[i].left != -1) {
            CHECK_GT(bfs[i].left, static_cast<int>(i));
        }
        if (bfs[i].right != -1) {
            CHECK_GT(bfs[i].right, static_cast<int>(i));
        }
    }
}

PERF(2, "vEB beats the BFS layout at every block size without knowing it", 1000) {
    const std::size_t n = 1ULL << 20;
    const auto keys = even_keys(n);
    const auto veb = build_veb_layout(keys);
    const auto bfs = build_bfs_layout(keys);
    check_bst(veb, keys);
    for (std::size_t block : {std::size_t{4096}, std::size_t{65536}}) {
        const double nodes_per_block = static_cast<double>(block) / sizeof(VebNode);
        const double veb_blocks = mean_blocks(veb, block, 3000, static_cast<long long>(n) * 2);
        const double bfs_blocks = mean_blocks(bfs, block, 3000, static_cast<long long>(n) * 2);
        INFO("block ", block, " veb ", veb_blocks, " bfs ", bfs_blocks);
        CHECK_LE(veb_blocks, 4 * std::ceil(std::log(static_cast<double>(n)) /
                                           std::log(nodes_per_block)) + 2);
        CHECK_LT(veb_blocks, 0.75 * bfs_blocks);
    }
}

TEST(3, "an empty index") {
    const VebIndex index{{}};
    CHECK(!index.contains(1));
    CHECK(!index.predecessor(1).has_value());
}

TEST(3, "predecessor boundaries") {
    const VebIndex index{{10, 20, 30}};
    CHECK(!index.predecessor(9).has_value());
    CHECK_EQ(index.predecessor(10), std::optional<long long>(10));
    CHECK_EQ(index.predecessor(25), std::optional<long long>(20));
    CHECK_EQ(index.predecessor(1000), std::optional<long long>(30));
}

TEST(3, "random queries agree with a sorted set") {
    Rng rng;
    for (int round = 0; round < 50; ++round) {
        std::set<long long> model;
        const int n = static_cast<int>(rng.uniform(0, 500));
        for (int i = 0; i < n; ++i) {
            model.insert(rng.uniform(0, 2000));
        }
        const std::vector<long long> sorted_keys(model.begin(), model.end());
        const VebIndex index(sorted_keys);
        for (int q = 0; q < 300; ++q) {
            const long long key = rng.uniform(-5, 2005);
            INFO("round ", round, " key ", key);
            CHECK_EQ(index.contains(key), model.count(key) == 1);
            auto it = model.upper_bound(key);
            const std::optional<long long> expected =
                it == model.begin() ? std::nullopt : std::optional<long long>(*std::prev(it));
            CHECK_EQ(index.predecessor(key), expected);
        }
    }
}

PERF(3, "a million predecessor queries", 2500) {
    Rng rng;
    const VebIndex index(even_keys(1ULL << 20));
    long long sum = 0;
    for (int q = 0; q < 1'000'000; ++q) {
        sum += index.predecessor(rng.uniform(0, 2'097'150)).value_or(0);
    }
    CHECK_GT(sum, 0);
}
