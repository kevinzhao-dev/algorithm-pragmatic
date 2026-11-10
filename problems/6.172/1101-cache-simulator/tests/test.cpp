#include <algorithm>
#include <deque>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

// A small, independent model used to check set-associative LRU behavior.
class ModelCache {
public:
    ModelCache(std::size_t capacity, std::size_t line, std::size_t ways)
        : line_(line), ways_(ways), sets_(capacity / line / ways), table_(sets_) {}

    bool access(std::uint64_t address) {
        const std::uint64_t line = address / line_;
        auto& set = table_[static_cast<std::size_t>(line) % sets_];
        const auto found = std::find(set.begin(), set.end(), line);
        const bool hit = found != set.end();
        if (hit) {
            set.erase(found);
        } else if (set.size() == ways_) {
            set.pop_back();
        }
        set.insert(set.begin(), line);
        return hit;
    }

private:
    std::size_t line_;
    std::size_t ways_;
    std::size_t sets_;
    std::vector<std::vector<std::uint64_t>> table_;
};

TEST(1, "invalid geometries are rejected") {
    CHECK_THROWS_AS(CacheSimulator(0, 64, 1), std::invalid_argument);
    CHECK_THROWS_AS(CacheSimulator(1024, 48, 1), std::invalid_argument);
    CHECK_THROWS_AS(CacheSimulator(1024, 64, 0), std::invalid_argument);
    CHECK_THROWS_AS(CacheSimulator(1000, 64, 1), std::invalid_argument);
    CHECK_THROWS_AS(CacheSimulator(64 * 3, 64, 1), std::invalid_argument);  // sets must be 2^k
    CHECK_THROWS_AS(CacheSimulator(64 * 6, 64, 2), std::invalid_argument);
}

TEST(1, "two addresses in the same line share a line") {
    CacheSimulator cache(1024, 64, 1);
    CHECK(!cache.access(100));
    CHECK(cache.access(101));
    CHECK(cache.access(127));
    CHECK(!cache.access(128));
    CHECK_EQ(cache.accesses(), 4);
    CHECK_EQ(cache.misses(), 2);
    CHECK_EQ(cache.hits(), 2);
}

TEST(1, "a direct-mapped set holds one line") {
    CacheSimulator cache(1024, 64, 1);  // 16 sets
    const std::uint64_t a = 0, b = 1024;  // both map to set 0
    CHECK(!cache.access(a));
    CHECK(!cache.access(b));
    CHECK(!cache.access(a));
    CHECK(!cache.access(b));
    CHECK_EQ(cache.misses(), 4);
}

TEST(1, "different sets do not disturb each other") {
    CacheSimulator cache(1024, 64, 1);
    for (std::uint64_t i = 0; i < 16; ++i) {
        CHECK(!cache.access(i * 64));
    }
    for (std::uint64_t i = 0; i < 16; ++i) {
        CHECK(cache.access(i * 64));
    }
}

TEST(1, "access_range touches each line once") {
    CacheSimulator cache(4096, 64, 1);
    cache.access_range(10, 100);  // lines 0 and 1
    CHECK_EQ(cache.accesses(), 2);
    CHECK_EQ(cache.misses(), 2);
    cache.access_range(64, 200);  // lines 1, 2, 3, 4
    CHECK_EQ(cache.accesses(), 6);
    CHECK_EQ(cache.misses(), 5);
    cache.access_range(0, 0);
    CHECK_EQ(cache.accesses(), 6);
}

TEST(1, "tags use the high address bits") {
    CacheSimulator cache(1024, 64, 1);
    const std::uint64_t low = 0x40;
    const std::uint64_t high = (std::uint64_t{1} << 40) | 0x40;  // same set, different tag
    CHECK(!cache.access(low));
    CHECK(!cache.access(high));
    CHECK(!cache.access(low));
}

TEST(1, "one-byte lines and a single set work at the top of the address space") {
    CacheSimulator cache(1, 1, 1);
    CHECK(!cache.access(~std::uint64_t{0}));
    CHECK(cache.access(~std::uint64_t{0}));
    CHECK(!cache.access(0));
    CHECK(!cache.access(~std::uint64_t{0}));
}

TEST(1, "access_range at the top of the address space does not overflow") {
    CacheSimulator lines(1024, 64, 2);
    lines.access_range(~std::uint64_t{0}, 1);
    CHECK_EQ(lines.accesses(), 1);
    CHECK_EQ(lines.misses(), 1);
    CacheSimulator bytes(4, 1, 4);
    bytes.access_range(~std::uint64_t{0} - 1, 2);
    CHECK_EQ(bytes.accesses(), 2);
    CHECK_EQ(bytes.misses(), 2);
}

TEST(2, "LRU picks the least recently used line") {
    CacheSimulator cache(2 * 64, 64, 2);  // one set, two ways
    CHECK(!cache.access(0));
    CHECK(!cache.access(64));
    CHECK(cache.access(0));     // 64 becomes least recent
    CHECK(!cache.access(128));  // evicts 64
    CHECK(cache.access(0));
    CHECK(!cache.access(64));
}

TEST(2, "a cyclic pattern that fits never misses after the first pass") {
    CacheSimulator cache(4 * 64, 64, 4);
    for (int pass = 0; pass < 3; ++pass) {
        for (std::uint64_t i = 0; i < 4; ++i) {
            cache.access(i * 64);
        }
    }
    CHECK_EQ(cache.misses(), 4);
    CHECK_EQ(cache.accesses(), 12);
}

TEST(2, "a cyclic pattern one line too big misses every time under LRU") {
    CacheSimulator cache(4 * 64, 64, 4);
    for (int pass = 0; pass < 5; ++pass) {
        for (std::uint64_t i = 0; i < 5; ++i) {
            cache.access(i * 64);
        }
    }
    CHECK_EQ(cache.misses(), 25);
}

TEST(2, "fully associative holds any set of lines up to capacity") {
    CacheSimulator cache(8 * 64, 64, 8);
    for (std::uint64_t i = 0; i < 8; ++i) {
        cache.access(i * 4096 * 64);  // would all clash in a set-mapped cache
    }
    for (std::uint64_t i = 0; i < 8; ++i) {
        CHECK(cache.access(i * 4096 * 64));
    }
}

TEST(2, "random traces match an independent model across geometries") {
    Rng rng;
    struct Geometry {
        std::size_t capacity, line, ways;
    };
    const std::vector<Geometry> geometries{
        {1024, 32, 1}, {1024, 32, 2}, {1024, 32, 4}, {4096, 64, 8}, {4096, 64, 16},
        {8192, 64, 4}, {512, 16, 32},  {256, 16, 16},
    };
    for (const auto& g : geometries) {
        INFO("capacity ", g.capacity, " line ", g.line, " ways ", g.ways);
        CacheSimulator cache(g.capacity, g.line, g.ways);
        ModelCache model(g.capacity, g.line, g.ways);
        std::uint64_t expected_misses = 0;
        for (int step = 0; step < 20'000; ++step) {
            INFO("step ", step);
            const auto address = static_cast<std::uint64_t>(rng.uniform(0, 3 * static_cast<std::int64_t>(g.capacity)));
            const bool want = model.access(address);
            REQUIRE_EQ(cache.access(address), want);
            expected_misses += want ? 0 : 1;
        }
        REQUIRE_EQ(cache.misses(), expected_misses);
    }
}

TEST(3, "first-touch misses are cold") {
    CacheSimulator cache(1024, 64, 2);
    for (std::uint64_t i = 0; i < 8; ++i) {
        cache.access(i * 64);
    }
    CHECK_EQ(cache.cold_misses(), 8);
    CHECK_EQ(cache.conflict_misses(), 0);
    CHECK_EQ(cache.capacity_misses(), 0);
}

TEST(3, "two lines fighting over one set are conflict misses") {
    CacheSimulator cache(1024, 64, 1);
    const std::uint64_t a = 0, b = 1024;  // same set, both would fit in a fully associative cache
    for (int i = 0; i < 10; ++i) {
        cache.access(a);
        cache.access(b);
    }
    CHECK_EQ(cache.cold_misses(), 2);
    CHECK_EQ(cache.conflict_misses(), 18);
    CHECK_EQ(cache.capacity_misses(), 0);
}

TEST(3, "cycling through more lines than capacity is capacity misses") {
    CacheSimulator cache(4 * 64, 64, 4);
    for (int pass = 0; pass < 6; ++pass) {
        for (std::uint64_t i = 0; i < 8; ++i) {
            cache.access(i * 64);
        }
    }
    CHECK_EQ(cache.cold_misses(), 8);
    CHECK_EQ(cache.capacity_misses(), 40);
    CHECK_EQ(cache.conflict_misses(), 0);
}

TEST(3, "the three kinds always add up to the total") {
    Rng rng;
    CacheSimulator cache(2048, 32, 4);
    for (int step = 0; step < 50'000; ++step) {
        cache.access(static_cast<std::uint64_t>(rng.uniform(0, 8000)));
        REQUIRE_EQ(cache.cold_misses() + cache.conflict_misses() + cache.capacity_misses(),
                   cache.misses());
    }
}

TEST(3, "set associativity cuts conflict misses but not cold misses") {
    // Lines 0 and 16, and 1 and 17, clash when direct mapped but fit in two ways.
    const std::vector<std::uint64_t> lines{0, 16, 1, 17};
    CacheSimulator direct(16 * 64, 64, 1);
    CacheSimulator associative(16 * 64, 64, 2);
    for (int pass = 0; pass < 20; ++pass) {
        for (const std::uint64_t line : lines) {
            direct.access(line * 64);
            associative.access(line * 64);
        }
    }
    CHECK_EQ(direct.cold_misses(), associative.cold_misses());
    CHECK_EQ(associative.conflict_misses(), 0);
    CHECK_EQ(direct.conflict_misses(), 4 * 19);
}

TEST(4, "invalidations show up as coherence misses") {
    CoherentCaches caches(2, 1024, 64, 2);
    CHECK(!caches.read(0, 0));
    CHECK(!caches.read(1, 0));
    CHECK(caches.read(0, 8));   // same line, still shared
    CHECK(!caches.write(0, 0) == false);
    // Core 0 now writes; core 1 was invalidated, so its next read misses.
    CHECK_EQ(caches.coherence_misses(), 0);
    CHECK(!caches.read(1, 0));
    CHECK_EQ(caches.coherence_misses(), 1);
}

TEST(4, "reads alone never invalidate") {
    CoherentCaches caches(3, 1024, 64, 2);
    for (int pass = 0; pass < 10; ++pass) {
        for (std::size_t core = 0; core < 3; ++core) {
            caches.read(core, 512);
        }
    }
    CHECK_EQ(caches.coherence_misses(), 0);
}

TEST(4, "a line written by a single core causes no coherence misses") {
    CoherentCaches caches(4, 1024, 64, 2);
    for (int i = 0; i < 100; ++i) {
        caches.write(2, 64);
        caches.read(2, 64);
    }
    CHECK_EQ(caches.coherence_misses(), 0);
}

TEST(4, "two cores writing alternately ping-pong the line") {
    CoherentCaches caches(2, 1024, 64, 2);
    for (int i = 0; i < 50; ++i) {
        caches.write(0, 0);
        caches.write(1, 0);
    }
    // After the first two cold misses, every write finds its line invalidated by the other.
    CHECK_EQ(caches.coherence_misses(), 98);
}

TEST(4, "false sharing: packed counters thrash, padded counters do not") {
    const int threads = 6;
    const int iterations = 200;
    const auto run = [&](std::size_t stride) {
        CoherentCaches caches(static_cast<std::size_t>(threads), 1 << 14, 64, 4);
        for (int iteration = 0; iteration < iterations; ++iteration) {
            for (int t = 0; t < threads; ++t) {
                caches.write(static_cast<std::size_t>(t), static_cast<std::uint64_t>(t) * stride);
            }
        }
        return caches.coherence_misses();
    };
    CHECK_GT(run(4), static_cast<std::uint64_t>(iterations));   // adjacent ints, one shared line
    CHECK_EQ(run(64), 0);                                        // one line per counter
}

TEST(4, "evicted lines are not counted as coherence misses") {
    CoherentCaches caches(2, 2 * 64, 64, 2);  // one set, two ways
    caches.read(0, 0);
    caches.read(0, 64);
    caches.read(0, 128);  // evicts 0 by capacity, not by invalidation
    CHECK(!caches.read(0, 0));
    CHECK_EQ(caches.coherence_misses(), 0);
}

// Scanning every line of the cache on each access would take minutes here.
PERF(2, "access cost does not grow with the number of lines", 8000) {
    CacheSimulator cache(std::size_t{1} << 29, 64, 8);  // 512 MiB: a million sets
    Rng rng;
    std::vector<std::uint64_t> addresses(200'000);
    for (auto& address : addresses) {
        address = static_cast<std::uint64_t>(rng.uniform(0, std::int64_t{1} << 40));
    }
    for (int round = 0; round < 10; ++round) {
        for (const std::uint64_t address : addresses) {
            cache.access(address);
        }
    }
    CHECK_GT(cache.hits(), 0);
}
