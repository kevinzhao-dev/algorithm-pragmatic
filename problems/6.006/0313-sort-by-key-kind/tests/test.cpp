#include <algorithm>
#include <cmath>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Slice make_slice(std::string id, std::uint32_t age, std::uint64_t thickness, std::uint64_t memory, int tag) {
    return Slice(std::move(id), age, thickness, memory, tag);
}

std::vector<Slice> batch(std::size_t n, const std::function<Slice(std::size_t)>& make) {
    std::vector<Slice> slices;
    slices.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        slices.push_back(make(i));
    }
    return slices;
}

template <typename Fn>
std::vector<std::uint64_t> extract(const std::vector<Slice>& slices, Fn field) {
    std::vector<std::uint64_t> out;
    out.reserve(slices.size());
    for (const Slice& slice : slices) {
        out.push_back(field(slice));
    }
    return out;
}

// All original tags must survive, each exactly once.
void check_permutation(const std::vector<Slice>& slices, std::size_t n) {
    std::vector<int> tags;
    tags.reserve(slices.size());
    for (const Slice& slice : slices) {
        tags.push_back(slice.tag);
    }
    std::sort(tags.begin(), tags.end());
    for (std::size_t i = 0; i < n; ++i) {
        REQUIRE_EQ(tags[i], static_cast<int>(i));
    }
}

}  // namespace

TEST(1, "sorts equal-length ids by byte value") {
    std::vector<Slice> slices;
    slices.push_back(make_slice("cab", 0, 0, 0, 0));
    slices.push_back(make_slice("abc", 0, 0, 0, 1));
    slices.push_back(make_slice("bca", 0, 0, 0, 2));
    slices.push_back(make_slice("aab", 0, 0, 0, 3));
    sort_by_core_id(slices);
    CHECK_EQ(slices[0].core_id, std::string("aab"));
    CHECK_EQ(slices[1].core_id, std::string("abc"));
    CHECK_EQ(slices[2].core_id, std::string("bca"));
    CHECK_EQ(slices[3].core_id, std::string("cab"));
}

TEST(1, "empty batch and zero-length ids") {
    std::vector<Slice> none;
    sort_by_core_id(none);
    CHECK(none.empty());

    auto zero = batch(3, [](std::size_t i) { return make_slice("", 0, 0, 0, static_cast<int>(i)); });
    sort_by_core_id(zero);
    check_permutation(zero, 3);
}

TEST(1, "ids that differ only at the last byte") {
    Rng rng;
    const std::string prefix(30, 'q');
    auto slices = batch(500, [&](std::size_t i) {
        return make_slice(prefix + static_cast<char>('a' + rng.uniform(0, 25)), 0, 0, 0, static_cast<int>(i));
    });
    sort_by_core_id(slices);
    check_permutation(slices, 500);
    for (std::size_t i = 1; i < slices.size(); ++i) {
        REQUIRE(slices[i - 1].core_id <= slices[i].core_id);
    }
}

TEST(1, "characters across the full ASCII range sort correctly") {
    std::vector<Slice> slices;
    for (int c = 127; c >= 0; c -= 3) {
        slices.push_back(make_slice(std::string(1, static_cast<char>(c)), 0, 0, 0, c));
    }
    sort_by_core_id(slices);
    for (std::size_t i = 1; i < slices.size(); ++i) {
        REQUIRE(slices[i - 1].core_id < slices[i].core_id);
    }
}

TEST(1, "random ids match std::sort") {
    Rng rng;
    for (int round = 0; round < 20; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 2000));
        const auto length = static_cast<std::size_t>(rng.uniform(1, 40));
        auto slices = batch(n, [&](std::size_t i) {
            return make_slice(rng.text(length, "abcdefghij0123"), 0, 0, 0, static_cast<int>(i));
        });
        std::vector<std::string> expected;
        for (const Slice& slice : slices) {
            expected.push_back(slice.core_id);
        }
        std::sort(expected.begin(), expected.end());
        sort_by_core_id(slices);
        check_permutation(slices, n);
        for (std::size_t i = 0; i < n; ++i) {
            REQUIRE_EQ(slices[i].core_id, expected[i]);
        }
    }
}

PERF(1, "200k ids of 64 bytes", 3000) {
    Rng rng;
    auto slices = batch(200'000, [&](std::size_t i) {
        return make_slice(rng.text(64, "abcdefgh"), 0, 0, 0, static_cast<int>(i));
    });
    sort_by_core_id(slices);
    for (std::size_t i = 1; i < slices.size(); ++i) {
        REQUIRE(slices[i - 1].core_id <= slices[i].core_id);
    }
}

TEST(2, "sorts by age") {
    auto slices = batch(6, [](std::size_t i) {
        return make_slice("", static_cast<std::uint32_t>(800'000 - i * 111), 0, 0, static_cast<int>(i));
    });
    sort_by_age(slices);
    for (std::size_t i = 1; i < slices.size(); ++i) {
        CHECK_LE(slices[i - 1].age, slices[i].age);
    }
    check_permutation(slices, 6);
}

TEST(2, "both extremes of the range") {
    std::vector<Slice> slices;
    slices.push_back(make_slice("", 800'000, 0, 0, 0));
    slices.push_back(make_slice("", 0, 0, 0, 1));
    slices.push_back(make_slice("", 400'000, 0, 0, 2));
    sort_by_age(slices);
    CHECK_EQ(slices[0].age, 0);
    CHECK_EQ(slices[1].age, 400'000);
    CHECK_EQ(slices[2].age, 800'000);
}

TEST(2, "empty and single-element batches") {
    std::vector<Slice> none;
    sort_by_age(none);
    auto one = batch(1, [](std::size_t) { return make_slice("", 77, 0, 0, 0); });
    sort_by_age(one);
    CHECK_EQ(one[0].age, 77);
}

TEST(2, "random batches of every size") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 3000));
        auto slices = batch(n, [&](std::size_t i) {
            return make_slice("", static_cast<std::uint32_t>(rng.uniform(0, 800'000)), 0, 0, static_cast<int>(i));
        });
        auto expected = extract(slices, [](const Slice& s) { return s.age; });
        std::sort(expected.begin(), expected.end());
        sort_by_age(slices);
        CHECK_EQ(extract(slices, [](const Slice& s) { return s.age; }), expected);
        check_permutation(slices, n);
    }
}

// A fresh 800001-entry table per call would cost about 1.6e11 steps here.
PERF(2, "two hundred thousand tiny batches", 3000) {
    Rng rng;
    std::uint64_t total = 0;
    for (int round = 0; round < 200'000; ++round) {
        auto slices = batch(5, [&](std::size_t i) {
            return make_slice("", static_cast<std::uint32_t>(rng.uniform(0, 800'000)), 0, 0, static_cast<int>(i));
        });
        sort_by_age(slices);
        total += slices[0].age;
    }
    CHECK_GE(total, 0);
}

PERF(2, "one and a half million ages in a single batch", 2500) {
    Rng rng;
    auto slices = batch(1'500'000, [&](std::size_t i) {
        return make_slice("", static_cast<std::uint32_t>(rng.uniform(0, 800'000)), 0, 0, static_cast<int>(i));
    });
    sort_by_age(slices);
    for (std::size_t i = 1; i < slices.size(); ++i) {
        REQUIRE(slices[i - 1].age <= slices[i].age);
    }
}

TEST(3, "sorts by numerator") {
    auto slices = batch(5, [](std::size_t i) {
        return make_slice("", 0, static_cast<std::uint64_t>(4 * 125 - i * 77), 0, static_cast<int>(i));
    });
    sort_by_thickness(slices);
    for (std::size_t i = 1; i < slices.size(); ++i) {
        CHECK_LE(slices[i - 1].thickness_numerator, slices[i].thickness_numerator);
    }
    check_permutation(slices, 5);
}

TEST(3, "numerators of zero and of the maximum 4 n^3") {
    const std::uint64_t n = 3;
    std::vector<Slice> slices;
    slices.push_back(make_slice("", 0, 4 * n * n * n, 0, 0));
    slices.push_back(make_slice("", 0, 0, 0, 1));
    slices.push_back(make_slice("", 0, 13, 0, 2));
    sort_by_thickness(slices);
    CHECK_EQ(slices[0].thickness_numerator, std::uint64_t{0});
    CHECK_EQ(slices[1].thickness_numerator, std::uint64_t{13});
    CHECK_EQ(slices[2].thickness_numerator, 4 * n * n * n);
}

// Spacing of 1 around 3.2e16 is finer than double precision there, so a detour through
// floating point collapses neighbours into ties.
TEST(3, "neighbouring numerators stay distinct above 2^53") {
    Rng rng;
    const std::size_t n = 200'000;
    const std::uint64_t cube = static_cast<std::uint64_t>(n) * n * n;
    const std::uint64_t band_start = 4 * cube - 1'000'000;
    std::vector<std::uint64_t> numerators(n);
    for (std::size_t i = 0; i < n; ++i) {
        numerators[i] = band_start + i * 3;  // distinct, spaced below double resolution
    }
    rng.shuffle(numerators);
    auto slices = batch(n, [&](std::size_t i) {
        return make_slice("", 0, numerators[i], 0, static_cast<int>(i));
    });
    sort_by_thickness(slices);
    auto expected = numerators;
    std::sort(expected.begin(), expected.end());
    CHECK_EQ(extract(slices, [](const Slice& s) { return s.thickness_numerator; }), expected);
    check_permutation(slices, n);
}

TEST(3, "random batches match std::sort") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(1, 3000));
        const std::uint64_t bound = 4 * static_cast<std::uint64_t>(n) * n * n;
        auto slices = batch(n, [&](std::size_t i) {
            return make_slice("", 0, rng.engine()() % (bound + 1), 0, static_cast<int>(i));
        });
        auto expected = extract(slices, [](const Slice& s) { return s.thickness_numerator; });
        std::sort(expected.begin(), expected.end());
        sort_by_thickness(slices);
        CHECK_EQ(extract(slices, [](const Slice& s) { return s.thickness_numerator; }), expected);
    }
}

PERF(3, "a million numerators up to 4 n^3", 2000) {
    Rng rng;
    const std::size_t n = 1'000'000;
    const std::uint64_t bound = 4 * static_cast<std::uint64_t>(n) * n * n;
    auto slices = batch(n, [&](std::size_t i) {
        return make_slice("", 0, rng.engine()() % (bound + 1), 0, static_cast<int>(i));
    });
    sort_by_thickness(slices);
    for (std::size_t i = 1; i < n; ++i) {
        REQUIRE(slices[i - 1].thickness_numerator <= slices[i].thickness_numerator);
    }
}

TEST(4, "sorts by an opaque predicate") {
    auto slices = batch(4, [](std::size_t i) {
        static const std::uint64_t values[] = {30, 10, 40, 20};
        return make_slice("", 0, 0, values[i], static_cast<int>(i));
    });
    sort_opaque(std::span<Slice>(slices), [](const Slice& a, const Slice& b) { return a.memory < b.memory; });
    CHECK_EQ(extract(slices, [](const Slice& s) { return s.memory; }),
             std::vector<std::uint64_t>{10, 20, 30, 40});
}

TEST(4, "any strict weak order works") {
    auto slices = batch(6, [](std::size_t i) { return make_slice("", 0, 0, i, static_cast<int>(i)); });
    sort_opaque(std::span<Slice>(slices), [](const Slice& a, const Slice& b) { return a.memory > b.memory; });
    CHECK_EQ(extract(slices, [](const Slice& s) { return s.memory; }),
             std::vector<std::uint64_t>{5, 4, 3, 2, 1, 0});
}

TEST(4, "empty and single-element batches") {
    std::vector<Slice> none;
    sort_opaque(std::span<Slice>(none), [](const Slice&, const Slice&) { return false; });
    auto one = batch(1, [](std::size_t) { return make_slice("", 0, 0, 1, 0); });
    sort_opaque(std::span<Slice>(one), [](const Slice&, const Slice&) { return false; });
    CHECK_EQ(one[0].memory, std::uint64_t{1});
}

TEST(4, "uses O(n log n) comparisons") {
    Rng rng;
    const std::size_t n = 100'000;
    auto slices = batch(n, [&](std::size_t i) {
        return make_slice("", 0, 0, rng.engine()(), static_cast<int>(i));
    });
    long comparisons = 0;
    sort_opaque(std::span<Slice>(slices), [&](const Slice& a, const Slice& b) {
        ++comparisons;
        return a.memory < b.memory;
    });
    for (std::size_t i = 1; i < n; ++i) {
        REQUIRE(slices[i - 1].memory <= slices[i].memory);
    }
    const auto lg = static_cast<long>(std::ceil(std::log2(static_cast<double>(n))));
    CHECK_LE(comparisons, 3 * static_cast<long>(n) * lg + 1000);
    check_permutation(slices, n);
}
