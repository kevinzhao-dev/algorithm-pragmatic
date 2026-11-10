#include <map>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

constexpr std::uint64_t kAllOnes = ~std::uint64_t{0};

// Depth of the longest root-to-base path when each level halves the universe bits.
int levels(int bits) {
    int count = 1;
    while (bits > 1) {
        bits = bits - bits / 2;
        ++count;
    }
    return count;
}

std::optional<std::uint64_t> model_successor(const std::set<std::uint64_t>& model, std::uint64_t x) {
    const auto it = model.upper_bound(x);
    return it == model.end() ? std::nullopt : std::optional(*it);
}

std::optional<std::uint64_t> model_predecessor(const std::set<std::uint64_t>& model, std::uint64_t x) {
    const auto it = model.lower_bound(x);
    return it == model.begin() ? std::nullopt : std::optional(*std::prev(it));
}

std::uint64_t random_key(Rng& rng, int bits) {
    if (bits == 64) {
        return rng.engine()();
    }
    return static_cast<std::uint64_t>(rng.uniform(0, (std::int64_t{1} << bits) - 1));
}

void check_extremes(const VanEmdeBoas& tree, const std::set<std::uint64_t>& model) {
    REQUIRE_EQ(tree.size(), model.size());
    REQUIRE_EQ(tree.empty(), model.empty());
    if (model.empty()) {
        REQUIRE(!tree.min().has_value());
        REQUIRE(!tree.max().has_value());
    } else {
        REQUIRE_EQ(*tree.min(), *model.begin());
        REQUIRE_EQ(*tree.max(), *model.rbegin());
    }
}

}  // namespace

TEST(1, "a universe size outside 1 to 64 bits is rejected") {
    CHECK_THROWS_AS(VanEmdeBoas(0), std::invalid_argument);
    CHECK_THROWS_AS(VanEmdeBoas(65), std::invalid_argument);
}

TEST(1, "an empty structure has no extremes or successors") {
    const VanEmdeBoas tree(16);
    CHECK(tree.empty());
    CHECK_EQ(tree.size(), 0);
    CHECK(!tree.min().has_value());
    CHECK(!tree.max().has_value());
    CHECK(!tree.contains(5));
    CHECK(!tree.successor(5).has_value());
}

TEST(1, "a key outside the universe is rejected") {
    VanEmdeBoas tree(4);
    CHECK_THROWS_AS(tree.insert(16), std::out_of_range);
    CHECK_THROWS_AS(tree.contains(16), std::out_of_range);
    CHECK_THROWS_AS(tree.successor(16), std::out_of_range);
    CHECK(tree.insert(15));
}

TEST(1, "the smallest universe holds two values") {
    VanEmdeBoas tree(1);
    CHECK(tree.insert(1));
    CHECK(tree.insert(0));
    CHECK(!tree.insert(1));
    CHECK(tree.contains(0));
    CHECK(tree.contains(1));
    CHECK_EQ(tree.size(), 2);
    CHECK_EQ(tree.successor(0), std::optional<std::uint64_t>(1));
    CHECK(!tree.successor(1).has_value());
}

TEST(1, "inserting tracks the minimum and maximum") {
    VanEmdeBoas tree(16);
    tree.insert(100);
    CHECK_EQ(*tree.min(), 100);
    CHECK_EQ(*tree.max(), 100);
    tree.insert(50);
    tree.insert(200);
    CHECK_EQ(*tree.min(), 50);
    CHECK_EQ(*tree.max(), 200);
}

TEST(1, "duplicates are refused") {
    VanEmdeBoas tree(16);
    CHECK(tree.insert(7));
    CHECK(tree.insert(9));
    CHECK(!tree.insert(7));
    CHECK(!tree.insert(9));
    CHECK_EQ(tree.size(), 2);
}

TEST(1, "successor crosses cluster boundaries") {
    VanEmdeBoas tree(16);
    for (std::uint64_t key : {3u, 250u, 257u, 1024u, 65535u}) {
        tree.insert(key);
    }
    CHECK_EQ(*tree.successor(0), 3);
    CHECK_EQ(*tree.successor(3), 250);
    CHECK_EQ(*tree.successor(250), 257);
    CHECK_EQ(*tree.successor(257), 1024);
    CHECK_EQ(*tree.successor(1024), 65535);
    CHECK(!tree.successor(65535).has_value());
}

TEST(1, "a query below the minimum returns the minimum") {
    VanEmdeBoas tree(8);
    tree.insert(200);
    CHECK_EQ(*tree.successor(5), 200);
}

TEST(1, "random operations match std::set") {
    Rng rng;
    for (int bits : {1, 2, 3, 4, 7, 8, 9, 13, 16}) {
        INFO("bits ", bits);
        VanEmdeBoas tree(bits);
        std::set<std::uint64_t> model;
        for (int step = 0; step < 15'000; ++step) {
            INFO("step ", step);
            const std::uint64_t key = random_key(rng, bits);
            REQUIRE_EQ(tree.insert(key), model.insert(key).second);
            const std::uint64_t probe = random_key(rng, bits);
            REQUIRE_EQ(tree.contains(probe), model.contains(probe));
            REQUIRE_EQ(tree.successor(probe), model_successor(model, probe));
            check_extremes(tree, model);
        }
    }
}

TEST(1, "each operation enters O(log log u) nodes") {
    Rng rng;
    for (int bits : {4, 8, 16}) {
        INFO("bits ", bits);
        const int limit = 2 * levels(bits) + 2;
        VanEmdeBoas tree(bits);
        for (int step = 0; step < 15'000; ++step) {
            const std::uint64_t key = random_key(rng, bits);
            const std::uint64_t before = tree.visits();
            tree.insert(key);
            const std::uint64_t after_insert = tree.visits();
            tree.successor(key);
            const std::uint64_t after_successor = tree.visits();
            tree.contains(key);
            const std::uint64_t after_contains = tree.visits();
            CHECK_GT(after_insert, before);
            CHECK_LE(after_insert - before, limit);
            CHECK_LE(after_successor - after_insert, limit);
            CHECK_LE(after_contains - after_successor, limit);
        }
    }
}

TEST(2, "erasing a missing key changes nothing") {
    VanEmdeBoas tree(16);
    CHECK(!tree.erase(5));
    tree.insert(9);
    CHECK(!tree.erase(5));
    CHECK_EQ(tree.size(), 1);
    CHECK(tree.contains(9));
}

TEST(2, "erasing a key outside the universe is rejected") {
    VanEmdeBoas tree(4);
    CHECK_THROWS_AS(tree.erase(16), std::out_of_range);
    CHECK_THROWS_AS(tree.predecessor(16), std::out_of_range);
}

TEST(2, "erasing the only key empties the structure") {
    VanEmdeBoas tree(16);
    tree.insert(9);
    CHECK(tree.erase(9));
    CHECK(tree.empty());
    CHECK(!tree.min().has_value());
    CHECK(!tree.max().has_value());
    CHECK(tree.insert(9));
}

TEST(2, "erasing the minimum promotes the next smallest") {
    VanEmdeBoas tree(16);
    for (std::uint64_t key : {10u, 20u, 300u}) {
        tree.insert(key);
    }
    CHECK(tree.erase(10));
    CHECK_EQ(*tree.min(), 20);
    CHECK(tree.erase(20));
    CHECK_EQ(*tree.min(), 300);
    CHECK_EQ(*tree.max(), 300);
}

TEST(2, "erasing the maximum finds the next largest") {
    VanEmdeBoas tree(16);
    for (std::uint64_t key : {10u, 20u, 300u, 40000u}) {
        tree.insert(key);
    }
    CHECK(tree.erase(40000));
    CHECK_EQ(*tree.max(), 300);
    CHECK(tree.erase(300));
    CHECK_EQ(*tree.max(), 20);
}

TEST(2, "predecessor crosses cluster boundaries") {
    VanEmdeBoas tree(16);
    for (std::uint64_t key : {3u, 250u, 257u, 1024u}) {
        tree.insert(key);
    }
    CHECK_EQ(*tree.predecessor(65535), 1024);
    CHECK_EQ(*tree.predecessor(1024), 257);
    CHECK_EQ(*tree.predecessor(257), 250);
    CHECK_EQ(*tree.predecessor(250), 3);
    CHECK(!tree.predecessor(3).has_value());
}

TEST(2, "predecessor reaches the minimum stored outside the clusters") {
    VanEmdeBoas tree(16);
    tree.insert(1000);
    tree.insert(2000);
    CHECK_EQ(*tree.predecessor(1500), 1000);
    CHECK_EQ(*tree.predecessor(2000), 1000);
}

TEST(2, "random operations match std::set") {
    Rng rng;
    for (int bits : {1, 2, 3, 4, 7, 8, 9, 13, 16, 22}) {
        INFO("bits ", bits);
        VanEmdeBoas tree(bits);
        std::set<std::uint64_t> model;
        for (int step = 0; step < 15'000; ++step) {
            INFO("step ", step);
            const std::uint64_t key = random_key(rng, bits);
            if (rng.coin(0.55)) {
                REQUIRE_EQ(tree.insert(key), model.insert(key).second);
            } else {
                REQUIRE_EQ(tree.erase(key), model.erase(key) == 1);
            }
            const std::uint64_t probe = random_key(rng, bits);
            REQUIRE_EQ(tree.contains(probe), model.contains(probe));
            REQUIRE_EQ(tree.successor(probe), model_successor(model, probe));
            REQUIRE_EQ(tree.predecessor(probe), model_predecessor(model, probe));
            check_extremes(tree, model);
        }
    }
}

TEST(2, "draining a dense universe keeps every query correct") {
    VanEmdeBoas tree(10);
    std::set<std::uint64_t> model;
    for (std::uint64_t key = 0; key < 1024; ++key) {
        tree.insert(key);
        model.insert(key);
    }
    Rng rng;
    auto order = rng.permutation(1024);
    for (int key : order) {
        INFO("erasing ", key);
        REQUIRE(tree.erase(static_cast<std::uint64_t>(key)));
        model.erase(static_cast<std::uint64_t>(key));
        const std::uint64_t probe = random_key(rng, 10);
        REQUIRE_EQ(tree.successor(probe), model_successor(model, probe));
        REQUIRE_EQ(tree.predecessor(probe), model_predecessor(model, probe));
    }
    CHECK(tree.empty());
}

TEST(2, "erase and predecessor enter O(log log u) nodes") {
    Rng rng;
    const int bits = 16;
    const int limit = 2 * levels(bits) + 2;
    VanEmdeBoas tree(bits);
    for (int step = 0; step < 40'000; ++step) {
        tree.insert(random_key(rng, bits));
    }
    for (int step = 0; step < 40'000; ++step) {
        const std::uint64_t key = random_key(rng, bits);
        const std::uint64_t before = tree.visits();
        tree.erase(key);
        const std::uint64_t after_erase = tree.visits();
        tree.predecessor(key);
        const std::uint64_t after_predecessor = tree.visits();
        CHECK_GT(after_erase, before);
        CHECK_LE(after_erase - before, limit);
        CHECK_LE(after_predecessor - after_erase, limit);
    }
}

PERF(2, "a mixed workload over a 24-bit universe", 4000) {
    Rng rng;
    VanEmdeBoas tree(24);
    std::vector<std::uint64_t> keys(300'000);
    for (std::uint64_t& key : keys) {
        key = random_key(rng, 24);
        tree.insert(key);
    }
    std::uint64_t total = 0;
    for (std::uint64_t key : keys) {
        total += tree.successor(key).value_or(0);
        total += tree.predecessor(key).value_or(0);
    }
    for (std::uint64_t key : keys) {
        tree.erase(key);
    }
    CHECK(tree.empty());
    CHECK_GT(total, 0);
}

TEST(3, "the whole 64-bit range is usable") {
    VanEmdeBoas tree(64);
    CHECK(tree.insert(kAllOnes));
    CHECK(tree.insert(0));
    CHECK(tree.insert(kAllOnes - 1));
    CHECK_EQ(*tree.max(), kAllOnes);
    CHECK_EQ(*tree.min(), 0);
    CHECK_EQ(*tree.successor(0), kAllOnes - 1);
    CHECK_EQ(*tree.successor(kAllOnes - 1), kAllOnes);
    CHECK(!tree.successor(kAllOnes).has_value());
    CHECK_EQ(*tree.predecessor(kAllOnes), kAllOnes - 1);
    CHECK(tree.erase(kAllOnes));
    CHECK_EQ(*tree.max(), kAllOnes - 1);
}

TEST(3, "a sparse huge universe uses memory proportional to the keys") {
    Rng rng;
    VanEmdeBoas tree(64);
    const std::size_t n = 20'000;
    std::set<std::uint64_t> model;
    while (model.size() < n) {
        const std::uint64_t key = random_key(rng, 64);
        tree.insert(key);
        model.insert(key);
    }
    CHECK_EQ(tree.size(), n);
    CHECK_LE(tree.node_count(), n * static_cast<std::size_t>(2 * levels(64)));
    CHECK_GE(tree.node_count(), 1);
}

TEST(3, "one key needs only the root") {
    VanEmdeBoas tree(64);
    tree.insert(12345);
    CHECK_EQ(tree.node_count(), 1);
}

TEST(3, "empty clusters are released") {
    Rng rng;
    VanEmdeBoas tree(64);
    std::vector<std::uint64_t> keys(5000);
    for (auto& key : keys) {
        key = random_key(rng, 64);
        tree.insert(key);
    }
    CHECK_GT(tree.node_count(), 1);
    for (std::uint64_t key : keys) {
        tree.erase(key);
    }
    CHECK(tree.empty());
    CHECK_EQ(tree.node_count(), 1);
}

TEST(3, "random operations in a 64-bit universe match std::set") {
    Rng rng;
    VanEmdeBoas tree(64);
    std::set<std::uint64_t> model;
    std::vector<std::uint64_t> pool(800);
    for (auto& key : pool) {
        key = random_key(rng, 64);
    }
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const std::uint64_t key = pool[static_cast<std::size_t>(rng.uniform(0, 799))];
        if (rng.coin(0.55)) {
            REQUIRE_EQ(tree.insert(key), model.insert(key).second);
        } else {
            REQUIRE_EQ(tree.erase(key), model.erase(key) == 1);
        }
        const std::uint64_t probe = pool[static_cast<std::size_t>(rng.uniform(0, 799))];
        REQUIRE_EQ(tree.successor(probe), model_successor(model, probe));
        REQUIRE_EQ(tree.predecessor(probe), model_predecessor(model, probe));
        REQUIRE_EQ(tree.contains(probe), model.contains(probe));
        check_extremes(tree, model);
    }
}

PERF(3, "successor queries over a sparse 64-bit set", 3000) {
    Rng rng;
    VanEmdeBoas tree(64);
    std::vector<std::uint64_t> keys(50'000);
    for (auto& key : keys) {
        key = random_key(rng, 64);
        tree.insert(key);
    }
    std::uint64_t total = 0;
    for (int round = 0; round < 8; ++round) {
        for (std::uint64_t key : keys) {
            total += tree.successor(key).value_or(0);
        }
    }
    CHECK_GT(total, 0);
}

TEST(4, "an empty table routes nothing") {
    const RouteTable table;
    CHECK_EQ(table.size(), 0);
    CHECK(!table.lookup(123).has_value());
}

TEST(4, "lookup finds the range containing an address") {
    RouteTable table;
    CHECK(table.add(100, 199, 1));
    CHECK(table.add(300, 300, 2));
    CHECK_EQ(table.lookup(100), std::optional<int>(1));
    CHECK_EQ(table.lookup(150), std::optional<int>(1));
    CHECK_EQ(table.lookup(199), std::optional<int>(1));
    CHECK(!table.lookup(200).has_value());
    CHECK(!table.lookup(99).has_value());
    CHECK_EQ(table.lookup(300), std::optional<int>(2));
    CHECK_EQ(table.size(), 2);
}

TEST(4, "overlapping ranges are refused") {
    RouteTable table;
    CHECK(table.add(100, 199, 1));
    CHECK(!table.add(150, 160, 2));
    CHECK(!table.add(50, 100, 2));
    CHECK(!table.add(199, 250, 2));
    CHECK(!table.add(0, 4'000'000'000u, 2));
    CHECK(table.add(200, 210, 2));
    CHECK(table.add(50, 99, 3));
    CHECK_EQ(table.size(), 3);
}

TEST(4, "an inverted range is rejected") {
    RouteTable table;
    CHECK_THROWS_AS(table.add(5, 4, 1), std::invalid_argument);
}

TEST(4, "removing a route frees its address space") {
    RouteTable table;
    table.add(100, 199, 1);
    CHECK(!table.remove(150));
    CHECK(table.remove(100));
    CHECK(!table.lookup(150).has_value());
    CHECK_EQ(table.size(), 0);
    CHECK(table.add(150, 160, 9));
}

TEST(4, "addresses at the ends of the IPv4 space") {
    RouteTable table;
    CHECK(table.add(0, 0, 1));
    CHECK(table.add(0xFFFFFFFFu, 0xFFFFFFFFu, 2));
    CHECK_EQ(table.lookup(0), std::optional<int>(1));
    CHECK_EQ(table.lookup(0xFFFFFFFFu), std::optional<int>(2));
    CHECK(!table.lookup(0xFFFFFFFEu).has_value());
}

TEST(4, "random operations match an interval map") {
    Rng rng;
    RouteTable table;
    std::map<std::uint32_t, std::pair<std::uint32_t, int>> model;
    const auto overlaps = [&](std::uint32_t first, std::uint32_t last) {
        for (const auto& [start, route] : model) {
            if (start <= last && first <= route.first) {
                return true;
            }
        }
        return false;
    };
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        const auto first = static_cast<std::uint32_t>(rng.uniform(0, 3000));
        const auto last = first + static_cast<std::uint32_t>(rng.uniform(0, 60));
        if (rng.coin(0.5)) {
            const bool expected = !overlaps(first, last);
            REQUIRE_EQ(table.add(first, last, step), expected);
            if (expected) {
                model[first] = {last, step};
            }
        } else if (!model.empty() && rng.coin()) {
            auto it = model.lower_bound(first);
            if (it == model.end()) {
                it = model.begin();
            }
            REQUIRE(table.remove(it->first));
            model.erase(it);
        } else {
            REQUIRE_EQ(table.remove(first), model.erase(first) == 1);
        }
        REQUIRE_EQ(table.size(), model.size());
        const auto ip = static_cast<std::uint32_t>(rng.uniform(0, 3100));
        auto it = model.upper_bound(ip);
        std::optional<int> expected;
        if (it != model.begin() && std::prev(it)->second.first >= ip) {
            expected = std::prev(it)->second.second;
        }
        REQUIRE_EQ(table.lookup(ip), expected);
    }
}

PERF(4, "a large routing table", 3000) {
    Rng rng;
    RouteTable table;
    std::uint32_t next = 1000;
    for (int i = 0; i < 100'000; ++i) {
        const auto width = static_cast<std::uint32_t>(rng.uniform(0, 500));
        table.add(next, next + width, i);
        next += width + static_cast<std::uint32_t>(rng.uniform(1, 500));
    }
    std::int64_t hits = 0;
    for (int i = 0; i < 1'000'000; ++i) {
        hits += table.lookup(static_cast<std::uint32_t>(rng.uniform(0, next))).has_value();
    }
    CHECK_GT(hits, 0);
}
