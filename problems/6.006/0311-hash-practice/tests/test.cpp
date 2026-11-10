#include <algorithm>
#include <cstdint>
#include <optional>
#include <set>
#include <span>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Keys = std::vector<std::int64_t>;

namespace {

std::int64_t brute_smallest_modulus(const Keys& keys, std::int64_t a, std::int64_t b,
                                    std::int64_t buckets, std::int64_t limit) {
    for (std::int64_t c = 1; c <= limit; ++c) {
        std::set<std::int64_t> seen;
        bool ok = true;
        for (std::int64_t key : keys) {
            ok = seen.insert(((a * key + b) % c) % buckets).second && ok;
        }
        if (ok) {
            return c;
        }
    }
    return -1;
}

}  // namespace

TEST(1, "keys land in the chain their hash chooses, in insertion order") {
    const Keys keys{10, 21, 32, 5, 16};
    const auto layout = chain_layout(std::span<const std::int64_t>(keys), 4,
                                     [](std::int64_t key) { return key % 4; });
    REQUIRE_EQ(layout.size(), 4);
    CHECK_EQ(layout[0], Keys{32, 16});
    CHECK_EQ(layout[1], Keys{21, 5});
    CHECK_EQ(layout[2], Keys{10});
    CHECK_EQ(layout[3], Keys{});
}

TEST(1, "empty key list gives empty chains") {
    const Keys keys;
    const auto layout = chain_layout(std::span<const std::int64_t>(keys), 3,
                                     [](std::int64_t) { return 0; });
    CHECK_EQ(layout, std::vector<Keys>(3));
}

TEST(1, "one bucket keeps every key in order") {
    const Keys keys{5, 4, 3, 2};
    const auto layout = chain_layout(std::span<const std::int64_t>(keys), 1,
                                     [](std::int64_t) { return 0; });
    REQUIRE_EQ(layout.size(), 1);
    CHECK_EQ(layout[0], keys);
}

TEST(1, "random layouts match a straightforward simulation") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        Keys keys;
        const auto n = static_cast<std::size_t>(rng.uniform(0, 200));
        const auto m = static_cast<std::size_t>(rng.uniform(1, 40));
        for (std::size_t i = 0; i < n; ++i) {
            keys.push_back(rng.uniform(0, 100'000));
        }
        const auto hash = [m](std::int64_t key) {
            return static_cast<std::size_t>((10 * key + 3) % static_cast<std::int64_t>(m));
        };
        std::vector<Keys> expected(m);
        for (std::int64_t key : keys) {
            expected[hash(key)].push_back(key);
        }
        CHECK_EQ(chain_layout(std::span<const std::int64_t>(keys), m, hash), expected);
    }
}

TEST(2, "finds the smallest working modulus") {
    const Keys keys{12, 25, 38};
    // c = 1 and c = 2 collide; c = 3 sends the keys to 0, 1, 2.
    CHECK_EQ(smallest_injective_modulus(std::span<const std::int64_t>(keys), 1, 0, 3, 100),
             std::optional<std::int64_t>(3));
}

TEST(2, "the answer can use the offset and multiplier") {
    const Keys keys{0, 1, 2};
    // ((2k + 3) mod c) mod 3 sees the values 3, 5, 7: c = 1 and c = 2 collide, c = 3 gives 0, 2, 1.
    CHECK_EQ(smallest_injective_modulus(std::span<const std::int64_t>(keys), 2, 3, 3, 10),
             std::optional<std::int64_t>(3));
}

TEST(2, "returns nullopt when the limit is too small") {
    const Keys keys{12, 25, 38};
    CHECK_EQ(smallest_injective_modulus(std::span<const std::int64_t>(keys), 1, 0, 3, 2),
             std::optional<std::int64_t>());
}

TEST(2, "more keys than buckets returns nullopt without scanning a huge limit") {
    const Keys keys{1, 2, 3, 4, 5};
    CHECK_EQ(smallest_injective_modulus(std::span<const std::int64_t>(keys), 1, 0, 4,
                                        std::int64_t{1} << 62),
             std::optional<std::int64_t>());
}

TEST(2, "no keys are trivially injective for c = 1") {
    const Keys keys;
    CHECK_EQ(smallest_injective_modulus(std::span<const std::int64_t>(keys), 1, 0, 3, 5),
             std::optional<std::int64_t>(1));
}

TEST(2, "random instances match brute force") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 8));
        std::set<std::int64_t> chosen;
        while (chosen.size() < n) {
            chosen.insert(rng.uniform(0, 1000));
        }
        Keys keys(chosen.begin(), chosen.end());
        rng.shuffle(keys);
        const std::int64_t a = rng.uniform(1, 50);
        const std::int64_t b = rng.uniform(0, 50);
        const std::int64_t m = rng.uniform(1, 12);
        const std::int64_t limit = 2000;
        const auto brute = brute_smallest_modulus(keys, a, b, m, limit);
        const auto got = smallest_injective_modulus(std::span<const std::int64_t>(keys), a, b, m, limit);
        if (brute < 0) {
            REQUIRE(!got.has_value());
        } else {
            REQUIRE_EQ(got, std::optional<std::int64_t>(brute));
        }
    }
}

TEST(3, "perfect hash separates every stored key") {
    Rng rng;
    for (int round = 0; round < 15; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 64));
        std::set<std::int64_t> chosen;
        while (chosen.size() < n) {
            chosen.insert(rng.uniform(0, 1'000'000));
        }
        const Keys keys(chosen.begin(), chosen.end());
        const PerfectHashSet table{std::span<const std::int64_t>(keys)};
        CHECK_LE(table.table_size(), 4 * n + 1);
        std::set<std::size_t> slots;
        for (std::int64_t key : keys) {
            REQUIRE(table.slot_of(key) < table.table_size());
            REQUIRE(slots.insert(table.slot_of(key)).second);
            REQUIRE(table.contains(key));
        }
        for (int probe = 0; probe < 1000; ++probe) {
            const std::int64_t key = rng.uniform(0, 1'100'000);
            REQUIRE_EQ(table.contains(key), chosen.contains(key));
        }
    }
}

TEST(3, "a single key is stored") {
    const Keys keys{424242};
    const PerfectHashSet table{std::span<const std::int64_t>(keys)};
    CHECK(table.contains(424242));
    CHECK(!table.contains(1));
    CHECK_LE(table.table_size(), 5);
}

TEST(3, "keys sharing residues still get distinct slots") {
    Keys keys;
    for (std::int64_t i = 0; i < 40; ++i) {
        keys.push_back(i * 1000);
    }
    const PerfectHashSet table{std::span<const std::int64_t>(keys)};
    std::set<std::size_t> slots;
    for (std::int64_t key : keys) {
        CHECK(slots.insert(table.slot_of(key)).second);
    }
    CHECK(!table.contains(999));
}
