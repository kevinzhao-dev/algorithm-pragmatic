#include <algorithm>
#include <bit>
#include <limits>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::uint64_t random_word(Rng& rng) {
    return (static_cast<std::uint64_t>(rng.uniform(0, 0xFFFFFFFFLL)) << 32) |
           static_cast<std::uint64_t>(rng.uniform(0, 0xFFFFFFFFLL));
}

std::uint64_t reverse_naive(std::uint64_t x) {
    std::uint64_t out = 0;
    for (int i = 0; i < 64; ++i) {
        out = (out << 1) | ((x >> i) & 1);
    }
    return out;
}

std::vector<std::uint64_t> interesting_words() {
    std::vector<std::uint64_t> words = {0,
                                        1,
                                        2,
                                        3,
                                        0x8000000000000000ULL,
                                        0xFFFFFFFFFFFFFFFFULL,
                                        0xAAAAAAAAAAAAAAAAULL,
                                        0x5555555555555555ULL,
                                        0x0F0F0F0F0F0F0F0FULL,
                                        0x8000000000000001ULL};
    for (int k = 0; k < 64; ++k) {
        const std::uint64_t power = std::uint64_t{1} << k;
        words.push_back(power);
        words.push_back(power - 1);
        words.push_back(power + 1);
    }
    return words;
}

}  // namespace

TEST(1, "set clear toggle touch only the target bit") {
    CHECK_EQ(set_bit(0, 0), 1U);
    CHECK_EQ(set_bit(0b1000, 3), 0b1000U);
    CHECK_EQ(clear_bit(0b1010, 3), 0b0010U);
    CHECK_EQ(clear_bit(0, 63), 0U);
    CHECK_EQ(toggle_bit(0b0101, 0), 0b0100U);
    CHECK_EQ(toggle_bit(0b0100, 0), 0b0101U);
    CHECK_EQ(set_bit(0, 63), 0x8000000000000000ULL);
    CHECK_EQ(toggle_bit(0x8000000000000000ULL, 63), 0U);
}

TEST(1, "extract_field reads the right window") {
    const std::uint64_t x = 0xDEADBEEFCAFEF00DULL;
    CHECK_EQ(extract_field(x, 0, 4), 0xDU);
    CHECK_EQ(extract_field(x, 4, 8), 0x00U);
    CHECK_EQ(extract_field(x, 60, 4), 0xDU);
    CHECK_EQ(extract_field(x, 0, 64), x);
    CHECK_EQ(extract_field(x, 10, 0), 0U);
    CHECK_EQ(extract_field(~std::uint64_t{0}, 0, 63), 0x7FFFFFFFFFFFFFFFULL);
}

TEST(1, "insert_field replaces only the window and ignores high value bits") {
    CHECK_EQ(insert_field(0, 4, 4, 0xF), 0xF0U);
    CHECK_EQ(insert_field(0xFF, 4, 4, 0x0), 0x0FU);
    CHECK_EQ(insert_field(0, 0, 8, 0x1FF), 0xFFU);
    CHECK_EQ(insert_field(0x123456789ABCDEF0ULL, 0, 64, 0x1), 0x1U);
    CHECK_EQ(insert_field(0xAB, 8, 0, 0xFF), 0xABU);
    CHECK_EQ(insert_field(0, 60, 4, 0xF), 0xF000000000000000ULL);
}

TEST(1, "random extract and insert round trip") {
    Rng rng;
    for (int step = 0; step < 50'000; ++step) {
        const std::uint64_t x = random_word(rng);
        const auto lo = static_cast<unsigned>(rng.uniform(0, 63));
        const auto len = static_cast<unsigned>(rng.uniform(0, 64 - lo));
        const std::uint64_t value = random_word(rng);
        INFO("x ", x, " lo ", lo, " len ", len);
        const std::uint64_t inserted = insert_field(x, lo, len, value);
        const std::uint64_t mask = len == 0 ? 0 : (len == 64 ? ~std::uint64_t{0} : ((std::uint64_t{1} << len) - 1)) << lo;
        REQUIRE_EQ(inserted & ~mask, x & ~mask);
        const std::uint64_t field = len == 0 ? 0 : (value & (len == 64 ? ~std::uint64_t{0} : ((std::uint64_t{1} << len) - 1)));
        REQUIRE_EQ(extract_field(inserted, lo, len), field);
    }
}

TEST(2, "min_no_branch agrees with std::min across the whole range") {
    Rng rng;
    const std::int64_t kMin = std::numeric_limits<std::int64_t>::min();
    const std::int64_t kMax = std::numeric_limits<std::int64_t>::max();
    CHECK_EQ(min_no_branch(kMin, kMax), kMin);
    CHECK_EQ(min_no_branch(kMax, kMin), kMin);
    CHECK_EQ(min_no_branch(-1, 0), -1);
    CHECK_EQ(min_no_branch(5, 5), 5);
    for (int step = 0; step < 100'000; ++step) {
        const auto a = static_cast<std::int64_t>(random_word(rng));
        const auto b = static_cast<std::int64_t>(random_word(rng));
        REQUIRE_EQ(min_no_branch(a, b), std::min(a, b));
    }
}

TEST(2, "abs_no_branch matches a plain absolute value") {
    CHECK_EQ(abs_no_branch(0), 0);
    CHECK_EQ(abs_no_branch(-1), 1);
    CHECK_EQ(abs_no_branch(7), 7);
    CHECK_EQ(abs_no_branch(std::numeric_limits<std::int64_t>::max()),
             std::numeric_limits<std::int64_t>::max());
    CHECK_EQ(abs_no_branch(std::numeric_limits<std::int64_t>::min() + 1),
             std::numeric_limits<std::int64_t>::max());
}

TEST(2, "mod_add_no_branch wraps exactly at n, including huge moduli") {
    CHECK_EQ(mod_add_no_branch(3, 4, 7), 0U);
    CHECK_EQ(mod_add_no_branch(3, 4, 8), 7U);
    CHECK_EQ(mod_add_no_branch(0, 0, 1), 0U);
    const std::uint64_t huge = std::uint64_t{1} << 63;
    CHECK_EQ(mod_add_no_branch(huge - 1, huge - 1, huge), huge - 2);
    CHECK_EQ(mod_add_no_branch(huge - 1, 1, huge), 0U);
    Rng rng;
    for (int step = 0; step < 20'000; ++step) {
        const auto n = static_cast<std::uint64_t>(rng.uniform(1, 1'000'000));
        const auto a = static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(n - 1)));
        const auto b = static_cast<std::uint64_t>(rng.uniform(0, static_cast<std::int64_t>(n - 1)));
        REQUIRE_EQ(mod_add_no_branch(a, b, n), (a + b) % n);
    }
}

TEST(2, "merge_no_branch produces the sorted union") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        std::vector<int> a = rng.ints(static_cast<std::size_t>(rng.uniform(0, 40)), -50, 50);
        std::vector<int> b = rng.ints(static_cast<std::size_t>(rng.uniform(0, 40)), -50, 50);
        std::sort(a.begin(), a.end());
        std::sort(b.begin(), b.end());
        std::vector<int> expected;
        std::merge(a.begin(), a.end(), b.begin(), b.end(), std::back_inserter(expected));
        std::vector<int> out(a.size() + b.size());
        merge_no_branch(a.data(), a.size(), b.data(), b.size(), out.data());
        INFO("round ", round);
        REQUIRE_EQ(out, expected);
    }
}

TEST(3, "ceil_pow2 matches bit_ceil and handles the edges") {
    CHECK_EQ(ceil_pow2(0), 1U);
    CHECK_EQ(ceil_pow2(1), 1U);
    CHECK_EQ(ceil_pow2(2), 2U);
    CHECK_EQ(ceil_pow2(3), 4U);
    CHECK_EQ(ceil_pow2(std::uint64_t{1} << 63), std::uint64_t{1} << 63);
    CHECK_EQ(ceil_pow2((std::uint64_t{1} << 63) + 1), 0U);
    CHECK_EQ(ceil_pow2(~std::uint64_t{0}), 0U);
    for (std::uint64_t x = 1; x < 70'000; ++x) {
        REQUIRE_EQ(ceil_pow2(x), std::bit_ceil(x));
    }
}

TEST(3, "lowest_set_bit isolates the lowest one") {
    CHECK_EQ(lowest_set_bit(0), 0U);
    CHECK_EQ(lowest_set_bit(0b1010000), 0b0010000U);
    CHECK_EQ(lowest_set_bit(0x8000000000000000ULL), 0x8000000000000000ULL);
    for (const std::uint64_t x : interesting_words()) {
        INFO("x ", x);
        CHECK_EQ(lowest_set_bit(x), x == 0 ? 0 : std::uint64_t{1} << std::countr_zero(x));
    }
}

TEST(3, "count_trailing_zeros agrees with the standard library") {
    Rng rng;
    for (const std::uint64_t x : interesting_words()) {
        if (x != 0) {
            INFO("x ", x);
            CHECK_EQ(count_trailing_zeros(x), std::countr_zero(x));
        }
    }
    for (int step = 0; step < 200'000; ++step) {
        const std::uint64_t x = random_word(rng) | 1ULL << static_cast<unsigned>(rng.uniform(0, 63));
        REQUIRE_EQ(count_trailing_zeros(x), std::countr_zero(x));
    }
}

TEST(3, "both popcounts agree with the standard library") {
    Rng rng;
    for (std::uint64_t x = 0; x < 70'000; ++x) {
        REQUIRE_EQ(popcount_table(x), std::popcount(x));
        REQUIRE_EQ(popcount_parallel(x), std::popcount(x));
    }
    for (const std::uint64_t x : interesting_words()) {
        INFO("x ", x);
        CHECK_EQ(popcount_table(x), std::popcount(x));
        CHECK_EQ(popcount_parallel(x), std::popcount(x));
    }
    for (int step = 0; step < 200'000; ++step) {
        const std::uint64_t x = random_word(rng);
        REQUIRE_EQ(popcount_table(x), std::popcount(x));
        REQUIRE_EQ(popcount_parallel(x), std::popcount(x));
    }
}

TEST(3, "reverse_bits is an involution and matches the naive version") {
    Rng rng;
    CHECK_EQ(reverse_bits(1), 0x8000000000000000ULL);
    CHECK_EQ(reverse_bits(0x8000000000000000ULL), 1U);
    CHECK_EQ(reverse_bits(0), 0U);
    for (int step = 0; step < 50'000; ++step) {
        const std::uint64_t x = random_word(rng);
        INFO("x ", x);
        REQUIRE_EQ(reverse_bits(x), reverse_naive(x));
        REQUIRE_EQ(reverse_bits(reverse_bits(x)), x);
    }
}

TEST(4, "SetBits iterates positions from low to high") {
    const std::uint64_t x = 0b1010011ULL | (std::uint64_t{1} << 63);
    std::vector<unsigned> positions;
    for (const unsigned position : SetBits(x)) {
        positions.push_back(position);
    }
    CHECK_EQ(positions, std::vector<unsigned>({0, 1, 4, 6, 63}));
}

TEST(4, "SetBits over zero is empty and over all ones visits 64 positions") {
    int count = 0;
    for ([[maybe_unused]] unsigned position : SetBits(0)) {
        ++count;
    }
    CHECK_EQ(count, 0);
    std::vector<unsigned> positions;
    for (const unsigned position : SetBits(~std::uint64_t{0})) {
        positions.push_back(position);
    }
    REQUIRE_EQ(positions.size(), 64);
    CHECK_EQ(positions[0], 0U);
    CHECK_EQ(positions[63], 63U);
}

TEST(4, "SetBits matches a bit-by-bit scan on random words") {
    Rng rng;
    for (int step = 0; step < 5'000; ++step) {
        const std::uint64_t x = random_word(rng);
        std::vector<unsigned> expected;
        for (unsigned i = 0; i < 64; ++i) {
            if ((x >> i) & 1) {
                expected.push_back(i);
            }
        }
        std::vector<unsigned> got;
        for (const unsigned position : SetBits(x)) {
            got.push_back(position);
        }
        REQUIRE_EQ(got, expected);
    }
}

TEST(4, "next_with_same_popcount walks k-subsets in increasing order") {
    // Every 3-element subset of 12 positions, by brute force.
    std::vector<std::uint64_t> expected;
    for (std::uint64_t mask = 0; mask < (1ULL << 12); ++mask) {
        if (std::popcount(mask) == 3) {
            expected.push_back(mask);
        }
    }
    std::vector<std::uint64_t> got;
    std::uint64_t mask = 0b111;
    while (mask < (1ULL << 12)) {
        got.push_back(mask);
        mask = next_with_same_popcount(mask);
    }
    CHECK_EQ(got, expected);
}

TEST(4, "next_with_same_popcount reports exhaustion") {
    CHECK_EQ(next_with_same_popcount(0), 0U);
    CHECK_EQ(next_with_same_popcount(~std::uint64_t{0}), 0U);
    CHECK_EQ(next_with_same_popcount(0xF000000000000000ULL), 0U);
    CHECK_EQ(next_with_same_popcount(std::uint64_t{1} << 63), 0U);
    CHECK_EQ(next_with_same_popcount(1), 2U);
    Rng rng;
    for (int step = 0; step < 50'000; ++step) {
        std::uint64_t x = random_word(rng);
        if (x == 0) {
            continue;
        }
        const std::uint64_t next = next_with_same_popcount(x);
        INFO("x ", x, " next ", next);
        if (next != 0) {
            CHECK_GT(next, x);
            CHECK_EQ(std::popcount(next), std::popcount(x));
        }
    }
}

TEST(4, "next_with_same_popcount never skips a valid successor") {
    Rng rng;
    for (int step = 0; step < 2'000; ++step) {
        const auto x = static_cast<std::uint64_t>(rng.uniform(1, (1LL << 20) - 1));
        std::uint64_t candidate = x + 1;
        while (std::popcount(candidate) != std::popcount(x)) {
            ++candidate;
        }
        INFO("x ", x);
        REQUIRE_EQ(next_with_same_popcount(x), candidate);
    }
}
