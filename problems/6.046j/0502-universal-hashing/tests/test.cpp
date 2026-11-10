#include <cmath>
#include <map>
#include <unordered_map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Tracked;

namespace {

// Measures how often one pair of keys collides across independently drawn functions.
template <typename MakeHash>
int collisions_for(std::uint64_t x, std::uint64_t y, int draws, MakeHash make_hash, std::mt19937_64& rng) {
    int hits = 0;
    for (int draw = 0; draw < draws; ++draw) {
        const auto hash = make_hash(rng);
        hits += hash(x) == hash(y);
    }
    return hits;
}

}  // namespace

TEST(1, "a non-prime table size is rejected") {
    Rng rng;
    CHECK_THROWS_AS(DotProductHash(0, rng.engine()), std::invalid_argument);
    CHECK_THROWS_AS(DotProductHash(1, rng.engine()), std::invalid_argument);
    CHECK_THROWS_AS(DotProductHash(15, rng.engine()), std::invalid_argument);
    CHECK_THROWS_AS(DotProductHash(2147483648ULL, rng.engine()), std::invalid_argument);
}

TEST(1, "hash values fall inside the table") {
    Rng rng;
    for (std::uint64_t slots : {2ULL, 101ULL, 65537ULL, 2147483647ULL}) {
        const DotProductHash hash(slots, rng.engine());
        CHECK_EQ(hash.slots(), slots);
        for (int i = 0; i < 2000; ++i) {
            CHECK_LT(hash(rng.engine()()), slots);
        }
    }
}

TEST(1, "the same function gives the same value") {
    Rng rng;
    const DotProductHash hash(1009, rng.engine());
    const DotProductHash copy = hash;
    for (std::uint64_t key : {0ULL, 1ULL, 12345ULL, ~0ULL}) {
        CHECK_EQ(hash(key), copy(key));
    }
}

TEST(1, "the full 64-bit key range is used") {
    Rng rng;
    // If the top bits were ignored, these would hash identically under every function.
    const std::uint64_t low = 12345;
    const std::uint64_t high = low | (std::uint64_t{1} << 63);
    int differ = 0;
    for (int draw = 0; draw < 200; ++draw) {
        const DotProductHash hash(1'000'003, rng.engine());
        differ += hash(low) != hash(high);
    }
    CHECK_GT(differ, 150);
}

TEST(1, "any fixed pair of keys collides with probability about one over m") {
    Rng rng;
    const std::uint64_t slots = 101;
    const int draws = 20'000;
    const double limit = draws / static_cast<double>(slots) + 4.0 * std::sqrt(draws / static_cast<double>(slots));
    const std::pair<std::uint64_t, std::uint64_t> pairs[] = {
        {0, 1},
        {1, 1 + slots},
        {7, 7 + slots * slots},
        {0, slots * slots * slots},
        {123456789, 987654321},
    };
    for (const auto& [x, y] : pairs) {
        INFO("pair ", x, ", ", y);
        const int hits = collisions_for(x, y, draws,
                                        [&](std::mt19937_64& r) { return DotProductHash(slots, r); },
                                        rng.engine());
        CHECK_LE(static_cast<double>(hits), limit);
    }
}

TEST(2, "zero slots is rejected") {
    Rng rng;
    CHECK_THROWS_AS(LinearHash(0, rng.engine()), std::invalid_argument);
}

TEST(2, "one slot maps everything to zero") {
    Rng rng;
    const LinearHash hash(1, rng.engine());
    CHECK_EQ(hash(123), 0);
    CHECK_EQ(hash(kHashPrime - 1), 0);
}

TEST(2, "values stay in the table for keys near the limit") {
    Rng rng;
    const LinearHash hash(1024, rng.engine());
    for (std::uint64_t key : {0ULL, 1ULL, kHashPrime - 1, kHashPrime - 2}) {
        CHECK_LT(hash(key), 1024);
    }
}

TEST(2, "any fixed pair of keys collides with probability about one over m") {
    Rng rng;
    const std::uint64_t slots = 128;
    const int draws = 40'000;
    const double limit = draws / static_cast<double>(slots) + 4.0 * std::sqrt(draws / static_cast<double>(slots));
    const std::pair<std::uint64_t, std::uint64_t> pairs[] = {
        {0, 1},
        {1, 1 + slots},
        {2, 2 + (slots << 32)},
        {kHashPrime - 1, 0},
    };
    for (const auto& [x, y] : pairs) {
        INFO("pair ", x, ", ", y);
        const int hits = collisions_for(x, y, draws,
                                        [&](std::mt19937_64& r) { return LinearHash(slots, r); },
                                        rng.engine());
        CHECK_LE(static_cast<double>(hits), limit);
    }
}

PERF(2, "hashing fifty million keys", 3000) {
    Rng rng;
    const LinearHash hash(1 << 20, rng.engine());
    std::uint64_t checksum = 0;
    std::uint64_t key = 12345;
    for (int i = 0; i < 50'000'000; ++i) {
        checksum += hash(key % kHashPrime);
        key = key * 25214903917ULL + 11;
    }
    CHECK_GT(checksum, 0);
}

TEST(3, "a new table is empty") {
    const UniversalHashMap<int> table;
    CHECK(table.empty());
    CHECK_EQ(table.size(), 0);
    CHECK(table.find(5) == nullptr);
}

TEST(3, "a key at or above the prime is rejected") {
    UniversalHashMap<int> table;
    CHECK_THROWS_AS(table.insert(kHashPrime, 1), std::invalid_argument);
    CHECK_THROWS_AS(table.find(kHashPrime), std::invalid_argument);
    CHECK_THROWS_AS(table.erase(~std::uint64_t{0}), std::invalid_argument);
}

TEST(3, "inserting returns whether the key was new and overwrites existing values") {
    UniversalHashMap<int> table;
    CHECK(table.insert(1, 10));
    CHECK(!table.insert(1, 20));
    CHECK_EQ(table.size(), 1);
    CHECK_EQ(*table.find(1), 20);
}

TEST(3, "erasing reports whether the key was present") {
    UniversalHashMap<int> table;
    table.insert(1, 10);
    CHECK(!table.erase(2));
    CHECK(table.erase(1));
    CHECK(table.empty());
    CHECK(table.find(1) == nullptr);
    CHECK(!table.erase(1));
}

TEST(3, "values can be modified through find") {
    UniversalHashMap<int> table;
    table.insert(1, 10);
    *table.find(1) = 11;
    const auto& constant = table;
    CHECK_EQ(*constant.find(1), 11);
}

TEST(3, "the load factor stays at most one") {
    Rng rng;
    UniversalHashMap<int> table;
    for (int i = 0; i < 20'000; ++i) {
        table.insert(static_cast<std::uint64_t>(rng.uniform(0, 1'000'000'000)), i);
        CHECK_LE(table.size(), table.bucket_count());
    }
}

TEST(3, "chains stay short under adversarial keys") {
    UniversalHashMap<int> table;
    const int n = 1 << 16;
    for (int i = 0; i < n; ++i) {
        // Every key is a multiple of a large power of two, the classic flood against `key % 2^k`.
        table.insert(static_cast<std::uint64_t>(i) << 40, i);
    }
    CHECK_LE(table.longest_chain(), 3 * 16);
    for (int i = 0; i < n; ++i) {
        REQUIRE(table.find(static_cast<std::uint64_t>(i) << 40) != nullptr);
    }
}

TEST(3, "random operations match std::unordered_map") {
    Rng rng;
    UniversalHashMap<int> table(321);
    std::unordered_map<std::uint64_t, int> model;
    for (int step = 0; step < 40'000; ++step) {
        INFO("step ", step);
        const auto key = static_cast<std::uint64_t>(rng.uniform(0, 3000));
        if (rng.coin(0.6)) {
            REQUIRE_EQ(table.insert(key, step), !model.contains(key));
            model[key] = step;
        } else {
            REQUIRE_EQ(table.erase(key), model.erase(key) == 1);
        }
        REQUIRE_EQ(table.size(), model.size());
        const auto probe = static_cast<std::uint64_t>(rng.uniform(0, 3000));
        const auto* found = table.find(probe);
        REQUIRE_EQ(found != nullptr, model.contains(probe));
        if (found != nullptr) {
            REQUIRE_EQ(*found, model[probe]);
        }
    }
}

TEST(3, "no value leaks through rehashing") {
    const auto before = Tracked::live();
    {
        UniversalHashMap<Tracked> table;
        for (int i = 0; i < 2000; ++i) {
            table.insert(static_cast<std::uint64_t>(i), Tracked(i));
        }
        for (int i = 0; i < 1000; ++i) {
            table.erase(static_cast<std::uint64_t>(i));
        }
    }
    CHECK_EQ(Tracked::live(), before);
}

PERF(3, "a million adversarial keys", 2500) {
    UniversalHashMap<int> table(7);
    const int n = 1'000'000;
    for (int i = 0; i < n; ++i) {
        table.insert(static_cast<std::uint64_t>(i) << 32, i);
    }
    int found = 0;
    for (int i = 0; i < n; ++i) {
        found += table.find(static_cast<std::uint64_t>(i) << 32) != nullptr;
    }
    CHECK_EQ(found, n);
}

TEST(4, "non-positive limits and windows are rejected") {
    CHECK_THROWS_AS(RateLimiter(0, 10), std::invalid_argument);
    CHECK_THROWS_AS(RateLimiter(1, 0), std::invalid_argument);
    CHECK_THROWS_AS(RateLimiter(1, -5), std::invalid_argument);
}

TEST(4, "requests are allowed up to the limit in one window") {
    RateLimiter limiter(2, 10);
    CHECK(limiter.allow(1, 5));
    CHECK(limiter.allow(1, 6));
    CHECK(!limiter.allow(1, 7));
    CHECK_EQ(limiter.client_count(), 1);
}

TEST(4, "the counter resets at the window boundary") {
    RateLimiter limiter(1, 10);
    CHECK(limiter.allow(1, 9));
    CHECK(!limiter.allow(1, 9));
    CHECK(limiter.allow(1, 10));
    CHECK(!limiter.allow(1, 19));
    CHECK(limiter.allow(1, 20));
}

TEST(4, "clients are counted separately") {
    RateLimiter limiter(1, 10);
    CHECK(limiter.allow(1, 1));
    CHECK(limiter.allow(2, 1));
    CHECK(!limiter.allow(1, 1));
    CHECK_EQ(limiter.client_count(), 2);
}

TEST(4, "a skipped window does not carry over") {
    RateLimiter limiter(1, 10);
    CHECK(limiter.allow(1, 1));
    CHECK(limiter.allow(1, 100));
    CHECK(!limiter.allow(1, 105));
}

TEST(4, "negative timestamps fall into their own windows") {
    RateLimiter limiter(1, 10);
    CHECK(limiter.allow(1, -1));
    CHECK(!limiter.allow(1, -10));
    CHECK(limiter.allow(1, -11));
    CHECK(limiter.allow(1, 0));
}

TEST(4, "random traffic matches a per-client ledger") {
    Rng rng;
    RateLimiter limiter(3, 50, 123);
    std::map<std::uint64_t, std::pair<std::int64_t, std::uint64_t>> model;
    std::int64_t now = 0;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        now += rng.uniform(0, 4);
        const auto client = static_cast<std::uint64_t>(rng.uniform(0, 40));
        const std::int64_t bucket = now / 50;
        auto& [saved_bucket, count] = model[client];
        if (saved_bucket != bucket) {
            saved_bucket = bucket;
            count = 0;
        }
        const bool expected = count < 3;
        count += expected ? 1 : 0;
        REQUIRE_EQ(limiter.allow(client, now), expected);
    }
}

TEST(4, "a flood of crafted client ids keeps chains short") {
    RateLimiter limiter(1, 10, 99);
    const int n = 1 << 15;
    for (int i = 0; i < n; ++i) {
        limiter.allow(static_cast<std::uint64_t>(i) << 42, 0);
    }
    CHECK_EQ(limiter.client_count(), static_cast<std::size_t>(n));
    CHECK_LE(limiter.longest_chain(), 3 * 15);
}

PERF(4, "a flood of crafted client ids", 2500) {
    RateLimiter limiter(1, 1'000, 17);
    const int n = 400'000;
    std::int64_t allowed = 0;
    for (int round = 0; round < 3; ++round) {
        for (int i = 0; i < n; ++i) {
            allowed += limiter.allow(static_cast<std::uint64_t>(i) << 40, round);
        }
    }
    CHECK_EQ(allowed, n);
}
