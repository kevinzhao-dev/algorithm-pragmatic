#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Recursion depth when each level keeps whichever of the two parts is larger.
template <typename Split>
int levels(int bits) {
    if (bits <= 1) {
        return 1;
    }
    const int high = Split::high_bits(bits);
    return 1 + std::max(levels<Split>(high), levels<Split>(bits - high));
}

std::uint64_t random_key(Rng& rng, int bits) {
    if (bits == 64) {
        return rng.engine()();
    }
    return static_cast<std::uint64_t>(rng.uniform(0, (std::int64_t{1} << bits) - 1));
}

// Randomized differential test against std::set plus a per-operation node-entry bound.
template <typename Tree, typename Split>
void compare_to_model(int bits, int steps, Rng& rng) {
    INFO("bits ", bits);
    const auto limit = static_cast<std::uint64_t>(2 * levels<Split>(bits) + 2);
    Tree tree(bits);
    std::set<std::uint64_t> model;
    for (int step = 0; step < steps; ++step) {
        INFO("step ", step);
        const std::uint64_t key = random_key(rng, bits);
        const std::uint64_t before_update = tree.visits();
        if (rng.coin(0.55)) {
            REQUIRE_EQ(tree.insert(key), model.insert(key).second);
        } else {
            REQUIRE_EQ(tree.erase(key), model.erase(key) == 1);
        }
        CHECK_LE(tree.visits() - before_update, limit);
        const std::uint64_t probe = random_key(rng, bits);
        const std::uint64_t before_query = tree.visits();
        REQUIRE_EQ(tree.contains(probe), model.contains(probe));
        const auto above = model.upper_bound(probe);
        REQUIRE_EQ(tree.successor(probe), above == model.end() ? std::nullopt : std::optional(*above));
        const auto below = model.lower_bound(probe);
        REQUIRE_EQ(tree.predecessor(probe), below == model.begin() ? std::nullopt : std::optional(*std::prev(below)));
        CHECK_LE(tree.visits() - before_query, 3 * limit);
        REQUIRE_EQ(tree.size(), model.size());
        if (model.empty()) {
            REQUIRE(!tree.min().has_value());
            REQUIRE(!tree.max().has_value());
        } else {
            REQUIRE_EQ(*tree.min(), *model.begin());
            REQUIRE_EQ(*tree.max(), *model.rbegin());
        }
    }
}

template <typename Tree>
std::uint64_t deepest_successor(int bits, std::uint64_t count) {
    Tree tree(bits);
    for (std::uint64_t key = 0; key < count; ++key) {
        tree.insert(key);
    }
    std::uint64_t deepest = 0;
    for (std::uint64_t key = 0; key + 1 < count; ++key) {
        const std::uint64_t before = tree.visits();
        tree.successor(key);
        deepest = std::max(deepest, tree.visits() - before);
    }
    return deepest;
}

template <int High>
struct FixedHighBits {
    static constexpr int high_bits(int bits) { return std::min(High, bits - 1); }
};

using CubeRootKeepsMax = Veb<CubeRootSplit, false>;
using SqrtExcludesMax = Veb<SqrtSplit, true>;
using CubeRootExcludesMax = Veb<CubeRootSplit, true>;

}  // namespace

TEST(1, "a universe size outside 1 to 64 bits is rejected") {
    CHECK_THROWS_AS(CubeRootKeepsMax(0), std::invalid_argument);
    CHECK_THROWS_AS(CubeRootKeepsMax(65), std::invalid_argument);
}

TEST(1, "keys outside the universe are rejected") {
    CubeRootKeepsMax tree(4);
    CHECK_THROWS_AS(tree.insert(16), std::out_of_range);
    CHECK_THROWS_AS(tree.erase(16), std::out_of_range);
    CHECK_THROWS_AS(tree.contains(16), std::out_of_range);
    CHECK_THROWS_AS(tree.successor(16), std::out_of_range);
    CHECK_THROWS_AS(tree.predecessor(16), std::out_of_range);
}

TEST(1, "a two-bit universe has no empty high part") {
    CubeRootKeepsMax tree(2);
    for (std::uint64_t key : {0u, 1u, 2u, 3u}) {
        CHECK(tree.insert(key));
    }
    CHECK_EQ(tree.size(), 4);
    CHECK_EQ(*tree.successor(1), 2);
}

TEST(1, "cube-root splitting matches std::set") {
    Rng rng;
    for (int bits : {1, 2, 3, 4, 5, 8, 12, 17, 24}) {
        compare_to_model<CubeRootKeepsMax, CubeRootSplit>(bits, 12'000, rng);
    }
}

TEST(1, "cube-root splitting works across the 64-bit universe") {
    Rng rng;
    compare_to_model<CubeRootKeepsMax, CubeRootSplit>(64, 8'000, rng);
}

TEST(1, "the recursion really follows the larger two-thirds part") {
    const std::uint64_t deepest = deepest_successor<CubeRootKeepsMax>(48, 5000);
    CHECK_GT(deepest, static_cast<std::uint64_t>(levels<SqrtSplit>(48)));
}

TEST(1, "a single remaining key uses only the root") {
    CubeRootKeepsMax tree(40);
    tree.insert(5);
    CHECK_EQ(tree.node_count(), 1);
}

TEST(1, "two keys already need a cluster when only the minimum is excluded") {
    CubeRootKeepsMax tree(40);
    tree.insert(5);
    tree.insert(1'000'000);
    CHECK_EQ(tree.node_count(), 3);
}

TEST(2, "duplicates and missing keys are refused") {
    SqrtExcludesMax tree(16);
    CHECK(tree.insert(5));
    CHECK(tree.insert(9));
    CHECK(tree.insert(100));
    CHECK(!tree.insert(5));
    CHECK(!tree.insert(9));
    CHECK(!tree.insert(100));
    CHECK(!tree.erase(6));
    CHECK_EQ(tree.size(), 3);
}

TEST(2, "two keys live entirely in the root") {
    SqrtExcludesMax tree(40);
    tree.insert(5);
    tree.insert(1'000'000);
    CHECK_EQ(tree.node_count(), 1);
}

TEST(2, "the third key is the first to reach a cluster") {
    SqrtExcludesMax tree(40);
    tree.insert(5);
    tree.insert(1'000'000);
    tree.insert(777);
    CHECK_EQ(tree.node_count(), 3);
    CHECK_EQ(*tree.min(), 5);
    CHECK_EQ(*tree.max(), 1'000'000);
}

TEST(2, "a new extreme pushes the old extreme into a cluster") {
    SqrtExcludesMax tree(16);
    tree.insert(100);
    tree.insert(200);
    tree.insert(50);
    tree.insert(300);
    CHECK_EQ(*tree.min(), 50);
    CHECK_EQ(*tree.max(), 300);
    CHECK_EQ(*tree.successor(50), 100);
    CHECK_EQ(*tree.successor(200), 300);
    CHECK_EQ(*tree.predecessor(100), 50);
    CHECK_EQ(*tree.predecessor(300), 200);
}

TEST(2, "erasing an extreme promotes a cluster element") {
    SqrtExcludesMax tree(16);
    for (std::uint64_t key : {10u, 500u, 501u, 40000u}) {
        tree.insert(key);
    }
    CHECK(tree.erase(40000));
    CHECK_EQ(*tree.max(), 501);
    CHECK(tree.erase(10));
    CHECK_EQ(*tree.min(), 500);
    CHECK_EQ(tree.node_count(), 1);
}

TEST(2, "successor finds the maximum that no cluster contains") {
    SqrtExcludesMax tree(16);
    for (std::uint64_t key : {1u, 2u, 3u, 60000u}) {
        tree.insert(key);
    }
    CHECK_EQ(*tree.successor(3), 60000);
    CHECK_EQ(*tree.successor(4), 60000);
    CHECK_EQ(*tree.predecessor(60000), 3);
}

TEST(2, "excluding the maximum matches std::set") {
    Rng rng;
    for (int bits : {1, 2, 3, 4, 7, 8, 13, 16, 22}) {
        compare_to_model<SqrtExcludesMax, SqrtSplit>(bits, 12'000, rng);
    }
}

TEST(2, "excluding the maximum works across the 64-bit universe") {
    Rng rng;
    compare_to_model<SqrtExcludesMax, SqrtSplit>(64, 8'000, rng);
}

TEST(2, "emptying the structure releases every cluster") {
    Rng rng;
    SqrtExcludesMax tree(64);
    std::vector<std::uint64_t> keys(4000);
    for (auto& key : keys) {
        key = random_key(rng, 64);
        tree.insert(key);
    }
    for (std::uint64_t key : keys) {
        tree.erase(key);
    }
    CHECK(tree.empty());
    CHECK_EQ(tree.node_count(), 1);
}

TEST(3, "both variations combined match std::set") {
    Rng rng;
    for (int bits : {1, 2, 3, 6, 11, 20}) {
        compare_to_model<CubeRootExcludesMax, CubeRootSplit>(bits, 12'000, rng);
    }
    compare_to_model<CubeRootExcludesMax, CubeRootSplit>(64, 8'000, rng);
}

TEST(3, "two keys live entirely in the root under both variations") {
    CubeRootExcludesMax tree(40);
    tree.insert(5);
    tree.insert(1'000'000);
    CHECK_EQ(tree.node_count(), 1);
}

TEST(3, "the structure works with an arbitrary split policy") {
    Rng rng;
    using FourHighBits = FixedHighBits<4>;
    compare_to_model<Veb<FourHighBits, false>, FourHighBits>(18, 8'000, rng);
    compare_to_model<Veb<FourHighBits, true>, FourHighBits>(18, 8'000, rng);
}

PERF(3, "random operations under every policy", 3500) {
    Rng rng;
    std::uint64_t checksum = 0;
    const auto run = [&]<typename Tree>(Tree tree) {
        std::vector<std::uint64_t> keys(50'000);
        for (auto& key : keys) {
            key = random_key(rng, 40);
            tree.insert(key);
        }
        for (std::uint64_t key : keys) {
            checksum += tree.successor(key).value_or(1);
            checksum += tree.predecessor(key).value_or(1);
        }
        for (std::uint64_t key : keys) {
            tree.erase(key);
        }
    };
    run(Veb<SqrtSplit, false>(40));
    run(CubeRootKeepsMax(40));
    run(SqrtExcludesMax(40));
    run(CubeRootExcludesMax(40));
    CHECK_GT(checksum, 0);
}
