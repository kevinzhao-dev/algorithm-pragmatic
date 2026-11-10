#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;
using Items = std::vector<std::pair<std::uint64_t, int>>;

namespace {

Items random_items(Rng& rng, std::size_t n, std::uint64_t limit) {
    std::set<std::uint64_t> keys;
    while (keys.size() < n) {
        keys.insert(static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(limit) - 1)));
    }
    Items items;
    int value = 0;
    for (std::uint64_t key : keys) {
        items.emplace_back(key, value++);
    }
    rng.shuffle(items);
    return items;
}

void check_table(const PerfectHashMap<int>& table, const Items& items, Rng& rng) {
    CHECK_EQ(table.size(), items.size());
    std::unordered_map<std::uint64_t, int> expected(items.begin(), items.end());
    for (const auto& [key, value] : items) {
        const int* found = table.find(key);
        REQUIRE(found != nullptr);
        CHECK_EQ(*found, value);
        CHECK(table.contains(key));
    }
    for (int trial = 0; trial < 2000; ++trial) {
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(kHashPrime) - 1));
        CHECK_EQ(table.find(key) != nullptr, expected.contains(key));
    }
}

}  // namespace

TEST(1, "an empty set of keys builds an empty table") {
    const PerfectHashMap<int> table(Items{});
    CHECK_EQ(table.size(), 0);
    CHECK_EQ(table.total_slots(), 0);
}

TEST(1, "duplicate keys are rejected instead of looping forever") {
    CHECK_THROWS_AS(PerfectHashMap<int>(Items{{1, 10}, {2, 20}, {1, 30}}), std::invalid_argument);
}

TEST(1, "a key at or above the prime is rejected") {
    CHECK_THROWS_AS(PerfectHashMap<int>(Items{{kHashPrime, 1}}), std::invalid_argument);
}

TEST(1, "the second level is linear in the number of keys") {
    Rng rng;
    for (std::size_t n : {1u, 2u, 10u, 1000u, 100'000u}) {
        INFO("n ", n);
        const PerfectHashMap<int> table(random_items(rng, n, 1'000'000'000'000), 77);
        CHECK_EQ(table.size(), n);
        CHECK_GE(table.total_slots(), n);
        CHECK_LE(table.total_slots(), 4 * n);
    }
}

TEST(1, "keys that collide modulo small numbers still get a linear table") {
    Items items;
    for (std::uint64_t i = 0; i < 50'000; ++i) {
        items.emplace_back(i * 1'000'000'007ULL, static_cast<int>(i));
    }
    const PerfectHashMap<int> table(std::move(items), 11);
    CHECK_LE(table.total_slots(), 4ULL * 50'000);
}

TEST(1, "values are moved out of the input without leaks") {
    const auto before = Tracked::live();
    {
        std::vector<std::pair<std::uint64_t, Tracked>> items;
        for (int i = 0; i < 2000; ++i) {
            items.emplace_back(static_cast<std::uint64_t>(i) * 7, Tracked(i));
        }
        const PerfectHashMap<Tracked> table(std::move(items), 3);
        CHECK_EQ(table.size(), 2000);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "every key is found with its own value") {
    Rng rng;
    const Items items = random_items(rng, 20'000, 1ULL << 40);
    const PerfectHashMap<int> table(items, 5);
    check_table(table, items, rng);
}

TEST(2, "an empty table finds nothing") {
    const PerfectHashMap<int> table(Items{});
    CHECK(table.find(0) == nullptr);
    CHECK(!table.contains(0));
}

TEST(2, "keys outside the build set report absent without crashing") {
    Rng rng;
    const Items items = random_items(rng, 100, 1'000);
    const PerfectHashMap<int> table(items, 9);
    for (std::uint64_t key = 0; key < 1'000; ++key) {
        const bool present = std::ranges::find_if(items, [&](const auto& item) { return item.first == key; }) != items.end();
        CHECK_EQ(table.contains(key), present);
    }
    CHECK(!table.contains(kHashPrime));
    CHECK(!table.contains(~std::uint64_t{0}));
}

TEST(2, "many different seeds all build correct tables") {
    Rng rng;
    for (std::uint64_t seed = 0; seed < 300; ++seed) {
        INFO("seed ", seed);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 40));
        const Items items = random_items(rng, n, 1'000);
        const PerfectHashMap<int> table(items, seed);
        for (const auto& [key, value] : items) {
            REQUIRE(table.contains(key));
            REQUIRE_EQ(*table.find(key), value);
        }
        CHECK_LE(table.total_slots(), 4 * n);
    }
}

TEST(2, "sequential keys are handled") {
    Items items;
    for (std::uint64_t i = 0; i < 40'000; ++i) {
        items.emplace_back(i, static_cast<int>(i * 3));
    }
    Rng rng;
    const PerfectHashMap<int> table(items, 2);
    check_table(table, items, rng);
}

TEST(2, "a key near the prime is usable") {
    const PerfectHashMap<int> table(Items{{kHashPrime - 1, 42}, {0, 1}});
    CHECK_EQ(*table.find(kHashPrime - 1), 42);
    CHECK_EQ(*table.find(0), 1);
}

TEST(2, "each lookup examines at most two slots") {
    Rng rng;
    const Items items = random_items(rng, 30'000, 1ULL << 50);
    const PerfectHashMap<int> table(items, 13);
    for (const auto& [key, value] : items) {
        const std::uint64_t before = table.probes();
        table.find(key);
        CHECK_GE(table.probes() - before, 1);
        CHECK_LE(table.probes() - before, 2);
    }
    for (int trial = 0; trial < 30'000; ++trial) {
        const std::uint64_t before = table.probes();
        table.find(static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(kHashPrime) - 1)));
        CHECK_LE(table.probes() - before, 2);
    }
}

PERF(2, "building and querying a million keys", 4500) {
    Rng rng;
    const Items items = random_items(rng, 1'000'000, 1ULL << 55);
    const PerfectHashMap<int> table(items, 19);
    std::int64_t sum = 0;
    for (const auto& [key, value] : items) {
        sum += *table.find(key);
    }
    CHECK_GT(sum, 0);
}

TEST(3, "an empty map finds nothing") {
    const StaticStringMap<int> map(std::vector<std::pair<std::string, int>>{});
    CHECK_EQ(map.size(), 0);
    CHECK(map.find("anything") == nullptr);
}

TEST(3, "duplicate strings are rejected") {
    using Item = std::pair<std::string, int>;
    CHECK_THROWS_AS(StaticStringMap<int>(std::vector<Item>{{"a", 1}, {"a", 2}}), std::invalid_argument);
}

TEST(3, "known keys map to their values") {
    using Item = std::pair<std::string, int>;
    const StaticStringMap<int> map(std::vector<Item>{{"alpha", 1}, {"beta", 2}, {"gamma", 3}});
    CHECK_EQ(map.size(), 3);
    CHECK_EQ(*map.find("alpha"), 1);
    CHECK_EQ(*map.find("gamma"), 3);
    CHECK(map.find("delta") == nullptr);
}

TEST(3, "the empty string is a valid key") {
    using Item = std::pair<std::string, int>;
    const StaticStringMap<int> map(std::vector<Item>{{"", 7}, {"x", 8}});
    CHECK_EQ(*map.find(""), 7);
    CHECK_EQ(*map.find("x"), 8);
}

TEST(3, "strings that differ only in length are distinguished") {
    using Item = std::pair<std::string, int>;
    const StaticStringMap<int> map(std::vector<Item>{{std::string(1, '\0'), 1}, {std::string(2, '\0'), 2}});
    CHECK_EQ(*map.find(std::string(1, '\0')), 1);
    CHECK_EQ(*map.find(std::string(2, '\0')), 2);
    CHECK(map.find(std::string(3, '\0')) == nullptr);
    CHECK(map.find("") == nullptr);
}

TEST(3, "a large dictionary finds members and rejects strangers") {
    using Item = std::pair<std::string, int>;
    Rng rng;
    std::set<std::string> names;
    while (names.size() < 30'000) {
        names.insert(rng.text(static_cast<std::size_t>(rng.uniform(1, 24)), "abcdef0123"));
    }
    std::vector<Item> items;
    int index = 0;
    for (const std::string& name : names) {
        items.emplace_back(name, index++);
    }
    const auto original = items;
    const StaticStringMap<int> map(std::move(items), 1234);
    CHECK_EQ(map.size(), original.size());
    for (const auto& [name, value] : original) {
        const int* found = map.find(name);
        REQUIRE(found != nullptr);
        CHECK_EQ(*found, value);
    }
    for (int trial = 0; trial < 30'000; ++trial) {
        const std::string probe = rng.text(static_cast<std::size_t>(rng.uniform(1, 24)), "abcdef0123");
        CHECK_EQ(map.find(probe) != nullptr, names.contains(probe));
    }
}

PERF(3, "a million lookups over two hundred thousand strings", 3000) {
    using Item = std::pair<std::string, int>;
    Rng rng;
    std::set<std::string> names;
    while (names.size() < 200'000) {
        names.insert(rng.text(16, "abcdefghijklmnop"));
    }
    std::vector<Item> items;
    int index = 0;
    for (const std::string& name : names) {
        items.emplace_back(name, index++);
    }
    const auto queries = items;
    const StaticStringMap<int> map(std::move(items), 99);
    std::int64_t sum = 0;
    for (int round = 0; round < 5; ++round) {
        for (const auto& [name, value] : queries) {
            sum += *map.find(name);
        }
    }
    CHECK_GT(sum, 0);
}
