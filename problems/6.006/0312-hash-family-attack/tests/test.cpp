#include <algorithm>
#include <cstdint>
#include <numeric>
#include <optional>
#include <set>
#include <unordered_map>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::uint64_t linear_hash(std::uint64_t a, std::uint64_t b, std::uint64_t k, std::uint64_t n) {
    return (a * k + b) % n;
}

std::uint64_t bucket_hash(std::uint64_t a, std::uint64_t k, std::uint64_t n, std::uint64_t u) {
    return (static_cast<std::uint64_t>(static_cast<unsigned __int128>(k) * n / u) + a) % n;
}

std::uint64_t universal_hash(std::uint64_t a, std::uint64_t b, std::uint64_t k, std::uint64_t p,
                             std::uint64_t n) {
    return ((a * k + b) % p) % n;
}

bool linear_always_collides(std::uint64_t n, std::uint64_t k1, std::uint64_t k2) {
    for (std::uint64_t a = 1; a < n; ++a) {
        for (std::uint64_t b = 0; b < n; ++b) {
            if (linear_hash(a, b, k1, n) != linear_hash(a, b, k2, n)) {
                return false;
            }
        }
    }
    return true;
}

bool bucket_always_collides(std::uint64_t n, std::uint64_t u, std::uint64_t k1, std::uint64_t k2) {
    for (std::uint64_t a = 0; a < u; ++a) {
        if (bucket_hash(a, k1, n, u) != bucket_hash(a, k2, n, u)) {
            return false;
        }
    }
    return true;
}

void check_forced_pair_linear(std::uint64_t n, std::uint64_t u, const std::optional<KeyPair>& got) {
    INFO("n=", n, " u=", u);
    bool exists = false;
    for (std::uint64_t k1 = 1; k1 < u && !exists; ++k1) {
        for (std::uint64_t k2 = k1 + 1; k2 < u && !exists; ++k2) {
            exists = linear_always_collides(n, k1, k2);
        }
    }
    REQUIRE_EQ(got.has_value(), exists);
    if (got.has_value()) {
        const auto [k1, k2] = *got;
        CHECK(k1 >= 1 && k1 < u && k2 >= 1 && k2 < u && k1 != k2);
        CHECK(linear_always_collides(n, k1, k2));
    }
}

void check_forced_pair_bucket(std::uint64_t n, std::uint64_t u, const std::optional<KeyPair>& got) {
    INFO("n=", n, " u=", u);
    bool exists = false;
    for (std::uint64_t k1 = 1; k1 < u && !exists; ++k1) {
        for (std::uint64_t k2 = k1 + 1; k2 < u && !exists; ++k2) {
            exists = bucket_always_collides(n, u, k1, k2);
        }
    }
    REQUIRE_EQ(got.has_value(), exists);
    if (got.has_value()) {
        const auto [k1, k2] = *got;
        CHECK(k1 >= 1 && k1 < u && k2 >= 1 && k2 < u && k1 != k2);
        CHECK(bucket_always_collides(n, u, k1, k2));
    }
}

}  // namespace

TEST(1, "a guaranteed colliding pair exists when u is large enough") {
    const auto got = break_linear_family(5, 100);
    REQUIRE(got.has_value());
    CHECK_NE(got->first, got->second);
    CHECK(linear_always_collides(5, got->first, got->second));
}

TEST(1, "no pair exists when every key has its own residue") {
    // Keys 1..=n have pairwise distinct residues modulo n.
    CHECK(!break_linear_family(7, 8).has_value());
    CHECK(!break_linear_family(7, 3).has_value());
}

TEST(1, "boundary at u = n + 2") {
    const auto got = break_linear_family(7, 9);
    REQUIRE(got.has_value());
    CHECK(linear_always_collides(7, got->first, got->second));
}

TEST(1, "exhaustive check over all small families") {
    for (std::uint64_t n = 2; n <= 8; ++n) {
        for (std::uint64_t u = 2; u <= 20; ++u) {
            check_forced_pair_linear(n, u, break_linear_family(n, u));
        }
    }
}

TEST(1, "works for huge parameters") {
    const std::uint64_t n = std::uint64_t{1} << 30;
    const std::uint64_t u = std::uint64_t{1} << 62;
    const auto got = break_linear_family(n, u);
    REQUIRE(got.has_value());
    CHECK((got->first - got->second) % n == 0);
    CHECK(got->first < u && got->second < u);
    CHECK_NE(got->first, got->second);
}

TEST(2, "a guaranteed pair exists when a block holds two keys") {
    const auto got = break_bucket_family(3, 50);
    REQUIRE(got.has_value());
    CHECK(bucket_always_collides(3, 50, got->first, got->second));
}

TEST(2, "no pair exists when each key gets its own block") {
    CHECK(!break_bucket_family(9, 10).has_value());
    CHECK(!break_bucket_family(9, 5).has_value());
}

TEST(2, "boundary at u = n + 2") {
    const auto got = break_bucket_family(9, 11);
    REQUIRE(got.has_value());
    CHECK(bucket_always_collides(9, 11, got->first, got->second));
}

TEST(2, "exhaustive check over all small families") {
    for (std::uint64_t n = 2; n <= 8; ++n) {
        for (std::uint64_t u = 2; u <= 24; ++u) {
            check_forced_pair_bucket(n, u, break_bucket_family(n, u));
        }
    }
}

TEST(2, "huge u needs neither a scan nor overflow") {
    const std::uint64_t n = std::uint64_t{1} << 31;
    const std::uint64_t u = std::uint64_t{1} << 62;
    const auto got = break_bucket_family(n, u);
    REQUIRE(got.has_value());
    const auto [k1, k2] = *got;
    CHECK(k1 >= 1 && k2 < u && k1 != k2);
    const auto block = [&](std::uint64_t k) {
        return static_cast<std::uint64_t>(static_cast<unsigned __int128>(k) * n / u);
    };
    CHECK_EQ(block(k1), block(k2));
}

TEST(2, "many random shapes match brute force") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::uint64_t>(rng.uniform(2, 12));
        const auto u = static_cast<std::uint64_t>(rng.uniform(2, 40));
        check_forced_pair_bucket(n, u, break_bucket_family(n, u));
    }
}

TEST(3, "reports the exact probability in lowest terms") {
    // p = 5, n = 2: residues {0,2,4} and {1,3} give 3*2 + 2*1 = 8 of 5*4 = 20 ordered pairs.
    CHECK_EQ(universal_collision_probability(5, 2), Fraction{2, 5});
}

TEST(3, "n equal to p never collides") {
    CHECK_EQ(universal_collision_probability(7, 7), Fraction{0, 1});
}

TEST(3, "matches exhaustive enumeration for every key pair") {
    for (std::uint64_t p : {3, 5, 7, 11, 13, 17}) {
        for (std::uint64_t n = 2; n <= p; ++n) {
            INFO("p=", p, " n=", n);
            const Fraction expected = universal_collision_probability(p, n);
            CHECK_EQ(std::gcd(expected.num, expected.den), expected.num == 0 ? expected.den : std::uint64_t{1});
            for (std::uint64_t k1 = 0; k1 < p; ++k1) {
                for (std::uint64_t k2 = k1 + 1; k2 < p; ++k2) {
                    std::uint64_t collisions = 0, total = 0;
                    for (std::uint64_t a = 1; a < p; ++a) {
                        for (std::uint64_t b = 0; b < p; ++b) {
                            ++total;
                            collisions += universal_hash(a, b, k1, p, n) == universal_hash(a, b, k2, p, n) ? 1 : 0;
                        }
                    }
                    const std::uint64_t divisor = std::gcd(collisions, total);
                    REQUIRE_EQ(expected, Fraction{collisions / divisor, total / divisor});
                }
            }
        }
    }
}

TEST(3, "never exceeds one over n") {
    for (std::uint64_t p : {101, 1009, 65537, 2147483647}) {
        for (std::uint64_t n : {2, 3, 4, 7, 16, 100, 1000, 65536}) {
            if (n > p) {
                continue;
            }
            INFO("p=", p, " n=", n);
            const Fraction f = universal_collision_probability(p, n);
            CHECK_LE(static_cast<unsigned __int128>(f.num) * n, static_cast<unsigned __int128>(f.den));
        }
    }
}

TEST(4, "linear flood keys collide under every function") {
    const auto keys = flood_linear(6, 1000, 50);
    REQUIRE_EQ(keys.size(), 50);
    CHECK_EQ(std::set<std::uint64_t>(keys.begin(), keys.end()).size(), 50);
    for (std::uint64_t key : keys) {
        CHECK(key >= 1 && key < 1000);
    }
    for (std::uint64_t a = 1; a < 6; ++a) {
        for (std::uint64_t b = 0; b < 6; ++b) {
            std::set<std::uint64_t> buckets;
            for (std::uint64_t key : keys) {
                buckets.insert(linear_hash(a, b, key, 6));
            }
            CHECK_EQ(buckets.size(), 1);
        }
    }
}

TEST(4, "bucket flood keys collide under every function") {
    const auto keys = flood_bucket(10, 1000, 80);
    REQUIRE_EQ(keys.size(), 80);
    CHECK_EQ(std::set<std::uint64_t>(keys.begin(), keys.end()).size(), 80);
    for (std::uint64_t a = 0; a < 1000; ++a) {
        std::set<std::uint64_t> buckets;
        for (std::uint64_t key : keys) {
            buckets.insert(bucket_hash(a, key, 10, 1000));
        }
        CHECK_EQ(buckets.size(), 1);
    }
}

TEST(4, "the same keys spread out under a universal family") {
    Rng rng;
    const std::uint64_t p = 2147483647;
    const std::uint64_t n = 1024;
    const auto keys = flood_linear(n, std::uint64_t{1} << 40, 5000);
    REQUIRE_EQ(keys.size(), 5000);
    for (int trial = 0; trial < 20; ++trial) {
        const auto a = static_cast<std::uint64_t>(rng.uniform(1, static_cast<std::int64_t>(p - 1)));
        const auto b = static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(p - 1)));
        std::unordered_map<std::uint64_t, int> chains;
        int longest = 0;
        for (std::uint64_t key : keys) {
            longest = std::max(longest, ++chains[universal_hash(a, b, key, p, n)]);
        }
        CHECK_LT(longest, 60);
    }
}

TEST(4, "impossible requests return nothing") {
    CHECK(flood_linear(10, 11, 2).empty());  // only 1..10: every residue appears once
    CHECK(flood_linear(10, 15, 3).empty());  // 1, 11 fit but 21 is too big
    CHECK(flood_bucket(10, 12, 3).empty());
    CHECK(flood_linear(3, 100, 0).empty());
    CHECK(flood_bucket(3, 100, 0).empty());
}

TEST(4, "flood results are valid or empty for random small parameters") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        const auto n = static_cast<std::uint64_t>(rng.uniform(2, 15));
        const auto u = static_cast<std::uint64_t>(rng.uniform(2, 60));
        const auto count = static_cast<std::size_t>(rng.uniform(1, 12));
        INFO("n=", n, " u=", u, " count=", count);

        const auto linear = flood_linear(n, u, count);
        std::size_t max_class = 0;
        std::unordered_map<std::uint64_t, std::size_t> per_residue;
        for (std::uint64_t k = 1; k < u; ++k) {
            max_class = std::max(max_class, ++per_residue[k % n]);
        }
        REQUIRE_EQ(!linear.empty(), max_class >= count);
        if (!linear.empty()) {
            REQUIRE_EQ(linear.size(), count);
            std::set<std::uint64_t> distinct(linear.begin(), linear.end());
            REQUIRE_EQ(distinct.size(), count);
            std::set<std::uint64_t> residues;
            for (std::uint64_t key : linear) {
                REQUIRE(key >= 1 && key < u);
                residues.insert(key % n);
            }
            REQUIRE_EQ(residues.size(), 1);
        }

        const auto bucket = flood_bucket(n, u, count);
        std::size_t max_block = 0;
        std::unordered_map<std::uint64_t, std::size_t> per_block;
        for (std::uint64_t k = 1; k < u; ++k) {
            max_block = std::max(max_block, ++per_block[static_cast<std::uint64_t>(static_cast<unsigned __int128>(k) * n / u)]);
        }
        REQUIRE_EQ(!bucket.empty(), max_block >= count);
        if (!bucket.empty()) {
            REQUIRE_EQ(bucket.size(), count);
            std::set<std::uint64_t> distinct(bucket.begin(), bucket.end());
            REQUIRE_EQ(distinct.size(), count);
            std::set<std::uint64_t> blocks;
            for (std::uint64_t key : bucket) {
                REQUIRE(key >= 1 && key < u);
                blocks.insert(static_cast<std::uint64_t>(static_cast<unsigned __int128>(key) * n / u));
            }
            REQUIRE_EQ(blocks.size(), 1);
        }
    }
}

TEST(4, "works for very large u without enumerating it") {
    const std::uint64_t n = std::uint64_t{1} << 20;
    const std::uint64_t u = std::uint64_t{1} << 50;
    const auto keys = flood_bucket(n, u, 1000);
    REQUIRE_EQ(keys.size(), 1000);
    const auto block = static_cast<std::uint64_t>(static_cast<unsigned __int128>(keys.front()) * n / u);
    for (std::uint64_t key : keys) {
        CHECK_EQ(static_cast<std::uint64_t>(static_cast<unsigned __int128>(key) * n / u), block);
    }
}
