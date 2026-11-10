#include <algorithm>
#include <list>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// A deliberately naive fully-associative LRU: one most-recent-first list.
std::uint64_t lru_misses(std::span<const std::uint64_t> addresses, std::size_t line_bytes,
                         std::size_t capacity) {
    if (capacity == 0) {
        return addresses.size();
    }
    std::list<std::uint64_t> recent;
    std::uint64_t misses = 0;
    for (const std::uint64_t address : addresses) {
        const std::uint64_t line = address / line_bytes;
        const auto found = std::find(recent.begin(), recent.end(), line);
        if (found != recent.end()) {
            recent.splice(recent.begin(), recent, found);
            continue;
        }
        ++misses;
        recent.push_front(line);
        if (recent.size() > capacity) {
            recent.pop_back();
        }
    }
    return misses;
}

std::vector<std::uint64_t> brute_miss_counts(std::span<const std::uint64_t> addresses,
                                             std::size_t line_bytes,
                                             std::span<const std::size_t> capacities) {
    std::vector<std::uint64_t> result;
    for (const std::size_t capacity : capacities) {
        result.push_back(lru_misses(addresses, line_bytes, capacity));
    }
    return result;
}

}  // namespace

TEST(1, "the level that satisfied each access is reported") {
    // A one-line L1 in front of a two-line L2.
    CacheHierarchy cache({{64, 64, 1}, {128, 64, 2}});
    CHECK_EQ(cache.access(0), 2);   // memory
    CHECK_EQ(cache.access(0), 0);   // L1
    CHECK_EQ(cache.access(64), 2);  // memory; L1 now holds line 1 only
    CHECK_EQ(cache.access(0), 1);   // evicted from L1, still in L2
}

TEST(1, "lower levels only see misses from the level above") {
    CacheHierarchy cache({{64 * 16 * 2, 64, 2}, {64 * 256 * 4, 64, 4}});
    cache.access(0);
    cache.access(0);
    cache.access(0);
    CHECK_EQ(cache.accesses(0), 3);
    CHECK_EQ(cache.misses(0), 1);
    CHECK_EQ(cache.accesses(1), 1);
    CHECK_EQ(cache.misses(1), 1);
}

TEST(1, "a working set between the two sizes is served by the second level") {
    CacheHierarchy cache({{4 * 64, 64, 4}, {64 * 64, 64, 8}});
    for (std::uint64_t line = 0; line < 16; ++line) {
        cache.access(line * 64);
    }
    for (int round = 0; round < 3; ++round) {
        for (std::uint64_t line = 0; line < 16; ++line) {
            INFO("round ", round, " line ", line);
            CHECK_EQ(cache.access(line * 64), 1);
        }
    }
}

TEST(1, "a multi-byte access goes through the hierarchy line by line") {
    CacheHierarchy cache({{64 * 4, 64, 2}, {64 * 64, 64, 8}});
    CHECK_EQ(cache.access_range(10, 8), 1);  // bytes 10..17: one line, from memory
    CHECK_EQ(cache.access_range(60, 8), 1);  // straddles lines 0 and 1; only line 1 is new
    CHECK_EQ(cache.accesses(0), 3);
    CHECK_EQ(cache.misses(0), 2);
    CHECK_EQ(cache.access_range(0, 640), 8);  // ten lines, two already cached somewhere
    CHECK_EQ(cache.access_range(0, 640), 0);  // all ten are in L2 now
}

TEST(1, "a zero-byte access does nothing") {
    CacheHierarchy cache({{1024, 64, 2}});
    CHECK_EQ(cache.access_range(100, 0), 0);
    CHECK_EQ(cache.accesses(0), 0);
}

TEST(1, "an access ending at the top of the address space does not overflow") {
    CacheHierarchy lines({{1024, 64, 2}});
    CHECK_EQ(lines.access_range(~std::uint64_t{0}, 1), 1);
    CHECK_EQ(lines.access_range(~std::uint64_t{0} - 63, 64), 0);
    CacheHierarchy bytes({{4, 1, 4}});
    CHECK_EQ(bytes.access_range(~std::uint64_t{0} - 1, 2), 2);
    CHECK_EQ(bytes.accesses(0), 2);
}

TEST(1, "invalid hierarchies are rejected") {
    CHECK_THROWS_AS(CacheHierarchy(std::vector<LevelConfig>{}), std::invalid_argument);
    CHECK_THROWS_AS(CacheHierarchy({{1024, 64, 2}, {4096, 128, 4}}), std::invalid_argument);
    CHECK_THROWS_AS(CacheHierarchy({{1000, 64, 2}}), std::invalid_argument);
}

TEST(2, "first touches always miss") {
    const std::vector<std::uint64_t> trace{0, 64, 128, 192};
    const std::vector<std::size_t> capacities{1, 2, 100};
    CHECK_EQ(miss_counts(trace, 64, capacities), std::vector<std::uint64_t>({4, 4, 4}));
}

TEST(2, "the stack distance decides hit or miss") {
    const std::vector<std::uint64_t> trace{0, 64, 0};  // line 0 is reused after one other line
    CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{1}), std::vector<std::uint64_t>({3}));
    CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{2}), std::vector<std::uint64_t>({2}));
}

TEST(2, "repeats within one line have distance zero") {
    const std::vector<std::uint64_t> trace{0, 8, 16, 63};
    CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{1}), std::vector<std::uint64_t>({1}));
}

TEST(2, "distance counts distinct lines, not accesses") {
    const std::vector<std::uint64_t> trace{0, 64, 64, 64, 0};  // three accesses, one distinct line
    CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{2}), std::vector<std::uint64_t>({2}));
}

TEST(2, "capacity zero misses everything") {
    const std::vector<std::uint64_t> trace{0, 0, 0};
    CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{0}), std::vector<std::uint64_t>({3}));
}

TEST(2, "capacities may come in any order") {
    const std::vector<std::uint64_t> trace{0, 64, 128, 0, 64, 128};
    const std::vector<std::size_t> capacities{3, 1, 2};
    CHECK_EQ(miss_counts(trace, 64, capacities), brute_miss_counts(trace, 64, capacities));
}

TEST(2, "an empty trace has no misses") {
    const std::vector<std::uint64_t> trace;
    CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{1, 8}),
             std::vector<std::uint64_t>({0, 0}));
}

TEST(2, "the curve matches brute-force LRU on random traces") {
    Rng rng;
    for (int round = 0; round < 60; ++round) {
        INFO("round ", round);
        const auto length = static_cast<std::size_t>(rng.uniform(1, 2000));
        const auto universe = rng.uniform(1, 800);
        std::vector<std::uint64_t> trace(length);
        for (auto& address : trace) {
            address = static_cast<std::uint64_t>(rng.uniform(0, universe)) * 16 +
                      static_cast<std::uint64_t>(rng.uniform(0, 15));
        }
        const std::vector<std::size_t> capacities{0, 1, 2, 3, 5, 8, 13, 21, 34, 55, 100, 1000};
        REQUIRE_EQ(miss_counts(trace, 16, capacities), brute_miss_counts(trace, 16, capacities));
    }
}

TEST(2, "the curve agrees with the simulator for a fully associative cache") {
    Rng rng;
    std::vector<std::uint64_t> trace(5000);
    for (auto& address : trace) {
        address = static_cast<std::uint64_t>(rng.uniform(0, 300)) * 64;
    }
    for (const std::size_t lines : {1, 4, 16, 64, 256}) {
        INFO("lines ", lines);
        CacheHierarchy cache({{lines * 64, 64, lines}});
        for (const std::uint64_t address : trace) {
            cache.access(address);
        }
        CHECK_EQ(miss_counts(trace, 64, std::vector<std::size_t>{lines}),
                 std::vector<std::uint64_t>({cache.misses(0)}));
    }
}

// Simulating each capacity separately with a list costs about n * C steps per capacity.
PERF(2, "the whole curve comes from one O(n log n) pass", 6000) {
    Rng rng;
    std::vector<std::uint64_t> trace(2'000'000);
    for (auto& address : trace) {
        address = static_cast<std::uint64_t>(rng.uniform(0, 200'000)) * 64;
    }
    const std::vector<std::size_t> capacities{1, 1000, 100'000, 1'000'000};
    const auto misses = miss_counts(trace, 64, capacities);
    REQUIRE_EQ(misses.size(), 4);
    CHECK_GT(misses[0], misses[1]);
    CHECK_GT(misses[1], misses[2]);
    CHECK_GT(misses[2], misses[3]);
}
