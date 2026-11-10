#include <algorithm>
#include <cstring>
#include <list>
#include <map>
#include <set>
#include <unordered_map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Stopwatch;
using practice::Tracked;

namespace {

struct UpstreamStats {
    long allocations = 0;
    long deallocations = 0;
    long long outstanding_bytes = 0;
    std::size_t largest_request = 0;
    std::unordered_map<void*, std::size_t> live;
};

// Records every chunk request so tests can check sizes, reuse and returns.
struct CountingUpstream {
    UpstreamStats* stats;
    void* allocate(std::size_t bytes) {
        void* p = ::operator new(bytes);
        ++stats->allocations;
        stats->outstanding_bytes += static_cast<long long>(bytes);
        stats->largest_request = std::max(stats->largest_request, bytes);
        stats->live[p] = bytes;
        return p;
    }
    void deallocate(void* p, std::size_t bytes) noexcept {
        ++stats->deallocations;
        stats->outstanding_bytes -= static_cast<long long>(bytes);
        const auto found = stats->live.find(p);
        CHECK(found != stats->live.end());
        if (found != stats->live.end()) {
            CHECK_EQ(found->second, bytes);
            stats->live.erase(found);
        }
        ::operator delete(p);
    }
};

using CountingPool = FixedBlockPool<CountingUpstream>;

bool aligned16(void* p) { return reinterpret_cast<std::uintptr_t>(p) % 16 == 0; }

}  // namespace

TEST(1, "blocks are distinct, aligned and hold block_size bytes") {
    UpstreamStats stats;
    CountingPool pool(40, 256, CountingUpstream{&stats});
    CHECK_EQ(pool.block_size(), 48);
    std::set<void*> seen;
    std::vector<void*> blocks;
    for (int i = 0; i < 200; ++i) {
        void* block = pool.allocate();
        REQUIRE(aligned16(block));
        CHECK(seen.insert(block).second);
        std::memset(block, static_cast<int>(i), pool.block_size());
        blocks.push_back(block);
    }
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        INFO("block ", i);
        auto* bytes = static_cast<unsigned char*>(blocks[i]);
        REQUIRE_EQ(static_cast<int>(bytes[0]), static_cast<int>(i));
        REQUIRE_EQ(static_cast<int>(bytes[pool.block_size() - 1]), static_cast<int>(i));
    }
}

TEST(1, "block size rounds up to fit a pointer and alignment") {
    FixedBlockPool<> tiny(1, 4);
    CHECK_EQ(tiny.block_size(), 16);
    FixedBlockPool<> exact(32, 4);
    CHECK_EQ(exact.block_size(), 32);
    FixedBlockPool<> odd(17, 4);
    CHECK_EQ(odd.block_size(), 32);
}

TEST(1, "the most recently freed block comes back first") {
    FixedBlockPool<> pool(64, 16);
    void* a = pool.allocate();
    void* b = pool.allocate();
    void* c = pool.allocate();
    pool.deallocate(a);
    pool.deallocate(c);
    pool.deallocate(b);
    CHECK(pool.allocate() == b);
    CHECK(pool.allocate() == c);
    CHECK(pool.allocate() == a);
}

TEST(1, "reusing a block does not touch its neighbours") {
    FixedBlockPool<> pool(32, 16);
    void* a = pool.allocate();
    void* b = pool.allocate();
    std::memset(a, 0xAA, 32);
    std::memset(b, 0xBB, 32);
    pool.deallocate(a);
    void* again = pool.allocate();
    CHECK(again == a);
    CHECK_EQ(static_cast<int>(static_cast<unsigned char*>(b)[0]), 0xBB);
    CHECK_EQ(static_cast<int>(static_cast<unsigned char*>(b)[31]), 0xBB);
}

TEST(1, "deallocating nullptr does nothing") {
    FixedBlockPool<> pool(16, 4);
    pool.deallocate(nullptr);
    CHECK_EQ(pool.allocated_blocks(), 0);
    void* a = pool.allocate();
    pool.deallocate(nullptr);
    CHECK_EQ(pool.allocated_blocks(), 1);
    pool.deallocate(a);
    CHECK_EQ(pool.allocated_blocks(), 0);
}

TEST(1, "allocated_blocks tracks what is outstanding") {
    FixedBlockPool<> pool(24, 16);
    void* a = pool.allocate();
    void* b = pool.allocate();
    CHECK_EQ(pool.allocated_blocks(), 2);
    pool.deallocate(a);
    CHECK_EQ(pool.allocated_blocks(), 1);
    pool.deallocate(b);
    CHECK_EQ(pool.allocated_blocks(), 0);
}

TEST(1, "random allocate and free never hand out a live block") {
    Rng rng;
    FixedBlockPool<> pool(48, 32);
    std::vector<void*> live;
    std::set<void*> live_set;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        if (live.empty() || (live.size() < 32 && rng.coin())) {
            void* block = pool.allocate();
            REQUIRE(live_set.insert(block).second);
            std::memset(block, 0x5C, pool.block_size());
            live.push_back(block);
        } else {
            const auto i = static_cast<std::size_t>(
                rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            REQUIRE(live_set.erase(live[i]) == 1);
            pool.deallocate(live[i]);
            live[i] = live.back();
            live.pop_back();
        }
    }
    CHECK_EQ(pool.allocated_blocks(), live.size());
}

TEST(2, "a new chunk is requested only when the current one is exhausted") {
    UpstreamStats stats;
    CountingPool pool(32, 8, CountingUpstream{&stats});
    CHECK_EQ(stats.allocations, 0);
    std::vector<void*> blocks;
    for (int i = 0; i < 8; ++i) {
        blocks.push_back(pool.allocate());
    }
    CHECK_EQ(pool.chunk_count(), 1);
    CHECK_EQ(stats.allocations, 1);
    blocks.push_back(pool.allocate());
    CHECK_EQ(pool.chunk_count(), 2);
    CHECK_EQ(stats.allocations, 2);
    for (void* block : blocks) {
        pool.deallocate(block);
    }
}

TEST(2, "freed blocks are used up before asking for a new chunk") {
    UpstreamStats stats;
    CountingPool pool(32, 4, CountingUpstream{&stats});
    std::vector<void*> blocks;
    for (int i = 0; i < 4; ++i) {
        blocks.push_back(pool.allocate());
    }
    for (void* block : blocks) {
        pool.deallocate(block);
    }
    for (int round = 0; round < 1000; ++round) {
        std::vector<void*> again;
        for (int i = 0; i < 4; ++i) {
            again.push_back(pool.allocate());
        }
        for (void* block : again) {
            pool.deallocate(block);
        }
    }
    CHECK_EQ(stats.allocations, 1);
    CHECK_EQ(pool.chunk_count(), 1);
}

TEST(2, "chunks are sized with constant overhead") {
    UpstreamStats stats;
    CountingPool pool(100, 10, CountingUpstream{&stats});
    pool.deallocate(pool.allocate());
    CHECK_GE(stats.largest_request, 10 * pool.block_size());
    CHECK_LE(stats.largest_request, 10 * pool.block_size() + 64);
}

TEST(2, "destructor returns every chunk even with blocks outstanding") {
    UpstreamStats stats;
    {
        CountingPool pool(32, 8, CountingUpstream{&stats});
        for (int i = 0; i < 20; ++i) {
            pool.allocate();
        }
        CHECK_EQ(pool.chunk_count(), 3);
        CHECK_GT(stats.outstanding_bytes, 0);
    }
    CHECK_EQ(stats.outstanding_bytes, 0);
    CHECK_EQ(stats.allocations, stats.deallocations);
    CHECK(stats.live.empty());
}

TEST(2, "blocks from different chunks never overlap") {
    FixedBlockPool<> pool(40, 7);
    std::vector<unsigned char*> blocks;
    for (int i = 0; i < 300; ++i) {
        blocks.push_back(static_cast<unsigned char*>(pool.allocate()));
        std::memset(blocks.back(), i & 0xFF, pool.block_size());
    }
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        INFO("block ", i);
        CHECK_EQ(static_cast<int>(blocks[i][0]), static_cast<int>(i & 0xFF));
        CHECK_EQ(static_cast<int>(blocks[i][pool.block_size() - 1]), static_cast<int>(i & 0xFF));
    }
}

TEST(3, "move construction transfers ownership of chunks and blocks") {
    UpstreamStats stats;
    {
        CountingPool a(32, 8, CountingUpstream{&stats});
        void* first = a.allocate();
        std::memset(first, 0x11, 32);

        CountingPool b(std::move(a));
        CHECK_EQ(a.chunk_count(), 0);  // NOLINT: moved-from state is part of the contract
        CHECK_EQ(a.allocated_blocks(), 0);
        CHECK_EQ(b.chunk_count(), 1);
        CHECK_EQ(b.allocated_blocks(), 1);
        CHECK_EQ(static_cast<int>(static_cast<unsigned char*>(first)[0]), 0x11);
        b.deallocate(first);
        CHECK_EQ(b.allocated_blocks(), 0);
        CHECK(b.allocate() == first);
    }
    CHECK_EQ(stats.outstanding_bytes, 0);
    CHECK(stats.live.empty());
}

TEST(3, "move assignment releases the old chunks and takes the new ones") {
    UpstreamStats stats_a, stats_b;
    {
        CountingPool a(32, 8, CountingUpstream{&stats_a});
        void* block = a.allocate();
        CountingPool b(16, 4, CountingUpstream{&stats_b});
        b.allocate();
        CHECK_EQ(stats_b.outstanding_bytes > 0, true);

        b = std::move(a);
        CHECK_EQ(stats_b.outstanding_bytes, 0);
        CHECK_EQ(b.block_size(), 32);
        b.deallocate(block);
        CHECK_EQ(b.allocated_blocks(), 0);
        CHECK(b.allocate() == block);
    }
    CHECK_EQ(stats_a.outstanding_bytes, 0);
    CHECK_EQ(stats_b.outstanding_bytes, 0);
}

TEST(3, "self move assignment keeps the pool usable") {
    UpstreamStats stats;
    {
        CountingPool pool(16, 8, CountingUpstream{&stats});
        void* a = pool.allocate();
        CountingPool& alias = pool;
        pool = std::move(alias);
        CHECK_EQ(pool.chunk_count(), 1);
        pool.deallocate(a);
        CHECK(pool.allocate() == a);
    }
    CHECK_EQ(stats.outstanding_bytes, 0);
}

TEST(3, "moved-from pools can be destroyed safely") {
    UpstreamStats stats;
    {
        CountingPool a(32, 8, CountingUpstream{&stats});
        a.allocate();
        { CountingPool b(std::move(a)); }
        CHECK_EQ(stats.outstanding_bytes, 0);
    }
    CHECK_EQ(stats.deallocations, stats.allocations);
}

TEST(4, "a list on PoolAllocator behaves like a list") {
    Rng rng;
    std::list<int, PoolAllocator<int>> list;
    std::list<int> model;
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 3);
        const int value = static_cast<int>(rng.uniform(0, 1'000));
        if (op <= 1 || model.empty()) {
            list.push_back(value);
            model.push_back(value);
        } else if (op == 2) {
            list.pop_front();
            model.pop_front();
        } else {
            list.pop_back();
            model.pop_back();
        }
    }
    CHECK(std::equal(list.begin(), list.end(), model.begin(), model.end()));
}

TEST(4, "a map on PoolAllocator works and destroys its elements") {
    const auto before = Tracked::live();
    {
        std::map<int, Tracked, std::less<int>, PoolAllocator<std::pair<const int, Tracked>>> map;
        for (int i = 0; i < 2000; ++i) {
            map.emplace(i, Tracked(i));
        }
        for (int i = 0; i < 2000; i += 2) {
            map.erase(i);
        }
        CHECK_EQ(map.size(), 1000);
        CHECK_EQ(map.at(1).value(), 1);
        CHECK_EQ(map.at(1999).value(), 1999);
    }
    CHECK_EQ(Tracked::live(), before);
}

TEST(4, "contiguous requests fall back to operator new") {
    std::vector<int, PoolAllocator<int>> values;
    for (int i = 0; i < 1000; ++i) {
        values.push_back(i);
    }
    CHECK_EQ(values[999], 999);
    CHECK_EQ(values[0], 0);
}

TEST(4, "copies and rebinds share one pool state") {
    PoolAllocator<int> a;
    PoolAllocator<int> copy(a);
    PoolAllocator<double> rebound(a);
    PoolAllocator<int> back(rebound);
    CHECK(a == copy);
    CHECK(a == back);

    PoolAllocator<int> other;
    CHECK(!(a == other));

    int* p = a.allocate(1);
    *p = 7;
    copy.deallocate(p, 1);
    int* q = a.allocate(1);
    CHECK(q == p);
    a.deallocate(q, 1);
}

namespace {

template <typename Fn>
double best_of(int rounds, Fn fn) {
    double best = 1e18;
    for (int i = 0; i < rounds; ++i) {
        Stopwatch watch;
        fn();
        best = std::min(best, watch.elapsed_ms());
    }
    return best;
}

// volatile keeps the optimizer from eliding the allocation pairs.
void* volatile sink;

void churn_new_delete(int cycles) {
    for (int i = 0; i < cycles; ++i) {
        void* a = ::operator new(64);
        void* b = ::operator new(64);
        sink = a;
        ::operator delete(a);
        sink = b;
        ::operator delete(b);
    }
}

void churn_pool(FixedBlockPool<>& pool, int cycles) {
    for (int i = 0; i < cycles; ++i) {
        void* a = pool.allocate();
        void* b = pool.allocate();
        sink = a;
        pool.deallocate(a);
        sink = b;
        pool.deallocate(b);
    }
}

}  // namespace

PERF(2, "pool allocate/free beats new/delete at least threefold", 8000) {
    FixedBlockPool<> pool(64, 4096);
    const double pool_ms = best_of(7, [&] { churn_pool(pool, 2'000'000); });
    const double new_ms = best_of(7, [] { churn_new_delete(2'000'000); });
    INFO("pool ", pool_ms, " ms, new/delete ", new_ms, " ms");
    CHECK_GT(new_ms, 3.0 * pool_ms);
}

PERF(4, "a pooled list costs no more than the default allocator", 8000) {
    const auto run_pool = [] {
        std::list<int, PoolAllocator<int>> list;
        for (int i = 0; i < 1'000'000; ++i) {
            list.push_back(i);
        }
        while (!list.empty()) {
            list.pop_front();
        }
    };
    const auto run_std = [] {
        std::list<int> list;
        for (int i = 0; i < 1'000'000; ++i) {
            list.push_back(i);
        }
        while (!list.empty()) {
            list.pop_front();
        }
    };
    const double pool_ms = best_of(5, run_pool);
    const double std_ms = best_of(5, run_std);
    INFO("pooled ", pool_ms, " ms, default ", std_ms, " ms");
    CHECK_LT(pool_ms, 1.3 * std_ms);
}
