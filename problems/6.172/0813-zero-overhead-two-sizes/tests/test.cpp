#include <algorithm>
#include <cstring>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct AlignedBuffer {
    explicit AlignedBuffer(std::size_t bytes)
        : size(bytes), memory(::operator new(bytes, std::align_val_t{64})) {}
    ~AlignedBuffer() { ::operator delete(memory, std::align_val_t{64}); }
    std::span<std::byte> span() { return {static_cast<std::byte*>(memory), size}; }
    std::size_t size;
    void* memory;
};

std::size_t offset_of(void* p, AlignedBuffer& heap) {
    return static_cast<std::size_t>(static_cast<std::byte*>(p) - heap.span().data());
}

}  // namespace

TEST(1, "blocks are 64-byte aligned and exactly 64 bytes apart") {
    AlignedBuffer heap(64 * 8);
    FixedAligned64 allocator(heap.span());
    for (std::size_t i = 0; i < 8; ++i) {
        void* p = allocator.allocate();
        REQUIRE(p != nullptr);
        CHECK_EQ(reinterpret_cast<std::uintptr_t>(p) % 64, 0);
        CHECK_EQ(offset_of(p, heap), i * 64);
    }
    CHECK_EQ(allocator.used_bytes(), 64 * 8);
}

TEST(1, "the heap runs out exactly when it should") {
    AlignedBuffer heap(64 * 3);
    FixedAligned64 allocator(heap.span());
    CHECK(allocator.allocate() != nullptr);
    CHECK(allocator.allocate() != nullptr);
    CHECK(allocator.allocate() != nullptr);
    CHECK(allocator.allocate() == nullptr);
    CHECK_EQ(allocator.used_bytes(), 64 * 3);
}

TEST(1, "freed blocks are reused LIFO without growing the heap") {
    AlignedBuffer heap(64 * 4);
    FixedAligned64 allocator(heap.span());
    void* a = allocator.allocate();
    void* b = allocator.allocate();
    allocator.deallocate(a);
    allocator.deallocate(b);
    CHECK(allocator.allocate() == b);
    CHECK(allocator.allocate() == a);
    CHECK_EQ(allocator.used_bytes(), 64 * 2);
}

TEST(1, "deallocating nullptr does nothing") {
    AlignedBuffer heap(64 * 2);
    FixedAligned64 allocator(heap.span());
    allocator.deallocate(nullptr);
    CHECK_EQ(allocator.used_bytes(), 0);
}

TEST(1, "there is no per-object space overhead at all") {
    AlignedBuffer heap(64 * 1024);
    FixedAligned64 allocator(heap.span());
    std::vector<void*> blocks;
    for (int i = 0; i < 1024; ++i) {
        void* p = allocator.allocate();
        REQUIRE(p != nullptr);
        std::memset(p, i & 0xFF, 64);  // the whole block belongs to the caller
        blocks.push_back(p);
    }
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        REQUIRE_EQ(static_cast<int>(static_cast<unsigned char*>(blocks[i])[63]),
                   static_cast<int>(i & 0xFF));
    }
    CHECK_EQ(allocator.used_bytes(), 64 * 1024);
}

TEST(2, "SmartPtr is no bigger than a raw pointer") {
    CHECK(!SmartPtr{});
    CHECK_EQ(sizeof(SmartPtr), sizeof(void*));
}

TEST(2, "a null SmartPtr is empty") {
    const SmartPtr empty;
    CHECK(!empty);
    CHECK(empty.get() == nullptr);
}

TEST(2, "the tag is stored and stripped") {
    alignas(64) unsigned char line[128];
    const SmartPtr small = SmartPtr::make(line, true);
    const SmartPtr large = SmartPtr::make(line + 64, false);
    CHECK(small);
    CHECK(large);
    CHECK(small.is_small());
    CHECK(!large.is_small());
    CHECK(small.get() == line);
    CHECK(large.get() == line + 64);
    const SmartPtr mid = SmartPtr::make(line + 32, true);
    CHECK(mid.is_small());
    CHECK(mid.get() == line + 32);
    CHECK_EQ(reinterpret_cast<std::uintptr_t>(mid.get()) % 32, 0);
}

TEST(2, "equality sees both the address and the tag") {
    alignas(64) unsigned char line[128];
    CHECK(SmartPtr::make(line, true) == SmartPtr::make(line, true));
    CHECK(!(SmartPtr::make(line, true) == SmartPtr::make(line, false)));
    CHECK(!(SmartPtr::make(line, true) == SmartPtr::make(line + 32, true)));
}

TEST(3, "sizes pick the right class and the tag reports it") {
    AlignedBuffer heap(4096);
    SmartAllocator allocator(heap.span());
    CHECK(allocator.allocate(1).is_small());
    CHECK(allocator.allocate(32).is_small());
    CHECK(!allocator.allocate(33).is_small());
    CHECK(!allocator.allocate(64).is_small());
}

TEST(3, "zero and oversized requests return an empty pointer") {
    AlignedBuffer heap(4096);
    SmartAllocator allocator(heap.span());
    CHECK(!allocator.allocate(0));
    CHECK(!allocator.allocate(65));
    CHECK_EQ(allocator.used_bytes(), 0);
}

TEST(3, "small blocks start on a half line and large on a line") {
    AlignedBuffer heap(1 << 15);
    SmartAllocator allocator(heap.span());
    Rng rng;
    for (int i = 0; i < 500; ++i) {
        INFO("allocation ", i);
        const bool small = rng.coin();
        const SmartPtr p = allocator.allocate(small ? 20 : 60);
        REQUIRE(static_cast<bool>(p));
        const auto offset = offset_of(p.get(), heap);
        REQUIRE_EQ(offset % 32, 0);
        if (!p.is_small()) {
            REQUIRE_EQ(offset % 64, 0);
        }
    }
}

TEST(3, "deallocation needs no size argument and reuses by tag") {
    AlignedBuffer heap(4096);
    SmartAllocator allocator(heap.span());
    const SmartPtr small = allocator.allocate(16);
    const SmartPtr large = allocator.allocate(50);
    allocator.deallocate(small);
    allocator.deallocate(large);
    const std::size_t used = allocator.used_bytes();
    CHECK(allocator.allocate(32) == small);  // comes back as a small block
    CHECK(allocator.allocate(64) == large);  // and the large one from its own list
    CHECK_EQ(allocator.used_bytes(), used);
}

TEST(3, "the half line skipped by a large allocation serves the next small one") {
    AlignedBuffer heap(4096);
    SmartAllocator allocator(heap.span());
    const SmartPtr s1 = allocator.allocate(32);      // [0, 32)
    const SmartPtr l1 = allocator.allocate(64);      // [64, 128), skipping [32, 64)
    const SmartPtr s2 = allocator.allocate(32);      // must be [32, 64), not a new bump
    CHECK_EQ(offset_of(s1.get(), heap), 0);
    CHECK_EQ(offset_of(l1.get(), heap), 64);
    CHECK_EQ(offset_of(s2.get(), heap), 32);
    CHECK_EQ(allocator.used_bytes(), 128);
}

TEST(3, "the small list is used before bumping, the large list too") {
    AlignedBuffer heap(1024);
    SmartAllocator allocator(heap.span());
    const SmartPtr s = allocator.allocate(32);
    allocator.deallocate(s);
    CHECK(allocator.allocate(32) == s);
    CHECK_EQ(allocator.used_bytes(), 32);
    const SmartPtr l = allocator.allocate(64);
    allocator.deallocate(l);
    CHECK(allocator.allocate(64) == l);
}

TEST(3, "an exhausted heap returns an empty pointer") {
    AlignedBuffer heap(64);
    SmartAllocator allocator(heap.span());
    CHECK(static_cast<bool>(allocator.allocate(32)));
    CHECK(static_cast<bool>(allocator.allocate(32)));
    CHECK(!allocator.allocate(32));
    CHECK(!allocator.allocate(64));
    CHECK_EQ(allocator.used_bytes(), 64);
}

TEST(3, "a large allocation that cannot fit leaves the heap state alone") {
    AlignedBuffer heap(96);
    SmartAllocator allocator(heap.span());
    CHECK(static_cast<bool>(allocator.allocate(32)));   // [0, 32)
    CHECK(!allocator.allocate(64));                     // only half a line left past 64
    CHECK_EQ(allocator.used_bytes(), 32);               // nothing was committed
    CHECK(static_cast<bool>(allocator.allocate(32)));   // [32, 64) is still available
}

TEST(3, "random allocation and free preserve every live object") {
    Rng rng;
    AlignedBuffer heap(1 << 16);
    SmartAllocator allocator(heap.span());
    struct Live {
        SmartPtr pointer;
        unsigned char tag;
    };
    std::vector<Live> live;
    for (int step = 0; step < 50'000; ++step) {
        INFO("step ", step);
        if (live.empty() || (live.size() < 500 && rng.coin(0.55))) {
            const bool small = rng.coin();
            const SmartPtr p = allocator.allocate(small ? 32 : 64);
            REQUIRE(static_cast<bool>(p));
            REQUIRE_EQ(p.is_small(), small);
            const auto tag = static_cast<unsigned char>(rng.uniform(0, 255));
            std::memset(p.get(), tag, small ? 32 : 64);
            live.push_back({p, tag});
        } else {
            const auto i = static_cast<std::size_t>(
                rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            auto* bytes = static_cast<unsigned char*>(live[i].pointer.get());
            const int size = live[i].pointer.is_small() ? 32 : 64;
            REQUIRE_EQ(static_cast<int>(bytes[0]), static_cast<int>(live[i].tag));
            REQUIRE_EQ(static_cast<int>(bytes[size - 1]), static_cast<int>(live[i].tag));
            allocator.deallocate(live[i].pointer);
            live[i] = live.back();
            live.pop_back();
        }
    }
    CHECK_LE(allocator.used_bytes(), 1 << 16);
}
