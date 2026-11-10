#include <algorithm>
#include <cstdlib>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "buggy_table.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::size_t kCapacity = 16;
constexpr std::size_t kOpsPerRun = 20;

bool corrupts_buggy_table(std::span<const Op> ops) {
    BuggyTable table(kCapacity);
    run_ops(table, ops);
    return table.corrupted();
}

const FailureCheck kFails = corrupts_buggy_table;

std::vector<Op> failing_ops() {
    const auto seed = find_failing_seed(0, 1000, kOpsPerRun, kFails);
    REQUIRE(seed.has_value());
    return generate_ops(*seed, kOpsPerRun);
}

std::size_t first_failing_seed_by_hand(std::uint64_t first, std::uint64_t last) {
    for (std::uint64_t seed = first; seed < last; ++seed) {
        if (corrupts_buggy_table(generate_ops(seed, kOpsPerRun))) {
            return static_cast<std::size_t>(seed);
        }
    }
    return static_cast<std::size_t>(last);
}

// Sends every key to the last slot so that any wraparound bug shows up.
struct EndOfTableHash {
    std::uint64_t operator()(std::uint64_t) const { return ~std::uint64_t{0}; }
};

}  // namespace

TEST(1, "the same seed produces the same operations") {
    const auto a = generate_ops(5, 200);
    const auto b = generate_ops(5, 200);
    CHECK_EQ(a, b);
    CHECK_EQ(a.size(), 200);
}

TEST(1, "different seeds produce different operations") {
    std::set<std::uint64_t> first_keys;
    for (std::uint64_t seed = 0; seed < 50; ++seed) {
        first_keys.insert(generate_ops(seed, 8).front().key);
    }
    CHECK_GT(first_keys.size(), 45);
}

TEST(1, "operations have the promised shape") {
    const auto ops = generate_ops(9, 5000);
    int inserts = 0;
    for (const Op& op : ops) {
        CHECK_NE(op.key, 0);
        if (op.kind == Op::Kind::Insert) {
            ++inserts;
            CHECK_NE(op.value, 0);
        }
    }
    CHECK_GT(inserts, 3200);
    CHECK_LT(inserts, 3800);
}

TEST(1, "a find always targets a key inserted earlier") {
    const auto ops = generate_ops(21, 3000);
    std::set<std::uint64_t> seen;
    for (const Op& op : ops) {
        if (op.kind == Op::Kind::Find) {
            CHECK(seen.contains(op.key));
        } else {
            seen.insert(op.key);
        }
    }
}

TEST(1, "results do not depend on addresses or heap state") {
    const auto ops = generate_ops(3, 2000);
    ProbingTable<> first(64 * 1024);
    const auto a = run_ops(first, ops);

    // Perturb the heap before the second run.
    std::vector<std::unique_ptr<char[]>> ballast;
    for (int i = 0; i < 2000; ++i) {
        ballast.push_back(std::make_unique<char[]>(static_cast<std::size_t>(16 + i % 300)));
    }
    ProbingTable<> second(64 * 1024);
    const auto b = run_ops(second, ops);
    CHECK_EQ(a, b);
    CHECK(!a.empty());
}

TEST(1, "run_ops feeds inserts and collects finds in order") {
    struct RecordingTable {
        std::vector<std::uint64_t> stored;
        void insert(std::uint64_t key, std::uint64_t value) {
            stored.push_back(key);
            stored.push_back(value);
        }
        std::optional<std::uint64_t> find(std::uint64_t key) const {
            return key == 7 ? std::optional<std::uint64_t>(70) : std::nullopt;
        }
    };
    RecordingTable table;
    const std::vector<Op> ops{{Op::Kind::Insert, 7, 70},
                              {Op::Kind::Find, 7, 0},
                              {Op::Kind::Find, 8, 0}};
    const auto results = run_ops(table, std::span<const Op>(ops));
    CHECK_EQ(table.stored, std::vector<std::uint64_t>({7, 70}));
    CHECK_EQ(results, std::vector<std::optional<std::uint64_t>>({70, std::nullopt}));
}

TEST(2, "the first failing seed is found") {
    const auto seed = find_failing_seed(0, 1000, kOpsPerRun, kFails);
    REQUIRE(seed.has_value());
    CHECK_EQ(*seed, first_failing_seed_by_hand(0, 1000));
}

TEST(2, "the search honours its range") {
    const auto seed = find_failing_seed(0, 1000, kOpsPerRun, kFails);
    REQUIRE(seed.has_value());
    const auto later = find_failing_seed(*seed + 1, *seed + 1000, kOpsPerRun, kFails);
    REQUIRE(later.has_value());
    CHECK_GT(*later, *seed);
    CHECK_EQ(*later, first_failing_seed_by_hand(*seed + 1, *seed + 1000));
}

TEST(2, "no failing seed in the range returns nullopt") {
    const FailureCheck never = [](std::span<const Op>) { return false; };
    CHECK(!find_failing_seed(0, 50, 10, never).has_value());
}

TEST(2, "a seed and its operations reproduce the failure") {
    const auto seed = find_failing_seed(0, 1000, kOpsPerRun, kFails);
    REQUIRE(seed.has_value());
    const auto ops = generate_ops(*seed, kOpsPerRun);
    CHECK(corrupts_buggy_table(ops));
    CHECK(corrupts_buggy_table(ops));
}

TEST(3, "shrinking keeps the failure and reaches a 1-minimal sequence") {
    const auto ops = failing_ops();
    const auto shrunk = shrink_ops(ops, kFails);
    CHECK(corrupts_buggy_table(shrunk));
    CHECK_LT(shrunk.size(), ops.size());
    for (std::size_t i = 0; i < shrunk.size(); ++i) {
        INFO("without op ", i);
        std::vector<Op> without = shrunk;
        without.erase(without.begin() + static_cast<std::ptrdiff_t>(i));
        REQUIRE(!corrupts_buggy_table(without));
    }
}

TEST(3, "shrinking only removes operations and preserves their order") {
    const auto ops = failing_ops();
    const auto shrunk = shrink_ops(ops, kFails);
    auto it = ops.begin();
    for (const Op& op : shrunk) {
        it = std::find(it, ops.end(), op);
        REQUIRE(it != ops.end());
        ++it;
    }
}

TEST(3, "a sequence that does not fail comes back unchanged") {
    const std::vector<Op> ops{{Op::Kind::Insert, 1, 1}};
    const FailureCheck never = [](std::span<const Op>) { return false; };
    CHECK_EQ(shrink_ops(ops, never), ops);
}

TEST(3, "shrinking handles arbitrary failure predicates") {
    // Fails as long as the keys 5 and 9 both appear.
    const FailureCheck both = [](std::span<const Op> ops) {
        bool five = false, nine = false;
        for (const Op& op : ops) {
            five = five || op.key == 5;
            nine = nine || op.key == 9;
        }
        return five && nine;
    };
    std::vector<Op> ops;
    for (std::uint64_t key = 1; key <= 12; ++key) {
        ops.push_back({Op::Kind::Insert, key, key});
    }
    const auto shrunk = shrink_ops(ops, both);
    REQUIRE_EQ(shrunk.size(), 2);
    CHECK_EQ(shrunk[0].key, 5);
    CHECK_EQ(shrunk[1].key, 9);
}

TEST(4, "inserts wrap past the end of the array") {
    ProbingTable<EndOfTableHash> table(4);
    for (std::uint64_t key = 1; key <= 3; ++key) {
        CHECK(table.insert(key, key * 10));
    }
    for (std::uint64_t key = 1; key <= 3; ++key) {
        CHECK_EQ(table.find(key), std::optional<std::uint64_t>(key * 10));
    }
}

TEST(4, "find also wraps past the end of the array") {
    ProbingTable<EndOfTableHash> table(8);
    for (std::uint64_t key = 1; key <= 6; ++key) {
        REQUIRE(table.insert(key, key));
    }
    CHECK_EQ(table.find(6), std::optional<std::uint64_t>(6));
    CHECK(!table.find(7).has_value());
}

TEST(4, "a full table refuses new keys but accepts overwrites") {
    ProbingTable<EndOfTableHash> table(3);
    CHECK(table.insert(1, 10));
    CHECK(table.insert(2, 20));
    CHECK(table.insert(3, 30));
    CHECK_EQ(table.size(), 3);
    CHECK(!table.insert(4, 40));
    CHECK_EQ(table.size(), 3);
    CHECK(table.insert(2, 200));
    CHECK_EQ(table.find(2), std::optional<std::uint64_t>(200));
    CHECK_EQ(table.size(), 3);
}

TEST(4, "probing past occupied slots does not miss a key") {
    // Keys with the same home collide; a find must walk past them.
    ProbingTable<EndOfTableHash> table(16);
    for (std::uint64_t key = 1; key <= 16; ++key) {
        REQUIRE(table.insert(key, key));
    }
    for (std::uint64_t key = 1; key <= 16; ++key) {
        CHECK_EQ(table.find(key), std::optional<std::uint64_t>(key));
    }
}

TEST(4, "random operations match std::unordered_map") {
    Rng rng;
    ProbingTable<> table(4096);
    std::unordered_map<std::uint64_t, std::uint64_t> model;
    for (int step = 0; step < 40'000; ++step) {
        INFO("step ", step);
        const auto key = static_cast<std::uint64_t>(rng.uniform(1, 3000));
        if (rng.coin(0.6) && model.size() < 3000) {
            const auto value = static_cast<std::uint64_t>(rng.uniform(1, 1'000'000));
            REQUIRE(table.insert(key, value));
            model[key] = value;
        } else {
            const auto found = model.find(key);
            const std::optional<std::uint64_t> want =
                found == model.end() ? std::nullopt : std::optional<std::uint64_t>(found->second);
            REQUIRE_EQ(table.find(key), want);
        }
    }
    CHECK_EQ(table.size(), model.size());
}

TEST(4, "no seed makes the fixed table lose data") {
    for (std::uint64_t seed = 0; seed < 300; ++seed) {
        INFO("seed ", seed);
        ProbingTable<> table(kCapacity * 8);
        std::map<std::uint64_t, std::uint64_t> model;
        for (const Op& op : generate_ops(seed, kOpsPerRun)) {
            if (op.kind == Op::Kind::Insert) {
                REQUIRE(table.insert(op.key, op.value));
                model[op.key] = op.value;
            } else {
                REQUIRE_EQ(table.find(op.key), std::optional<std::uint64_t>(model.at(op.key)));
            }
        }
    }
}
