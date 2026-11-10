#include <algorithm>
#include <cstring>
#include <map>
#include <memory>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using practice::cachesim::Cache;

namespace {

// Hands out 8-byte-aligned blocks at unpredictable offsets from one real buffer.
class ScatterUpstream {
public:
    ScatterUpstream(std::size_t capacity, Rng& rng) : rng_(rng), buffer_(capacity) {}

    void* allocate(std::size_t bytes) {
        cursor_ += static_cast<std::size_t>(rng_.uniform(0, 7)) * 8;
        REQUIRE(cursor_ + bytes <= buffer_.size());
        std::byte* block = buffer_.data() + cursor_;
        cursor_ += bytes;
        cursor_ = (cursor_ + 7) & ~std::size_t{7};
        grants_[block] = bytes;
        ++calls;
        return block;
    }

    void deallocate(void* p) {
        const auto found = grants_.find(static_cast<std::byte*>(p));
        REQUIRE(found != grants_.end());
        grants_.erase(found);
        ++frees;
    }

    // The range this upstream granted that contains [p, p + bytes).
    bool inside_a_grant(const void* p, std::size_t bytes) const {
        const auto* pointer = static_cast<const std::byte*>(p);
        auto after = grants_.upper_bound(pointer);
        if (after == grants_.begin()) {
            return false;
        }
        --after;
        return pointer >= after->first && pointer + bytes <= after->first + after->second;
    }

    int live_grants() const { return static_cast<int>(grants_.size()); }

    int calls = 0;
    int frees = 0;

private:
    Rng& rng_;
    std::vector<std::byte> buffer_;
    std::size_t cursor_ = 0;
    std::map<const std::byte*, std::size_t> grants_;
};

struct AlignedBuffer {
    explicit AlignedBuffer(std::size_t bytes)
        : size(bytes), memory(::operator new(bytes, std::align_val_t{kCacheLine})) {}
    ~AlignedBuffer() { ::operator delete(memory, std::align_val_t{kCacheLine}); }
    std::span<std::byte> span() { return {static_cast<std::byte*>(memory), size}; }
    std::size_t size;
    void* memory;
};

std::size_t lines_spanned(const void* p, std::size_t bytes) {
    const auto start = reinterpret_cast<std::uintptr_t>(p);
    return (start + bytes - 1) / kCacheLine - start / kCacheLine + 1;
}

}  // namespace

TEST(1, "pointers are cache-line aligned for any upstream state") {
    Rng rng;
    ScatterUpstream upstream(1 << 22, rng);
    CacheAlignedWrapper wrapper(upstream);
    for (int i = 0; i < 1000; ++i) {
        INFO("allocation ", i);
        const auto size = static_cast<std::size_t>(rng.uniform(1, 500));
        void* p = wrapper.allocate(size);
        REQUIRE(p != nullptr);
        REQUIRE_EQ(reinterpret_cast<std::uintptr_t>(p) % kCacheLine, 0);
        REQUIRE(upstream.inside_a_grant(p, size));
        std::memset(p, 0xCD, size);
    }
    CHECK_EQ(upstream.calls, 1000);
}

TEST(1, "zero bytes returns nullptr and calls upstream never") {
    Rng rng;
    ScatterUpstream upstream(1 << 16, rng);
    CacheAlignedWrapper wrapper(upstream);
    CHECK(wrapper.allocate(0) == nullptr);
    CHECK_EQ(upstream.calls, 0);
}

TEST(1, "the request to upstream stays within the worst-case padding") {
    Rng rng;
    ScatterUpstream upstream(1 << 18, rng);
    CacheAlignedWrapper wrapper(upstream);
    void* p = wrapper.allocate(100);
    REQUIRE(p != nullptr);
    CHECK_LE(wrapper.overhead_bytes(), 63 + sizeof(void*));
    void* q = wrapper.allocate(1);
    REQUIRE(q != nullptr);
    CHECK_LE(wrapper.overhead_bytes(), 2 * (63 + sizeof(void*)));
}

TEST(1, "writing every object leaves its neighbours intact") {
    Rng rng;
    ScatterUpstream upstream(1 << 20, rng);
    CacheAlignedWrapper wrapper(upstream);
    std::vector<std::pair<unsigned char*, std::size_t>> objects;
    for (int i = 0; i < 300; ++i) {
        const auto size = static_cast<std::size_t>(rng.uniform(1, 200));
        auto* p = static_cast<unsigned char*>(wrapper.allocate(size));
        std::memset(p, i & 0xFF, size);
        objects.emplace_back(p, size);
    }
    for (std::size_t i = 0; i < objects.size(); ++i) {
        INFO("object ", i);
        REQUIRE_EQ(static_cast<int>(objects[i].first[0]), static_cast<int>(i & 0xFF));
        REQUIRE_EQ(static_cast<int>(objects[i].first[objects[i].second - 1]),
                   static_cast<int>(i & 0xFF));
    }
}

TEST(2, "deallocate returns the original upstream pointer") {
    Rng rng;
    ScatterUpstream upstream(1 << 18, rng);
    CacheAlignedWrapper wrapper(upstream);
    std::vector<void*> objects;
    for (int i = 0; i < 200; ++i) {
        objects.push_back(wrapper.allocate(static_cast<std::size_t>(rng.uniform(1, 300))));
    }
    CHECK_EQ(upstream.live_grants(), 200);
    rng.shuffle(objects);
    for (void* p : objects) {
        wrapper.deallocate(p);
    }
    CHECK_EQ(upstream.live_grants(), 0);
    CHECK_EQ(upstream.frees, 200);
}

TEST(2, "deallocating nullptr does nothing") {
    Rng rng;
    ScatterUpstream upstream(1 << 12, rng);
    CacheAlignedWrapper wrapper(upstream);
    wrapper.deallocate(nullptr);
    CHECK_EQ(upstream.frees, 0);
}

TEST(3, "every object spans the fewest possible cache lines") {
    Rng rng;
    AlignedBuffer heap(1 << 20);
    PackedAllocator allocator(heap.span());
    std::vector<std::pair<void*, std::size_t>> objects;
    for (int i = 0; i < 2000; ++i) {
        INFO("allocation ", i);
        const auto size = static_cast<std::size_t>(rng.uniform(1, 400));
        void* p = allocator.allocate(size);
        REQUIRE(p != nullptr);
        REQUIRE_EQ(reinterpret_cast<std::uintptr_t>(p) % 8, 0);
        REQUIRE_EQ(lines_spanned(p, size), (size + kCacheLine - 1) / kCacheLine);
        REQUIRE_EQ(allocator.size_of(p), size);
        objects.emplace_back(p, size);
    }
    // Objects, and the headers in front of them, must not overlap.
    std::vector<std::pair<std::uintptr_t, std::uintptr_t>> ranges;
    for (const auto& [p, size] : objects) {
        const auto start = reinterpret_cast<std::uintptr_t>(p);
        ranges.emplace_back(start - 8, start + size);
    }
    std::sort(ranges.begin(), ranges.end());
    for (std::size_t i = 0; i + 1 < ranges.size(); ++i) {
        INFO("range ", i);
        REQUIRE(ranges[i].second <= ranges[i + 1].first);
    }
}

TEST(3, "small objects share cache lines") {
    AlignedBuffer heap(1 << 17);
    PackedAllocator allocator(heap.span());
    for (int i = 0; i < 1000; ++i) {
        REQUIRE(allocator.allocate(24) != nullptr);
    }
    // Two per line: an 8-byte header and a 24-byte object fill 32 bytes exactly.
    CHECK_EQ(allocator.used_bytes(), 32'000);
}

TEST(3, "objects that would straddle a line start on the next one") {
    AlignedBuffer heap(1 << 12);
    PackedAllocator allocator(heap.span());
    void* first = allocator.allocate(40);
    REQUIRE(first != nullptr);
    void* second = allocator.allocate(40);
    REQUIRE(second != nullptr);
    CHECK_EQ(reinterpret_cast<std::uintptr_t>(second) % kCacheLine, 0);
    CHECK_EQ(lines_spanned(second, 40), 1);
}

TEST(3, "an unaligned heap still places line boundaries correctly") {
    AlignedBuffer raw(1 << 14);
    PackedAllocator allocator(raw.span().subspan(5 * 8));  // starts 40 bytes past a line
    for (int i = 0; i < 100; ++i) {
        INFO("allocation ", i);
        void* p = allocator.allocate(56);
        REQUIRE(p != nullptr);
        REQUIRE_EQ(lines_spanned(p, 56), 1);
    }
}

TEST(3, "an exhausted heap returns nullptr") {
    AlignedBuffer heap(128);
    PackedAllocator allocator(heap.span());
    CHECK(allocator.allocate(100) != nullptr);
    CHECK(allocator.allocate(100) == nullptr);
    CHECK(allocator.allocate(0) == nullptr);
}

namespace {

struct Layout {
    std::vector<std::uint64_t> addresses;
    std::vector<std::size_t> sizes;
};

// Uses allocation order, not real addresses, so results do not depend on heap layout.
Layout wrapped_layout(int count, std::size_t size) {
    Rng rng(1);
    ScatterUpstream upstream(static_cast<std::size_t>(count) * (size + 200) + 4096, rng);
    CacheAlignedWrapper wrapper(upstream);
    Layout layout;
    const auto* base = static_cast<std::byte*>(upstream.allocate(8));
    for (int i = 0; i < count; ++i) {
        auto* p = static_cast<std::byte*>(wrapper.allocate(size));
        layout.addresses.push_back(static_cast<std::uint64_t>(p - base));
        layout.sizes.push_back(size);
    }
    // Align the simulated addresses the way the real ones are aligned.
    const auto first = layout.addresses.front();
    for (auto& address : layout.addresses) {
        address = (address - first) / kCacheLine * kCacheLine;
    }
    return layout;
}

Layout packed_layout(int count, std::size_t size) {
    AlignedBuffer heap(static_cast<std::size_t>(count) * (size + 64) + 4096);
    PackedAllocator allocator(heap.span());
    Layout layout;
    for (int i = 0; i < count; ++i) {
        auto* p = static_cast<std::byte*>(allocator.allocate(size));
        REQUIRE(p != nullptr);
        layout.addresses.push_back(static_cast<std::uint64_t>(p - heap.span().data()));
        layout.sizes.push_back(size);
    }
    return layout;
}

Layout sequential_layout(int count, std::size_t size) {
    Layout layout;
    for (int i = 0; i < count; ++i) {
        layout.addresses.push_back(static_cast<std::uint64_t>(i) * size);
        layout.sizes.push_back(size);
    }
    return layout;
}

std::vector<std::size_t> random_order(int objects, int accesses) {
    Rng rng;
    std::vector<std::size_t> order(static_cast<std::size_t>(accesses));
    for (auto& i : order) {
        i = static_cast<std::size_t>(rng.uniform(0, objects - 1));
    }
    return order;
}

}  // namespace

TEST(4, "lines_per_access averages over the access order") {
    const std::vector<std::uint64_t> addresses{0, 60};
    const std::vector<std::size_t> sizes{8, 8};  // the second object straddles a line
    const std::vector<std::size_t> order{0, 1, 1, 0};
    Cache cache(4096, 64, 4);
    const PlacementStats stats = measure_placement(addresses, sizes, order, cache);
    CHECK_NEAR(stats.lines_per_access, 1.5, 1e-12);
    CHECK_EQ(stats.misses, 2);  // lines 0 and 1 miss once each; the rest are hits
}

TEST(4, "misses counted are only those added by this run") {
    Cache cache(4096, 64, 4);
    cache.access(0);
    cache.access(64);
    CHECK_EQ(cache.misses(), 2);
    const std::vector<std::uint64_t> addresses{0, 64, 128};
    const std::vector<std::size_t> sizes{8, 8, 8};
    const std::vector<std::size_t> order{0, 1, 2};
    const PlacementStats stats = measure_placement(addresses, sizes, order, cache);
    CHECK_EQ(stats.misses, 1);
}

TEST(4, "aligned and packed objects each touch one line, a naive layout more") {
    const int count = 4096;
    const auto order = random_order(count, 50'000);
    Cache c1(1 << 21, 64, 16), c2(1 << 21, 64, 16), c3(1 << 21, 64, 16);
    const Layout wrapped = wrapped_layout(count, 24);
    const Layout packed = packed_layout(count, 24);
    const Layout naive = sequential_layout(count, 24);
    const PlacementStats w = measure_placement(wrapped.addresses, wrapped.sizes, order, c1);
    const PlacementStats p = measure_placement(packed.addresses, packed.sizes, order, c2);
    const PlacementStats n = measure_placement(naive.addresses, naive.sizes, order, c3);
    CHECK_NEAR(w.lines_per_access, 1.0, 1e-12);
    CHECK_NEAR(p.lines_per_access, 1.0, 1e-12);
    CHECK_GT(n.lines_per_access, 1.2);
}

TEST(4, "padding every object wastes the cache: packed misses far less") {
    const int count = 4096;
    const auto order = random_order(count, 120'000);
    // Packed objects fill exactly half the lines the aligned ones need.
    Cache small_aligned(1 << 17, 64, 16), small_packed(1 << 17, 64, 16);
    const Layout wrapped = wrapped_layout(count, 24);
    const Layout packed = packed_layout(count, 24);
    const PlacementStats w = measure_placement(wrapped.addresses, wrapped.sizes, order, small_aligned);
    const PlacementStats p = measure_placement(packed.addresses, packed.sizes, order, small_packed);
    INFO("aligned misses ", w.misses, ", packed misses ", p.misses);
    CHECK_GT(w.misses, 3 * p.misses);
}

TEST(4, "when every object fits a cache line anyway, both layouts converge") {
    const int count = 2000;
    const auto order = random_order(count, 40'000);
    Cache big_aligned(1 << 21, 64, 16), big_packed(1 << 21, 64, 16);
    const Layout wrapped = wrapped_layout(count, 56);
    const Layout packed = packed_layout(count, 56);
    const PlacementStats w = measure_placement(wrapped.addresses, wrapped.sizes, order, big_aligned);
    const PlacementStats p = measure_placement(packed.addresses, packed.sizes, order, big_packed);
    CHECK_EQ(w.misses, static_cast<std::uint64_t>(count));
    CHECK_LE(p.misses, static_cast<std::uint64_t>(count) + 5);
    CHECK_NEAR(w.lines_per_access, 1.0, 1e-12);
    CHECK_NEAR(p.lines_per_access, 1.0, 1e-12);
}
