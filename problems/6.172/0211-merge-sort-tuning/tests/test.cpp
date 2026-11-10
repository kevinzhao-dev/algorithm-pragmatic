#include <algorithm>
#include <cmath>
#include <cstring>
#include <random>
#include <vector>

#include "01_bench.hpp"
#include "alloc_counter.hpp"
#include "practice.hpp"
#include "solution.hpp"
#include "sort_baselines.hpp"

using practice::Counted;
using practice::Rng;

namespace {

struct Record {
    int key = 0;
    int sequence = 0;
    friend bool operator==(const Record&, const Record&) = default;
};

struct ByKey {
    bool operator()(const Record& a, const Record& b) const { return a.key < b.key; }
};

std::ostream& operator<<(std::ostream& os, const Record& r) {
    return os << '(' << r.key << ',' << r.sequence << ')';
}

// `sort` is a generic callable: sort(std::span<T>, compare).
template <typename Sort>
void battery(Sort sort) {
    {
        std::vector<int> none;
        sort(std::span(none), std::less<>{});
        CHECK(none.empty());
        std::vector<int> one = {5};
        sort(std::span(one), std::less<>{});
        CHECK_EQ(one, std::vector<int>{5});
        std::vector<int> two = {2, 1};
        sort(std::span(two), std::less<>{});
        CHECK_EQ(two, std::vector<int>({1, 2}));
    }
    {
        // A comparator other than the default.
        std::vector<int> values = {3, 1, 4, 1, 5, 9, 2, 6};
        sort(std::span(values), std::greater<>{});
        CHECK_EQ(values, std::vector<int>({9, 6, 5, 4, 3, 2, 1, 1}));
    }
    {
        // Already sorted, reversed, all equal.
        std::vector<int> sorted(500);
        std::iota(sorted.begin(), sorted.end(), 0);
        std::vector<int> input = sorted;
        sort(std::span(input), std::less<>{});
        CHECK_EQ(input, sorted);
        std::reverse(input.begin(), input.end());
        sort(std::span(input), std::less<>{});
        CHECK_EQ(input, sorted);
        std::vector<int> equal(300, 7);
        std::vector<int> expected = equal;
        sort(std::span(equal), std::less<>{});
        CHECK_EQ(equal, expected);
    }
    {
        Rng rng;
        for (const std::size_t n : {3, 4, 7, 16, 17, 33, 100, 1000, 4096}) {
            std::vector<int> values = rng.ints(n, -1000, 1000);
            std::vector<int> expected = values;
            std::sort(expected.begin(), expected.end());
            sort(std::span(values), std::less<>{});
            INFO("n ", n);
            REQUIRE_EQ(values, expected);
        }
    }
    {
        // Stability: equal keys keep their original relative order.
        Rng rng;
        for (const std::size_t n : {2, 16, 17, 40, 1000, 3000}) {
            std::vector<Record> records(n);
            for (std::size_t i = 0; i < n; ++i) {
                records[i] = {static_cast<int>(rng.uniform(0, 8)), static_cast<int>(i)};
            }
            std::vector<Record> expected = records;
            std::stable_sort(expected.begin(), expected.end(), ByKey{});
            sort(std::span(records), ByKey{});
            INFO("n ", n);
            REQUIRE_EQ(records, expected);
        }
    }
}

// sort(span, compare, cutoff) variants must also honour extreme cutoffs.
template <typename Sort>
void cutoff_battery(Sort sort) {
    Rng rng;
    for (const std::size_t cutoff : {0, 1, 2, 3, 16, 64, 100000}) {
        for (const std::size_t n : {0, 1, 5, 16, 17, 100, 999, 2048}) {
            std::vector<Record> records(n);
            for (std::size_t i = 0; i < n; ++i) {
                records[i] = {static_cast<int>(rng.uniform(0, 30)), static_cast<int>(i)};
            }
            std::vector<Record> expected = records;
            std::stable_sort(expected.begin(), expected.end(), ByKey{});
            sort(std::span(records), ByKey{}, cutoff);
            INFO("cutoff ", cutoff, " n ", n);
            REQUIRE_EQ(records, expected);
        }
    }
}

std::vector<std::uint32_t> random_u32(std::size_t n) {
    std::mt19937_64 engine(0x6172);
    std::vector<std::uint32_t> values(n);
    for (auto& value : values) {
        value = static_cast<std::uint32_t>(engine());
    }
    return values;
}

constexpr std::size_t kSpeedN = 1U << 20;
constexpr int kTrials = 5;

}  // namespace

TEST(1, "sort_a sorts correctly and stably") {
    battery([](auto span, auto cmp) { sort_a(span, cmp); });
}

TEST(1, "sort_a stays within n ceil(lg n) comparisons") {
    Rng rng;
    for (const std::size_t n : {1000, 1024, 4097}) {
        std::vector<Counted<int>> values;
        for (const int value : rng.ints(n, 0, 1'000'000)) {
            values.emplace_back(value);
        }
        Counted<int>::comparisons() = 0;
        sort_a(std::span(values));
        const auto limit = static_cast<std::uint64_t>(
            static_cast<double>(n) * std::ceil(std::log2(static_cast<double>(n))));
        INFO("n ", n, " comparisons ", Counted<int>::comparisons(), " limit ", limit);
        CHECK_LE(Counted<int>::comparisons(), limit);
        CHECK(std::is_sorted(values.begin(), values.end()));
    }
}

TEST(2, "sort_i sorts correctly and stably") {
    battery([](auto span, auto cmp) { sort_i(span, cmp); });
}

PERF(2, "inlining beats a type-erased baseline", 15000) {
    using baseline::KeyedRecord;
    std::vector<KeyedRecord> source(kSpeedN);
    std::mt19937_64 engine(0x6172);
    for (KeyedRecord& record : source) {
        record = {engine(), engine()};
    }
    const double ratio = bench::speedup(
        kTrials,
        [&] {
            std::vector<KeyedRecord> data = source;
            baseline::erased_sort(reinterpret_cast<char*>(data.data()), 0, data.size(),
                                  sizeof(KeyedRecord), baseline::compare_records);
            bench::do_not_optimize(data.data());
        },
        [&] {
            std::vector<KeyedRecord> data = source;
            sort_i(std::span(data),
                   [](const KeyedRecord& a, const KeyedRecord& b) { return a.key < b.key; });
            bench::do_not_optimize(data.data());
        });
    INFO("erased/sort_i speedup ", ratio);
    CHECK_GE(ratio, 1.1);
}

TEST(3, "sort_p sorts correctly and stably") {
    battery([](auto span, auto cmp) { sort_p(span, cmp); });
}

TEST(3, "add_to_all adds with wraparound and handles empty views") {
    std::vector<std::uint8_t> bytes = {0, 1, 200, 255};
    ByteView view{bytes.data(), bytes.size()};
    add_to_all(view, 56);
    CHECK_EQ(bytes, std::vector<std::uint8_t>({56, 57, 0, 55}));
    ByteView empty{nullptr, 0};
    add_to_all(empty, 7);
    CHECK_EQ(empty.size, 0);
}

PERF(3, "hoisting the fields beats reloading them through memory", 5000) {
    std::vector<std::uint8_t> bytes(1 << 20, 3);
    const double ratio = bench::speedup(
        kTrials,
        [&] {
            baseline::ByteView view{bytes.data(), bytes.size()};
            for (int round = 0; round < 40; ++round) {
                baseline::add_with_reload(view, 5);
            }
            bench::do_not_optimize(bytes.data());
        },
        [&] {
            ByteView view{bytes.data(), bytes.size()};
            for (int round = 0; round < 40; ++round) {
                add_to_all(view, 5);
            }
            bench::do_not_optimize(bytes.data());
        });
    INFO("reload/hoisted speedup ", ratio);
    CHECK_GE(ratio, 4.0);
}

TEST(4, "sort_c sorts correctly and stably") {
    battery([](auto span, auto cmp) { sort_c(span, cmp); });
}

TEST(4, "sort_c honours every cutoff") {
    cutoff_battery([](auto span, auto cmp, std::size_t cutoff) { sort_c(span, cmp, cutoff); });
}

PERF(4, "coarsening beats recursing to single elements", 15000) {
    const std::vector<std::uint32_t> source = random_u32(kSpeedN);
    const double ratio = bench::speedup(
        kTrials,
        [&] {
            std::vector<std::uint32_t> data = source;
            baseline::fine_grained_sort(data.data(), 0, data.size(), baseline::less_u32);
            bench::do_not_optimize(data.data());
        },
        [&] {
            std::vector<std::uint32_t> data = source;
            sort_c(std::span(data), [](std::uint32_t a, std::uint32_t b) { return a < b; });
            bench::do_not_optimize(data.data());
        });
    INFO("fine/coarse speedup ", ratio);
    CHECK_GE(ratio, 1.25);
}

TEST(5, "sort_m sorts correctly and stably") {
    battery([](auto span, auto cmp) { sort_m(span, cmp); });
}

TEST(5, "sort_m honours every cutoff") {
    cutoff_battery([](auto span, auto cmp, std::size_t cutoff) { sort_m(span, cmp, cutoff); });
}

TEST(5, "sort_m keeps scratch memory under half the input") {
    Rng rng;
    for (const std::size_t n : {1024, 2049, 10'000}) {
        std::vector<std::uint32_t> data(n);
        for (auto& value : data) {
            value = static_cast<std::uint32_t>(rng.uniform(0, 1'000'000));
        }
        alloc_counter::start();
        const std::int64_t live_before = alloc_counter::stats().live_bytes;
        sort_m(std::span(data));
        const std::int64_t peak = alloc_counter::stats().peak_bytes - live_before;
        INFO("n ", n, " peak bytes ", peak);
        CHECK(std::is_sorted(data.begin(), data.end()));
        CHECK_LE(peak, static_cast<std::int64_t>((n / 2 + 1) * sizeof(std::uint32_t)) + 16);
    }
}

TEST(6, "sort_f sorts correctly and stably") {
    battery([](auto span, auto cmp) { sort_f(span, cmp); });
}

TEST(6, "sort_f honours every cutoff") {
    cutoff_battery([](auto span, auto cmp, std::size_t cutoff) { sort_f(span, cmp, cutoff); });
}

TEST(6, "sort_f allocates its scratch exactly once") {
    Rng rng;
    for (const std::size_t n : {100, 1024, 4096, 40'000}) {
        std::vector<std::uint32_t> data(n);
        for (auto& value : data) {
            value = static_cast<std::uint32_t>(rng.uniform(0, 1'000'000));
        }
        alloc_counter::start();
        const std::int64_t live_before = alloc_counter::stats().live_bytes;
        sort_f(std::span(data));
        const std::uint64_t allocations = alloc_counter::stats().allocations;
        const std::int64_t peak = alloc_counter::stats().peak_bytes - live_before;
        INFO("n ", n, " allocations ", allocations, " peak bytes ", peak);
        CHECK(std::is_sorted(data.begin(), data.end()));
        CHECK_EQ(allocations, 1);
        CHECK_LE(peak, static_cast<std::int64_t>((n / 2 + 1) * sizeof(std::uint32_t)) + 16);
    }
}

TEST(6, "a tiny input needs no allocation at all") {
    std::vector<std::uint32_t> data = {3, 1, 2};
    alloc_counter::start();
    sort_f(std::span(data));
    CHECK_LE(alloc_counter::stats().allocations, 1);
    CHECK_EQ(data, std::vector<std::uint32_t>({1, 2, 3}));
}

PERF(6, "the fully tuned sort beats the fixed baseline", 15000) {
    const std::vector<std::uint32_t> source = random_u32(kSpeedN);
    const double ratio = bench::speedup(
        kTrials,
        [&] {
            std::vector<std::uint32_t> data = source;
            baseline::fine_grained_sort(data.data(), 0, data.size(), baseline::less_u32);
            bench::do_not_optimize(data.data());
        },
        [&] {
            std::vector<std::uint32_t> data = source;
            sort_f(std::span(data), [](std::uint32_t a, std::uint32_t b) { return a < b; });
            bench::do_not_optimize(data.data());
        });
    INFO("baseline/tuned speedup ", ratio);
    CHECK_GE(ratio, 1.4);
}
