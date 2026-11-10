#include <cstdint>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

namespace {

constexpr std::uint64_t kP = (std::uint64_t{1} << 61) - 1;

std::uint64_t reference_hash(std::uint64_t a, std::uint64_t b, std::uint64_t k, std::uint64_t m) {
    const unsigned __int128 value = static_cast<unsigned __int128>(a) * k + b;
    return static_cast<std::uint64_t>(value % kP) % m;
}

struct IdentityHash {
    std::size_t operator()(std::uint64_t key) const { return static_cast<std::size_t>(key); }
};

struct ConstantHash {
    std::size_t operator()(std::uint64_t) const { return 42; }
};

struct CountingHash {
    static inline std::uint64_t calls = 0;
    std::size_t operator()(std::uint64_t key) const {
        ++calls;
        return static_cast<std::size_t>(key);
    }
};

using Map = ChainedHashMap<std::uint64_t, std::uint64_t>;

}  // namespace

TEST(1, "hash matches the formula for small values") {
    const UniversalHash hash(3, 5);
    CHECK_EQ(hash(7, 100), std::size_t{26});
    CHECK_EQ(hash(0, 7), std::size_t{5});
}

TEST(1, "result is reduced modulo the bucket count") {
    const UniversalHash hash(1, 0);
    CHECK_EQ(hash(10, 7), std::size_t{3});
    CHECK_EQ(hash(10, 1), std::size_t{0});
}

TEST(1, "huge a, b and key do not overflow") {
    const UniversalHash hash(kP - 1, kP - 1);
    CHECK_EQ(hash(kP - 1, 1000), reference_hash(kP - 1, kP - 1, kP - 1, 1000));
    CHECK_EQ(hash(~std::uint64_t{0}, 1'000'000'007),
             reference_hash(kP - 1, kP - 1, ~std::uint64_t{0}, 1'000'000'007));
}

TEST(1, "constructor rejects parameters outside the family") {
    CHECK_THROWS_AS(UniversalHash(0, 1), std::invalid_argument);
    CHECK_THROWS_AS(UniversalHash(kP, 1), std::invalid_argument);
    CHECK_THROWS_AS(UniversalHash(1, kP), std::invalid_argument);
}

TEST(1, "agrees with the formula on random inputs") {
    Rng rng;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto a = static_cast<std::uint64_t>(rng.uniform(1, static_cast<std::int64_t>(kP - 1)));
        const auto b = static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(kP - 1)));
        const auto key = rng.engine()();
        const auto m = static_cast<std::uint64_t>(rng.uniform(1, 1'000'000));
        const UniversalHash hash(a, b);
        REQUIRE_EQ(hash(key, m), reference_hash(a, b, key, m));
    }
}

TEST(1, "two keys collide about once in m random hash functions") {
    Rng rng;
    const std::uint64_t k1 = 12345, k2 = 67890;
    const std::size_t m = 97;
    int collisions = 0;
    const int trials = 50'000;
    for (int i = 0; i < trials; ++i) {
        const UniversalHash hash(
            static_cast<std::uint64_t>(rng.uniform(1, static_cast<std::int64_t>(kP - 1))),
            static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(kP - 1))));
        collisions += hash(k1, m) == hash(k2, m) ? 1 : 0;
    }
    const double rate = static_cast<double>(collisions) / trials;
    CHECK_LT(rate, 2.0 / static_cast<double>(m));
    CHECK_GT(rate, 0.25 / static_cast<double>(m));
}

TEST(2, "a new map is empty") {
    Map map;
    CHECK(map.empty());
    CHECK_EQ(map.size(), 0);
    CHECK(map.find(1) == nullptr);
    CHECK(!map.erase(1).has_value());
}

TEST(2, "inserted keys are found with their values") {
    Map map;
    CHECK(map.insert(1, 10));
    CHECK(map.insert(2, 20));
    REQUIRE(map.find(1) != nullptr);
    CHECK_EQ(*map.find(1), std::uint64_t{10});
    CHECK_EQ(*map.find(2), std::uint64_t{20});
    CHECK(map.find(3) == nullptr);
    CHECK_EQ(map.size(), 2);
}

TEST(2, "inserting an existing key overwrites and reports false") {
    Map map;
    CHECK(map.insert(5, 1));
    CHECK(!map.insert(5, 2));
    CHECK_EQ(*map.find(5), std::uint64_t{2});
    CHECK_EQ(map.size(), 1);
}

TEST(2, "erase returns the value and removes the key") {
    Map map;
    map.insert(5, 50);
    map.insert(6, 60);
    CHECK_EQ(map.erase(5), std::optional<std::uint64_t>(50));
    CHECK(map.find(5) == nullptr);
    CHECK(!map.erase(5).has_value());
    CHECK_EQ(map.size(), 1);
    CHECK_EQ(*map.find(6), std::uint64_t{60});
}

TEST(2, "find works through a const reference and can mutate through the non-const one") {
    Map map;
    map.insert(1, 1);
    *map.find(1) = 2;
    const Map& view = map;
    REQUIRE(view.find(1) != nullptr);
    CHECK_EQ(*view.find(1), std::uint64_t{2});
}

TEST(2, "string keys work with the default prehash") {
    ChainedHashMap<std::string, int> map;
    CHECK(map.insert("alpha", 1));
    CHECK(map.insert("beta", 2));
    CHECK(!map.insert("alpha", 3));
    CHECK_EQ(*map.find("alpha"), 3);
    CHECK_EQ(map.erase("beta"), std::optional<int>(2));
    CHECK(map.find("beta") == nullptr);
}

TEST(2, "every entry is released") {
    const auto before = Tracked::live();
    {
        ChainedHashMap<int, Tracked> map;
        for (int i = 0; i < 2000; ++i) {
            map.insert(i, Tracked(i));
        }
        for (int i = 0; i < 1000; ++i) {
            map.erase(i);
        }
        CHECK_EQ(map.size(), 1000);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(2, "random operations match std::unordered_map") {
    Rng rng;
    Map map;
    std::unordered_map<std::uint64_t, std::uint64_t> model;
    for (int step = 0; step < 100'000; ++step) {
        INFO("step ", step);
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, 500));
        const auto op = rng.uniform(0, 2);
        if (op == 0) {
            const auto value = static_cast<std::uint64_t>(rng.uniform(0, 1'000'000));
            const bool fresh = model.insert_or_assign(key, value).second;
            REQUIRE_EQ(map.insert(key, value), fresh);
        } else if (op == 1) {
            const auto it = model.find(key);
            const auto got = map.erase(key);
            if (it == model.end()) {
                REQUIRE(!got.has_value());
            } else {
                REQUIRE_EQ(got, std::optional<std::uint64_t>(it->second));
                model.erase(it);
            }
        } else {
            const auto it = model.find(key);
            const auto* got = map.find(key);
            REQUIRE_EQ(got != nullptr, it != model.end());
            if (got != nullptr) {
                REQUIRE_EQ(*got, it->second);
            }
        }
        REQUIRE_EQ(map.size(), model.size());
    }
}

TEST(3, "the table grows as keys arrive and stays near linear in size") {
    Map map;
    for (std::uint64_t key = 0; key < 50'000; ++key) {
        map.insert(key, key);
        REQUIRE(map.size() <= map.bucket_count());
        REQUIRE(map.bucket_count() <= 8 * map.size() + 64);
    }
    for (std::uint64_t key = 0; key < 50'000; ++key) {
        REQUIRE(map.find(key) != nullptr);
    }
}

TEST(3, "the table shrinks again after mass deletion") {
    Map map;
    for (std::uint64_t key = 0; key < 50'000; ++key) {
        map.insert(key, key);
    }
    for (std::uint64_t key = 0; key < 49'990; ++key) {
        CHECK(map.erase(key).has_value());
        REQUIRE(map.bucket_count() <= 8 * map.size() + 64);
    }
    CHECK_EQ(map.size(), 10);
    for (std::uint64_t key = 49'990; key < 50'000; ++key) {
        CHECK_EQ(*map.find(key), key);
    }
}

TEST(3, "load factor is size over bucket count") {
    Map map;
    for (std::uint64_t key = 0; key < 100; ++key) {
        map.insert(key, key);
    }
    CHECK_NEAR(map.load_factor(), static_cast<double>(map.size()) / static_cast<double>(map.bucket_count()), 1e-12);
}

TEST(3, "alternating insert and erase at the resize boundary does not rehash every time") {
    CountingHash::calls = 0;
    ChainedHashMap<std::uint64_t, std::uint64_t, CountingHash> map;
    const std::uint64_t n = 4096;
    for (std::uint64_t key = 0; key < n; ++key) {
        map.insert(key, key);
    }
    CountingHash::calls = 0;
    // A one-size-fits-all threshold would rehash ~n keys on every step.
    for (int step = 0; step < 2000; ++step) {
        map.insert(n, n);
        map.erase(n);
    }
    CHECK_LE(CountingHash::calls, 10 * 2000 + 4 * n);
}

PERF(3, "a million keys inserted, found and erased", 2500) {
    Map map;
    Rng rng;
    std::vector<std::uint64_t> keys(1'000'000);
    for (auto& key : keys) {
        key = rng.engine()();
    }
    for (std::uint64_t key : keys) {
        map.insert(key, key);
    }
    for (std::uint64_t key : keys) {
        REQUIRE(map.find(key) != nullptr);
    }
    for (std::uint64_t key : keys) {
        map.erase(key);
    }
    CHECK(map.empty());
}

TEST(4, "bucket_of stays below the bucket count") {
    Map map;
    Rng rng;
    for (int i = 0; i < 2000; ++i) {
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, 1'000'000));
        map.insert(key, key);
        REQUIRE(map.bucket_of(key) < map.bucket_count());
    }
}

TEST(4, "different seeds lay the keys out differently") {
    ChainedHashMap<std::uint64_t, std::uint64_t, IdentityHash> first(1), second(2);
    int differences = 0;
    for (std::uint64_t key = 0; key < 1000; ++key) {
        first.insert(key, key);
        second.insert(key, key);
    }
    REQUIRE_EQ(first.bucket_count(), second.bucket_count());
    for (std::uint64_t key = 0; key < 1000; ++key) {
        differences += first.bucket_of(key) != second.bucket_of(key) ? 1 : 0;
    }
    CHECK_GT(differences, 100);
}

TEST(4, "keys that differ only in high bits do not pile up") {
    ChainedHashMap<std::uint64_t, std::uint64_t, IdentityHash> map;
    for (std::uint64_t i = 0; i < 20'000; ++i) {
        map.insert(i << 20, i);
    }
    CHECK_LE(map.max_chain_length(), 32);
    for (std::uint64_t i = 0; i < 20'000; ++i) {
        REQUIRE(map.find(i << 20) != nullptr);
    }
}

TEST(4, "a prehash that collides on everything is slow but correct") {
    ChainedHashMap<std::uint64_t, std::uint64_t, ConstantHash> map;
    for (std::uint64_t key = 0; key < 2000; ++key) {
        CHECK(map.insert(key, key * 2));
    }
    CHECK_EQ(map.max_chain_length(), 2000);
    for (std::uint64_t key = 0; key < 2000; key += 37) {
        CHECK_EQ(*map.find(key), key * 2);
    }
    for (std::uint64_t key = 0; key < 2000; ++key) {
        CHECK(map.erase(key).has_value());
    }
    CHECK(map.empty());
}

TEST(4, "move construction and assignment transfer ownership") {
    const auto before = Tracked::live();
    {
        ChainedHashMap<int, Tracked> a;
        for (int i = 0; i < 100; ++i) {
            a.insert(i, Tracked(i));
        }
        ChainedHashMap<int, Tracked> b(std::move(a));
        CHECK_EQ(b.size(), 100);
        CHECK(a.empty());  // NOLINT: moved-from state is part of this class's contract
        CHECK(a.insert(7, Tracked(7)));  // NOLINT
        CHECK_EQ(a.find(7)->value(), 7);  // NOLINT

        ChainedHashMap<int, Tracked> c;
        c.insert(1, Tracked(1));
        c = std::move(b);
        CHECK_EQ(c.size(), 100);
        CHECK_EQ(c.find(42)->value(), 42);
        CHECK(b.empty());  // NOLINT
    }
    CHECK_EQ(Tracked::live(), before);
}
