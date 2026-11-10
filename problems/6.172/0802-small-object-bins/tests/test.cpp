#include <algorithm>
#include <cstring>
#include <map>
#include <unordered_map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::Stopwatch;

namespace {

struct UpstreamStats {
    long allocations = 0;
    long deallocations = 0;
    long long outstanding = 0;
    std::unordered_map<void*, std::size_t> live;
};

struct CountingUpstream {
    UpstreamStats* stats;
    void* allocate(std::size_t bytes) {
        void* p = ::operator new(bytes);
        ++stats->allocations;
        stats->outstanding += static_cast<long long>(bytes);
        stats->live[p] = bytes;
        return p;
    }
    void deallocate(void* p, std::size_t bytes) noexcept {
        ++stats->deallocations;
        stats->outstanding -= static_cast<long long>(bytes);
        const auto found = stats->live.find(p);
        CHECK(found != stats->live.end());
        if (found != stats->live.end()) {
            CHECK_EQ(found->second, bytes);
            stats->live.erase(found);
        }
        ::operator delete(p);
    }
};

using Counting = BinnedAllocator<CountingUpstream>;
using Plain = BinnedAllocator<>;

bool aligned16(void* p) { return reinterpret_cast<std::uintptr_t>(p) % 16 == 0; }

}  // namespace

TEST(1, "bin_index maps sizes at and around every boundary") {
    CHECK_EQ(Plain::bin_index(1), 0);
    CHECK_EQ(Plain::bin_index(16), 0);
    CHECK_EQ(Plain::bin_index(17), 1);
    CHECK_EQ(Plain::bin_index(32), 1);
    CHECK_EQ(Plain::bin_index(33), 2);
    CHECK_EQ(Plain::bin_index(2048), 7);
    CHECK_EQ(Plain::bin_index(2049), 8);
    CHECK_EQ(Plain::bin_index(4096), 8);
}

TEST(1, "bin_size is the block size of each index") {
    std::size_t expected = 16;
    for (std::size_t index = 0; index < Plain::kBinCount; ++index) {
        INFO("index ", index);
        CHECK_EQ(Plain::bin_size(index), expected);
        expected *= 2;
    }
}

TEST(1, "every size lands in the smallest bin that holds it") {
    for (std::size_t size = 1; size <= Plain::kMaxBinSize; ++size) {
        INFO("size ", size);
        const std::size_t bin = Plain::bin_size(Plain::bin_index(size));
        REQUIRE(bin >= size);
        REQUIRE(bin < 16 || bin < 2 * size || size <= 16);
        if (Plain::bin_index(size) > 0) {
            REQUIRE(Plain::bin_size(Plain::bin_index(size) - 1) < size);
        }
    }
}

TEST(2, "blocks are aligned and large enough for their request") {
    Plain allocator;
    for (std::size_t size : {1, 8, 16, 17, 40, 100, 1000, 4096}) {
        INFO("size ", size);
        void* p = allocator.allocate(size);
        REQUIRE(aligned16(p));
        std::memset(p, 0x7E, size);
        allocator.deallocate(p, size);
    }
}

TEST(2, "blocks are recycled within one bin, most recent first") {
    Plain allocator;
    void* a = allocator.allocate(30);
    void* b = allocator.allocate(30);
    allocator.deallocate(a, 30);
    allocator.deallocate(b, 30);
    CHECK(allocator.allocate(25) == b);  // same bin (32), most recently freed comes back
    CHECK(allocator.allocate(32) == a);
}

TEST(2, "a freed block never serves a different bin") {
    Plain allocator;
    void* small = allocator.allocate(16);
    allocator.deallocate(small, 16);
    void* big = allocator.allocate(64);
    CHECK(big != small);
    allocator.deallocate(big, 64);
}

TEST(2, "blocks of the same size never overlap across many allocations") {
    Plain allocator(1 << 12);
    std::vector<unsigned char*> blocks;
    for (int i = 0; i < 2000; ++i) {
        auto* block = static_cast<unsigned char*>(allocator.allocate(48));
        std::memset(block, i & 0xFF, 48);
        blocks.push_back(block);
    }
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        INFO("block ", i);
        REQUIRE_EQ(static_cast<int>(blocks[i][0]), static_cast<int>(i & 0xFF));
        REQUIRE_EQ(static_cast<int>(blocks[i][47]), static_cast<int>(i & 0xFF));
    }
    for (unsigned char* block : blocks) {
        allocator.deallocate(block, 48);
    }
}

TEST(2, "each bin grows through its own chunks") {
    UpstreamStats stats;
    Counting allocator(1 << 10, CountingUpstream{&stats});
    std::vector<void*> blocks;
    for (int i = 0; i < 100; ++i) {
        blocks.push_back(allocator.allocate(100));
    }
    CHECK_GE(stats.allocations, 1);
    CHECK_LE(stats.allocations, 16);  // 7 blocks of 128 fit a 1 KiB chunk with a small header
    for (void* block : blocks) {
        allocator.deallocate(block, 100);
    }
}

TEST(2, "random mixed sizes never overlap a live block") {
    Rng rng;
    Plain allocator(1 << 13);
    struct Live {
        unsigned char* data;
        std::size_t size;
        int tag;
    };
    std::vector<Live> live;
    for (int step = 0; step < 30'000; ++step) {
        INFO("step ", step);
        if (live.size() < 64 && (live.empty() || rng.coin(0.6))) {
            const auto size = static_cast<std::size_t>(rng.uniform(1, 4096));
            auto* data = static_cast<unsigned char*>(allocator.allocate(size));
            const int tag = static_cast<int>(rng.uniform(0, 255));
            std::memset(data, tag, size);
            live.push_back({data, size, tag});
        } else {
            const auto i = static_cast<std::size_t>(
                rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            REQUIRE_EQ(static_cast<int>(live[i].data[0]), live[i].tag);
            REQUIRE_EQ(static_cast<int>(live[i].data[live[i].size - 1]), live[i].tag);
            allocator.deallocate(live[i].data, live[i].size);
            live[i] = live.back();
            live.pop_back();
        }
    }
}

TEST(3, "large requests bypass the bins") {
    UpstreamStats stats;
    Counting allocator(1 << 12, CountingUpstream{&stats});
    void* p = allocator.allocate(4097);
    std::memset(p, 1, 4097);
    allocator.deallocate(p, 4097);
    void* q = allocator.allocate(100000);
    allocator.deallocate(q, 100000);
    CHECK_EQ(allocator.large_allocations(), 2);
    CHECK_EQ(stats.allocations, 0);
}

TEST(3, "exactly 4096 bytes is still a small request") {
    UpstreamStats stats;
    Counting allocator(1 << 13, CountingUpstream{&stats});
    void* p = allocator.allocate(4096);
    CHECK_EQ(allocator.large_allocations(), 0);
    CHECK_EQ(stats.allocations, 1);
    allocator.deallocate(p, 4096);
}

TEST(3, "a zero-byte request returns a usable pointer") {
    Plain allocator;
    void* p = allocator.allocate(0);
    CHECK(p != nullptr);
    CHECK_EQ(allocator.live_bytes(), 0);
    allocator.deallocate(p, 0);
    CHECK_EQ(allocator.live_bytes(), 0);
    void* q = allocator.allocate(1);
    CHECK(q == p);  // same 16-byte bin, most recently freed comes back
    allocator.deallocate(q, 1);
}

TEST(3, "live_bytes counts what callers asked for") {
    Plain allocator;
    void* a = allocator.allocate(33);
    void* b = allocator.allocate(4097);
    CHECK_EQ(allocator.live_bytes(), 33 + 4097);
    allocator.deallocate(a, 33);
    CHECK_EQ(allocator.live_bytes(), 4097);
    allocator.deallocate(b, 4097);
    CHECK_EQ(allocator.live_bytes(), 0);
}

TEST(3, "destructor returns every chunk to the upstream") {
    UpstreamStats stats;
    {
        Counting allocator(1 << 12, CountingUpstream{&stats});
        for (int i = 0; i < 500; ++i) {
            allocator.allocate(static_cast<std::size_t>(16 + (i * 37) % 3000));
        }
        CHECK_GT(stats.outstanding, 0);
    }
    CHECK_EQ(stats.outstanding, 0);
    CHECK(stats.live.empty());
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

// A batch of mixed small requests, then freeing them in the same order.
template <typename Alloc, typename Free>
void churn(const std::vector<std::size_t>& sizes, int rounds, Alloc alloc, Free free) {
    std::vector<void*> slots(sizes.size());
    for (int r = 0; r < rounds; ++r) {
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            slots[i] = alloc(sizes[i]);
        }
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            free(slots[i], sizes[i]);
        }
    }
}

}  // namespace

PERF(4, "mixed small allocations beat new and delete at least twofold", 8000) {
    Rng rng;
    std::vector<std::size_t> sizes(256);
    for (auto& size : sizes) {
        size = static_cast<std::size_t>(rng.uniform(16, 1024));
    }
    const int rounds = 5000;

    Plain allocator;
    const double mine = best_of(5, [&] {
        churn(
            sizes, rounds, [&](std::size_t n) { return allocator.allocate(n); },
            [&](void* p, std::size_t n) { allocator.deallocate(p, n); });
    });
    const double baseline = best_of(5, [&] {
        churn(
            sizes, rounds, [](std::size_t n) { return ::operator new(n); },
            [](void* p, std::size_t) { ::operator delete(p); });
    });
    INFO("binned ", mine, " ms, new/delete ", baseline, " ms");
    CHECK_GT(baseline, 2.0 * mine);
}

PERF(3, "allocation does not get slower as the live set grows", 4000) {
    Plain allocator;
    std::vector<void*> live;
    live.reserve(1'000'000);
    Stopwatch small;
    for (int i = 0; i < 1'000'000; ++i) {
        live.push_back(allocator.allocate(32));
    }
    for (void* p : live) {
        allocator.deallocate(p, 32);
    }
    CHECK_LT(small.elapsed_ms(), 1000.0);
}
