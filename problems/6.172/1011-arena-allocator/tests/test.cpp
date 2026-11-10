#include <sys/mman.h>
#include <unistd.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <type_traits>
#include <vector>

#include "alloc_counter.hpp"
#include "practice.hpp"
#include "replay.hpp"
#include "solution.hpp"
#include "trace.hpp"

using practice::Rng;

namespace {

constexpr std::size_t kHeapBytes = std::size_t{1} << 28;

std::uint64_t page_size() { return static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE)); }

bool is_aligned(const void* p) { return reinterpret_cast<std::uintptr_t>(p) % 16 == 0; }

void fill(void* p, std::size_t n, unsigned char value) { std::memset(p, value, n); }

bool holds(const void* p, std::size_t n, unsigned char value) {
    const auto* bytes = static_cast<const unsigned char*>(p);
    return std::all_of(bytes, bytes + n, [value](unsigned char b) { return b == value; });
}

// ---------------------------------------------------------------------------------------------
// The public block layout from Stage 2 on: the word just before the payload is the header.
// ---------------------------------------------------------------------------------------------

std::uint64_t header_of(const void* payload) {
    std::uint64_t word;
    std::memcpy(&word, static_cast<const std::byte*>(payload) - 8, sizeof word);
    return word;
}

void set_header(void* payload, std::uint64_t word) {
    std::memcpy(static_cast<std::byte*>(payload) - 8, &word, sizeof word);
}

std::uint64_t size_bits(std::uint64_t header) { return header & ~std::uint64_t{15}; }
std::size_t block_size(const void* payload) { return size_bits(header_of(payload)); }
bool allocated_bit(const void* payload) { return (header_of(payload) & 1) != 0; }

std::byte* next_payload(void* payload) { return static_cast<std::byte*>(payload) + block_size(payload); }

struct WalkedBlock {
    std::byte* payload;
    std::size_t size;
    bool allocated;
};

// Walks the blocks in address order from a block known to be real, independent of check().
template <typename A>
std::vector<WalkedBlock> walk_from(const A& a, void* first_payload) {
    std::vector<WalkedBlock> blocks;
    auto* payload = static_cast<std::byte*>(first_payload);
    const std::byte* end = a.arena().base() + a.arena().used();
    while (payload < end) {
        const std::size_t size = block_size(payload);
        if (size == 0) {
            break;  // an end-of-heap sentinel is allowed
        }
        if (size < 32 || size % 16 != 0 || static_cast<std::size_t>(end - (payload - 8)) < size) {
            INFO("offset ", (payload - 8) - a.arena().base(), " size ", size);
            CHECK(false);
            break;
        }
        blocks.push_back({payload, size, allocated_bit(payload)});
        payload += size;
    }
    return blocks;
}

// ---------------------------------------------------------------------------------------------
// The contract every allocator shares.
// ---------------------------------------------------------------------------------------------

template <typename A>
void null_and_zero_are_handled() {
    A a(kHeapBytes);
    CHECK(a.malloc(0) == nullptr);
    a.free(nullptr);
    void* p = a.realloc(nullptr, 64);
    REQUIRE(p != nullptr);
    CHECK(is_aligned(p));
    fill(p, 64, 0xCC);
    CHECK(a.realloc(p, 0) == nullptr);
    CHECK(a.check());
}

template <typename A>
void pointers_are_aligned_and_inside_the_heap() {
    A a(kHeapBytes);
    Rng rng;
    for (int i = 0; i < 2000; ++i) {
        const auto size = static_cast<std::size_t>(rng.uniform(1, 5000));
        void* p = a.malloc(size);
        REQUIRE(p != nullptr);
        CHECK(is_aligned(p));
        const auto* begin = static_cast<const std::byte*>(p);
        const std::byte* base = a.arena().base();
        CHECK_GE(begin - base, 0);
        CHECK_LE(static_cast<std::size_t>(begin - base) + size, a.heap_bytes());
    }
    CHECK(a.check());
}

template <typename A>
void reallocation_preserves_data() {
    A a(kHeapBytes);
    void* other = a.malloc(32);  // keeps the block below from sitting at the very start
    REQUIRE(other != nullptr);
    void* p = a.malloc(100);
    REQUIRE(p != nullptr);
    fill(p, 100, 0x5A);

    void* grown = a.realloc(p, 300);
    REQUIRE(grown != nullptr);
    CHECK(is_aligned(grown));
    CHECK(holds(grown, 100, 0x5A));
    fill(grown, 300, 0x6B);

    void* same = a.realloc(grown, 300);
    REQUIRE(same != nullptr);
    CHECK(holds(same, 300, 0x6B));

    void* shrunk = a.realloc(same, 20);
    REQUIRE(shrunk != nullptr);
    CHECK(holds(shrunk, 20, 0x6B));
    CHECK(a.check());
}

template <typename A>
void running_out_of_space_returns_null() {
    A a(1 << 20);
    std::vector<void*> blocks;
    while (void* p = a.malloc(4096)) {
        fill(p, 4096, 0x11);
        blocks.push_back(p);
        REQUIRE_EQ(blocks.size() < 1000, true);
    }
    CHECK_GT(blocks.size(), 200);
    CHECK_LE(a.heap_bytes(), a.arena().capacity());
    for (void* p : blocks) {
        CHECK(holds(p, 4096, 0x11));
    }
    CHECK(a.malloc(a.arena().capacity()) == nullptr);
    CHECK(a.check());
}

template <typename A>
void absurd_sizes_are_refused_cleanly() {
    A a(1 << 20);
    const std::size_t huge = std::numeric_limits<std::size_t>::max();
    CHECK(a.malloc(huge) == nullptr);
    CHECK(a.malloc(huge - 7) == nullptr);
    CHECK(a.malloc(huge / 2) == nullptr);
    void* p = a.malloc(64);
    REQUIRE(p != nullptr);
    fill(p, 64, 0x77);
    CHECK(a.realloc(p, huge) == nullptr);
    CHECK(a.realloc(p, huge - 7) == nullptr);
    CHECK(holds(p, 64, 0x77));
    CHECK(a.check());
    void* q = a.malloc(128);
    CHECK(q != nullptr);
    CHECK(a.check());
}

template <typename A>
void failed_realloc_keeps_the_old_block() {
    A a(1 << 20);
    void* p = a.malloc(4096);
    REQUIRE(p != nullptr);
    fill(p, 4096, 0x4C);
    void* blocker = a.malloc(8192);  // stops the first block from simply growing
    REQUIRE(blocker != nullptr);
    while (a.malloc(16 * 1024) != nullptr) {
    }
    // Plausible size, but the remaining room cannot hold it.
    CHECK(a.realloc(p, 512 * 1024) == nullptr);
    CHECK(holds(p, 4096, 0x4C));
    if constexpr (!std::is_same_v<A, BumpAllocator>) {
        CHECK(allocated_bit(p));  // the block must still be allocated, not quietly freed
    }
    CHECK(a.check());
    a.free(p);
    CHECK(a.check());
}

template <typename A>
void all_shapes_validate(std::size_t scale, std::size_t check_every) {
    for (const trace::Trace& t : trace::all_shapes(42, scale)) {
        A a(kHeapBytes);
        const trace::Report report = trace::validate(a, t, check_every);
        INFO("trace ", t.name, " utilization ", report.utilization());
        CHECK_EQ(report.error, "");
        CHECK(a.check());
    }
}

template <typename A>
void random_ops_match_a_model() {
    A a(kHeapBytes);
    Rng rng;
    struct Tracked {
        void* p;
        std::size_t size;
        unsigned char value;
    };
    std::vector<Tracked> live;
    for (int step = 0; step < 6000; ++step) {
        INFO("step ", step);
        const auto action = rng.uniform(0, 2);
        const auto size = static_cast<std::size_t>(rng.uniform(1, 2048));
        const auto value = static_cast<unsigned char>(rng.uniform(0, 255));
        if (action == 0 || live.empty()) {
            void* p = a.malloc(size);
            REQUIRE(p != nullptr);
            fill(p, size, value);
            live.push_back({p, size, value});
        } else if (action == 1) {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            CHECK(holds(live[index].p, live[index].size, live[index].value));
            a.free(live[index].p);
            live[index] = live.back();
            live.pop_back();
        } else {
            const auto index = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            Tracked& block = live[index];
            void* q = a.realloc(block.p, size);
            REQUIRE(q != nullptr);
            CHECK(holds(q, std::min(size, block.size), block.value));
            fill(q, size, value);
            block = {q, size, value};
        }
        REQUIRE(a.check());
    }
}

// ---------------------------------------------------------------------------------------------
// Behaviour shared by the two allocators that actually reuse memory.
// ---------------------------------------------------------------------------------------------

template <typename A>
void headers_follow_the_published_layout() {
    A a(kHeapBytes);
    Rng rng;
    for (int i = 0; i < 500; ++i) {
        const auto request = static_cast<std::size_t>(rng.uniform(1, 3000));
        void* p = a.malloc(request);
        REQUIRE(p != nullptr);
        INFO("request ", request);
        CHECK(allocated_bit(p));
        CHECK_EQ(block_size(p) % 16, 0);
        CHECK_GE(block_size(p), request + 8);
    }
}

template <typename A>
void every_block_is_reachable_by_walking() {
    A a(kHeapBytes);
    Rng rng;
    std::map<std::byte*, std::size_t> live;
    for (int i = 0; i < 2500; ++i) {
        const bool allocate = live.size() < 5 || rng.coin(0.55);
        if (allocate) {
            const auto request = static_cast<std::size_t>(rng.uniform(1, 400));
            auto* p = static_cast<std::byte*>(a.malloc(request));
            REQUIRE(p != nullptr);
            live[p] = request;
        } else {
            auto it = live.begin();
            std::advance(it, rng.uniform(1, static_cast<std::int64_t>(live.size()) - 1));
            a.free(it->first);
            live.erase(it);
        }
    }
    // Walks from the lowest live block; anything below it can only be free space or a sentinel.
    const auto blocks = walk_from(a, live.begin()->first);
    REQUIRE(blocks.size() > 0);
    std::size_t seen_live = 0;
    for (std::size_t i = 0; i < blocks.size(); ++i) {
        INFO("block ", i);
        if (blocks[i].allocated) {
            CHECK(live.count(blocks[i].payload) == 1);
            ++seen_live;
        } else if (i > 0) {
            CHECK(blocks[i - 1].allocated);  // no two free blocks in a row
        }
    }
    CHECK_EQ(seen_live, live.size());
}

template <typename A>
struct Neighbours {
    A heap;
    void* a;
    void* b;
    void* c;
    void* guard;

    Neighbours() : heap(kHeapBytes) {
        a = heap.malloc(256);
        b = heap.malloc(256);
        c = heap.malloc(256);
        guard = heap.malloc(256);
        REQUIRE(a != nullptr && b != nullptr && c != nullptr && guard != nullptr);
        REQUIRE(next_payload(a) == b);
        REQUIRE(next_payload(b) == c);
        REQUIRE(next_payload(c) == guard);
    }
};

template <typename A>
void coalesces_with_the_next_block() {
    Neighbours<A> n;
    const std::size_t merged = block_size(n.a) + block_size(n.b);
    n.heap.free(n.b);
    n.heap.free(n.a);
    CHECK_EQ(block_size(n.a), merged);
    CHECK(!allocated_bit(n.a));
    CHECK(n.heap.check());
}

template <typename A>
void coalesces_with_the_previous_block() {
    Neighbours<A> n;
    const std::size_t merged = block_size(n.a) + block_size(n.b);
    n.heap.free(n.a);
    n.heap.free(n.b);
    CHECK_EQ(block_size(n.a), merged);
    CHECK(n.heap.check());
}

template <typename A>
void coalesces_with_both_neighbours() {
    Neighbours<A> n;
    const std::size_t merged = block_size(n.a) + block_size(n.b) + block_size(n.c);
    n.heap.free(n.a);
    n.heap.free(n.c);
    n.heap.free(n.b);
    CHECK_EQ(block_size(n.a), merged);
    CHECK(n.heap.check());
}

template <typename A>
void freed_memory_is_reused_before_growing() {
    Neighbours<A> n;
    const std::size_t heap_before = n.heap.heap_bytes();
    n.heap.free(n.b);
    void* again = n.heap.malloc(256);
    REQUIRE(again != nullptr);
    CHECK_EQ(n.heap.heap_bytes(), heap_before);
    CHECK(n.heap.check());
}

template <typename A>
void merged_holes_serve_a_larger_request() {
    Neighbours<A> n;
    const std::size_t heap_before = n.heap.heap_bytes();
    n.heap.free(n.a);
    n.heap.free(n.b);
    n.heap.free(n.c);
    void* big = n.heap.malloc(3 * 256);
    REQUIRE(big != nullptr);
    fill(big, 3 * 256, 0x42);
    CHECK_EQ(n.heap.heap_bytes(), heap_before);
    CHECK(n.heap.check());
}

template <typename A>
void splitting_keeps_the_rest_usable() {
    A a(kHeapBytes);
    void* big = a.malloc(8192);
    void* guard = a.malloc(64);
    REQUIRE(big != nullptr && guard != nullptr);
    const std::size_t heap_before = a.heap_bytes();
    a.free(big);
    for (int i = 0; i < 16; ++i) {
        void* piece = a.malloc(256);
        REQUIRE(piece != nullptr);
        fill(piece, 256, static_cast<unsigned char>(i));
        REQUIRE(a.check());
    }
    CHECK_EQ(a.heap_bytes(), heap_before);
}

template <typename A>
void realloc_shrinks_and_grows_in_place_when_it_can() {
    A a(kHeapBytes);
    void* p = a.malloc(2000);
    void* neighbour = a.malloc(2000);
    void* guard = a.malloc(64);
    REQUIRE(p != nullptr && neighbour != nullptr && guard != nullptr);
    fill(p, 2000, 0x31);

    // Shrinking never moves the block.
    void* shrunk = a.realloc(p, 512);
    CHECK(shrunk == p);
    CHECK(holds(shrunk, 512, 0x31));

    // The trimmed tail plus a freed neighbour are room to grow into without moving.
    a.free(neighbour);
    void* grown = a.realloc(shrunk, 3000);
    CHECK(grown == p);
    CHECK(holds(grown, 512, 0x31));
    CHECK(a.check());
}

template <typename A>
void the_last_block_grows_without_moving() {
    A a(kHeapBytes);
    void* first = a.malloc(64);
    void* p = a.malloc(1024);
    REQUIRE(first != nullptr && p != nullptr);
    fill(p, 1024, 0x2D);
    const std::size_t heap_before = a.heap_bytes();
    void* grown = a.realloc(p, 1 << 16);
    CHECK(grown == p);
    CHECK(holds(grown, 1024, 0x2D));
    CHECK_LE(a.heap_bytes(), heap_before + (1 << 16) + 64);
    CHECK(a.check());
}

template <typename A>
void a_free_tail_is_reused_when_the_heap_grows() {
    A a(kHeapBytes);
    void* first = a.malloc(64);
    void* last = a.malloc(4096);
    REQUIRE(first != nullptr && last != nullptr);
    a.free(last);
    const std::size_t heap_before = a.heap_bytes();
    void* bigger = a.malloc(8192);
    REQUIRE(bigger != nullptr);
    // Growing by the whole request would add 8192 bytes; the free tail already covers 4096.
    CHECK_LE(a.heap_bytes(), heap_before + 8192 - 4000);
    CHECK(a.check());
}

template <typename A>
void corruption_of_a_size_is_noticed() {
    {
        Neighbours<A> n;
        CHECK(n.heap.check());
        set_header(n.b, header_of(n.b) & 15);  // size becomes zero
        CHECK(!n.heap.check());
    }
    {
        Neighbours<A> n;
        set_header(n.b, (header_of(n.b) & 15) | (std::uint64_t{1} << 40));  // far past the heap end
        CHECK(!n.heap.check());
    }
}

template <typename A>
void a_derailed_walk_is_noticed() {
    Neighbours<A> n;
    fill(n.c, 256, 0);
    set_header(n.b, header_of(n.b) + 16);  // the next "header" now lands inside c's zeroed payload
    CHECK(!n.heap.check());
}

template <typename A>
void uncoalesced_free_neighbours_are_noticed() {
    Neighbours<A> n;
    n.heap.free(n.a);
    n.heap.free(n.c);
    CHECK(n.heap.check());
    set_header(n.b, header_of(n.b) & ~std::uint64_t{1});  // b now looks free, between two free blocks
    CHECK(!n.heap.check());
}

template <typename A>
void construction_is_cheap() {
    A a(kHeapBytes);
    CHECK_LE(a.heap_bytes(), 4096);
    CHECK_LE(sizeof(A), 8192);
}

template <typename A>
void scale_trace(std::size_t scale) {
    all_shapes_validate<A>(scale, 512);
}

// ---------------------------------------------------------------------------------------------
// The validator itself is tested first: these allocators each carry one planted defect.
// ---------------------------------------------------------------------------------------------

class BufferView {
public:
    BufferView(std::byte* base, std::size_t used) : base_(base), used_(used) {}
    std::byte* base() const { return base_; }
    std::size_t used() const { return used_; }

private:
    std::byte* base_;
    std::size_t used_;
};

class TestHeap {
public:
    TestHeap() : storage_(1 << 24) {}

    void* malloc(std::size_t size) {
        offset_ = (offset_ + 15) & ~std::size_t{15};
        void* result = storage_.data() + offset_;
        offset_ += size == 0 ? 16 : size;
        return result;
    }
    void free(void*) {}
    void* realloc(void* p, std::size_t size) {
        void* result = malloc(size);
        std::memcpy(result, p, size);
        return result;
    }
    bool check() const { return true; }
    std::size_t heap_bytes() const { return offset_; }
    BufferView arena() const { return BufferView(storage_.data(), offset_); }

protected:
    mutable std::vector<std::byte> storage_;
    std::size_t offset_ = 0;
};

class OverlappingHeap : public TestHeap {
public:
    void* malloc(std::size_t size) {
        void* p = TestHeap::malloc(size);
        if (++calls_ == 50) {
            return previous_;  // hands the same memory out twice
        }
        previous_ = p;
        return p;
    }

private:
    int calls_ = 0;
    void* previous_ = nullptr;
};

class MisalignedHeap : public TestHeap {
public:
    void* malloc(std::size_t size) {
        void* p = TestHeap::malloc(size + 8);
        return static_cast<std::byte*>(p) + 8;
    }
};

class ForgetfulReallocHeap : public TestHeap {
public:
    void* realloc(void* p, std::size_t size) { return malloc(size); }
};

class TruncatingReallocHeap : public TestHeap {
public:
    void* realloc(void* p, std::size_t size) {
        void* result = malloc(size);
        std::memcpy(result, p, size / 2);
        return result;
    }
};

class SpoilingFreeHeap : public TestHeap {
public:
    void free(void* p) { static_cast<std::byte*>(p)[-16] = std::byte{0xFF}; }
};

class EscapingHeap : public TestHeap {
public:
    void* malloc(std::size_t size) {
        void* p = TestHeap::malloc(size);
        offset_ = offset_ > 4096 ? offset_ - 4096 : offset_;
        return p;
    }
};

class OperatorNewHeap : public TestHeap {
public:
    void* malloc(std::size_t size) {
        table_.push_back(size);
        return TestHeap::malloc(size);
    }

private:
    std::vector<std::size_t> table_;
};

class LyingCheckHeap : public TestHeap {
public:
    bool check() const { return false; }
};

// Two fixed traces that exercise the defects above.
trace::Trace meta_trace() {
    trace::Builder b("meta", 7);
    std::vector<std::uint32_t> slots;
    for (int i = 0; i < 200; ++i) {
        slots.push_back(b.malloc_block(b.rng.range(16, 256)));
    }
    for (int i = 0; i < 100; i += 2) {
        b.realloc_block(slots[static_cast<std::size_t>(i)], 400);
    }
    for (std::uint32_t slot : slots) {
        b.free_block(slot);
    }
    return b.finish();
}

template <typename H>
std::string complaint_about() {
    H heap;
    return trace::validate(heap, meta_trace(), 1).error;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Stage 1 — validator, arena, bump allocator
// ---------------------------------------------------------------------------------------------

TEST(1, "the validator accepts a correct allocator") { CHECK_EQ(complaint_about<TestHeap>(), ""); }

TEST(1, "the validator catches blocks that overlap") {
    CHECK(complaint_about<OverlappingHeap>().find("overlaps") != std::string::npos);
}

TEST(1, "the validator catches misaligned pointers") {
    CHECK(complaint_about<MisalignedHeap>().find("misaligned") != std::string::npos);
}

TEST(1, "the validator catches realloc that drops the old contents") {
    CHECK(complaint_about<ForgetfulReallocHeap>().find("lost data") != std::string::npos);
    CHECK(complaint_about<TruncatingReallocHeap>().find("lost data") != std::string::npos);
}

TEST(1, "the validator catches free corrupting a neighbour") {
    CHECK(complaint_about<SpoilingFreeHeap>().find("lost data") != std::string::npos);
}

TEST(1, "the validator catches blocks outside the heap") {
    CHECK(complaint_about<EscapingHeap>().find("outside") != std::string::npos);
}

TEST(1, "the validator catches an allocator using the C++ heap for metadata") {
    CHECK(complaint_about<OperatorNewHeap>().find("operator new") != std::string::npos);
}

TEST(1, "the validator reports a failing heap checker") {
    CHECK(complaint_about<LyingCheckHeap>().find("check()") != std::string::npos);
}

TEST(1, "generated traces are reproducible and fairly sized") {
    const auto first = trace::all_shapes(9, 1);
    const auto second = trace::all_shapes(9, 1);
    REQUIRE_EQ(first.size(), second.size());
    for (std::size_t i = 0; i < first.size(); ++i) {
        INFO("trace ", first[i].name);
        CHECK_EQ(first[i].ops.size(), second[i].ops.size());
        CHECK_GT(first[i].ops.size(), 1000);
        CHECK_GT(first[i].peak_live, 0);
        CHECK(first[i].peak_live <= first[i].total_requested);
    }
}

TEST(1, "an arena rounds its capacity up to whole pages") {
    Arena one_byte(1);
    CHECK_EQ(one_byte.capacity(), page_size());
    Arena exact(page_size() * 3);
    CHECK_EQ(exact.capacity(), page_size() * 3);
    Arena zero(0);
    CHECK(zero.capacity() >= page_size());
    CHECK(zero.base() != nullptr);
}

TEST(1, "grow hands out consecutive writable bytes") {
    Arena arena(1 << 20);
    CHECK_EQ(arena.used(), 0);
    std::byte* first = arena.grow(100);
    std::byte* second = arena.grow(50);
    REQUIRE(first == arena.base());
    REQUIRE(second == first + 100);
    CHECK_EQ(arena.used(), 150);
    std::memset(first, 0xAA, 150);
    CHECK(holds(first, 150, 0xAA));
}

TEST(1, "grow past the capacity returns nullptr and changes nothing") {
    Arena arena(page_size());
    CHECK(arena.grow(page_size()) != nullptr);
    CHECK(arena.grow(1) == nullptr);
    CHECK_EQ(arena.used(), page_size());
    CHECK(arena.grow(std::numeric_limits<std::size_t>::max()) == nullptr);
    CHECK_EQ(arena.used(), page_size());
}

TEST(1, "grow of zero bytes returns the current end") {
    Arena arena(page_size());
    arena.grow(40);
    CHECK(arena.grow(0) == arena.base() + 40);
    CHECK_EQ(arena.used(), 40);
}

TEST(1, "moving an arena transfers the mapping") {
    Arena source(page_size());
    std::byte* block = source.grow(64);
    std::memset(block, 0x7E, 64);
    std::byte* base = source.base();

    Arena moved(std::move(source));
    CHECK(moved.base() == base);
    CHECK_EQ(moved.used(), 64);
    CHECK(source.base() == nullptr);  // NOLINT: moved-from state is part of this class's contract
    CHECK_EQ(source.used(), 0);
    CHECK_EQ(source.capacity(), 0);
    CHECK(holds(block, 64, 0x7E));

    Arena target(page_size());
    target = std::move(moved);
    CHECK(target.base() == base);
    CHECK(moved.base() == nullptr);  // NOLINT
    CHECK(holds(target.base(), 64, 0x7E));
}

TEST(1, "destroying an arena unmaps its pages") {
    void* mapped = nullptr;
    std::size_t length = 0;
    {
        Arena arena(1 << 20);
        arena.grow(8);
        mapped = arena.base();
        length = arena.capacity();
    }
    // Asking the kernel to change protection on an unmapped range fails with ENOMEM.
    const int status = mprotect(mapped, length, PROT_READ);
    CHECK_NE(status, 0);
}

TEST(1, "bump: null and zero sizes follow the contract") { null_and_zero_are_handled<BumpAllocator>(); }
TEST(1, "bump: pointers are aligned and inside the heap") { pointers_are_aligned_and_inside_the_heap<BumpAllocator>(); }
TEST(1, "bump: realloc preserves data") { reallocation_preserves_data<BumpAllocator>(); }
TEST(1, "bump: exhausting the arena returns nullptr") { running_out_of_space_returns_null<BumpAllocator>(); }
TEST(1, "bump: absurd sizes are refused without side effects") { absurd_sizes_are_refused_cleanly<BumpAllocator>(); }
TEST(1, "bump: random ops keep every live block intact") { random_ops_match_a_model<BumpAllocator>(); }

TEST(1, "bump: a failed realloc keeps the old block") { failed_realloc_keeps_the_old_block<BumpAllocator>(); }

TEST(1, "bump: the most recent block grows in place") {
    BumpAllocator a(kHeapBytes);
    void* first = a.malloc(32);
    void* last = a.malloc(32);
    REQUIRE(first != nullptr && last != nullptr);
    fill(last, 32, 0x12);
    void* grown = a.realloc(last, 4000);
    CHECK(grown == last);
    CHECK(holds(grown, 32, 0x12));
}

TEST(1, "bump: free accepts any live pointer and does nothing else") {
    BumpAllocator a(kHeapBytes);
    void* p = a.malloc(64);
    void* q = a.malloc(64);
    REQUIRE(p != nullptr && q != nullptr);
    const std::size_t heap_before = a.heap_bytes();
    a.free(q);
    CHECK_EQ(a.heap_bytes(), heap_before);
    CHECK(a.check());
}

TEST(1, "bump: every workload shape validates") { all_shapes_validate<BumpAllocator>(1, 256); }

// ---------------------------------------------------------------------------------------------
// Stage 2 — implicit free list
// ---------------------------------------------------------------------------------------------

TEST(2, "implicit: null and zero sizes follow the contract") { null_and_zero_are_handled<ImplicitAllocator>(); }
TEST(2, "implicit: pointers are aligned and inside the heap") { pointers_are_aligned_and_inside_the_heap<ImplicitAllocator>(); }
TEST(2, "implicit: realloc preserves data") { reallocation_preserves_data<ImplicitAllocator>(); }
TEST(2, "implicit: exhausting the arena returns nullptr") { running_out_of_space_returns_null<ImplicitAllocator>(); }
TEST(2, "implicit: absurd sizes are refused without side effects") { absurd_sizes_are_refused_cleanly<ImplicitAllocator>(); }
TEST(2, "implicit: construction is cheap") { construction_is_cheap<ImplicitAllocator>(); }
TEST(2, "implicit: random ops keep every live block intact") { random_ops_match_a_model<ImplicitAllocator>(); }
TEST(2, "implicit: headers follow the published layout") { headers_follow_the_published_layout<ImplicitAllocator>(); }
TEST(2, "implicit: every block is reachable by walking the heap") { every_block_is_reachable_by_walking<ImplicitAllocator>(); }
TEST(2, "implicit: coalesces with the next block") { coalesces_with_the_next_block<ImplicitAllocator>(); }
TEST(2, "implicit: coalesces with the previous block") { coalesces_with_the_previous_block<ImplicitAllocator>(); }
TEST(2, "implicit: coalesces with both neighbours") { coalesces_with_both_neighbours<ImplicitAllocator>(); }
TEST(2, "implicit: freed memory is reused before growing") { freed_memory_is_reused_before_growing<ImplicitAllocator>(); }
TEST(2, "implicit: merged holes serve a larger request") { merged_holes_serve_a_larger_request<ImplicitAllocator>(); }
TEST(2, "implicit: splitting keeps the rest usable") { splitting_keeps_the_rest_usable<ImplicitAllocator>(); }
TEST(2, "implicit: realloc shrinks and grows in place when it can") { realloc_shrinks_and_grows_in_place_when_it_can<ImplicitAllocator>(); }
TEST(2, "implicit: the last block grows without moving") { the_last_block_grows_without_moving<ImplicitAllocator>(); }
TEST(2, "implicit: a failed realloc keeps the old block") { failed_realloc_keeps_the_old_block<ImplicitAllocator>(); }
TEST(2, "implicit: a free tail is reused when the heap grows") { a_free_tail_is_reused_when_the_heap_grows<ImplicitAllocator>(); }
TEST(2, "implicit: check notices a corrupted size") { corruption_of_a_size_is_noticed<ImplicitAllocator>(); }
TEST(2, "implicit: check notices a derailed walk") { a_derailed_walk_is_noticed<ImplicitAllocator>(); }
TEST(2, "implicit: check notices uncoalesced neighbours") { uncoalesced_free_neighbours_are_noticed<ImplicitAllocator>(); }
TEST(2, "implicit: every workload shape validates") { all_shapes_validate<ImplicitAllocator>(1, 16); }

// ---------------------------------------------------------------------------------------------
// Stage 3 — binned free lists
// ---------------------------------------------------------------------------------------------

TEST(3, "binned: null and zero sizes follow the contract") { null_and_zero_are_handled<BinnedAllocator>(); }
TEST(3, "binned: pointers are aligned and inside the heap") { pointers_are_aligned_and_inside_the_heap<BinnedAllocator>(); }
TEST(3, "binned: realloc preserves data") { reallocation_preserves_data<BinnedAllocator>(); }
TEST(3, "binned: exhausting the arena returns nullptr") { running_out_of_space_returns_null<BinnedAllocator>(); }
TEST(3, "binned: absurd sizes are refused without side effects") { absurd_sizes_are_refused_cleanly<BinnedAllocator>(); }
TEST(3, "binned: construction is cheap") { construction_is_cheap<BinnedAllocator>(); }
TEST(3, "binned: random ops keep every live block intact") { random_ops_match_a_model<BinnedAllocator>(); }
TEST(3, "binned: headers follow the published layout") { headers_follow_the_published_layout<BinnedAllocator>(); }
TEST(3, "binned: every block is reachable by walking the heap") { every_block_is_reachable_by_walking<BinnedAllocator>(); }
TEST(3, "binned: coalesces with the next block") { coalesces_with_the_next_block<BinnedAllocator>(); }
TEST(3, "binned: coalesces with the previous block") { coalesces_with_the_previous_block<BinnedAllocator>(); }
TEST(3, "binned: coalesces with both neighbours") { coalesces_with_both_neighbours<BinnedAllocator>(); }
TEST(3, "binned: freed memory is reused before growing") { freed_memory_is_reused_before_growing<BinnedAllocator>(); }
TEST(3, "binned: merged holes serve a larger request") { merged_holes_serve_a_larger_request<BinnedAllocator>(); }
TEST(3, "binned: splitting keeps the rest usable") { splitting_keeps_the_rest_usable<BinnedAllocator>(); }
TEST(3, "binned: realloc shrinks and grows in place when it can") { realloc_shrinks_and_grows_in_place_when_it_can<BinnedAllocator>(); }
TEST(3, "binned: the last block grows without moving") { the_last_block_grows_without_moving<BinnedAllocator>(); }
TEST(3, "binned: a failed realloc keeps the old block") { failed_realloc_keeps_the_old_block<BinnedAllocator>(); }
TEST(3, "binned: a free tail is reused when the heap grows") { a_free_tail_is_reused_when_the_heap_grows<BinnedAllocator>(); }
TEST(3, "binned: check notices a corrupted size") { corruption_of_a_size_is_noticed<BinnedAllocator>(); }
TEST(3, "binned: check notices a derailed walk") { a_derailed_walk_is_noticed<BinnedAllocator>(); }
TEST(3, "binned: check notices uncoalesced neighbours") { uncoalesced_free_neighbours_are_noticed<BinnedAllocator>(); }

TEST(3, "binned: check notices a free block that claims to be allocated") {
    Neighbours<BinnedAllocator> n;
    n.heap.free(n.b);
    CHECK(n.heap.check());
    set_header(n.b, header_of(n.b) | 1);
    CHECK(!n.heap.check());
}

TEST(3, "binned: check notices an allocated block missing from every free list") {
    Neighbours<BinnedAllocator> n;
    CHECK(n.heap.check());
    set_header(n.b, header_of(n.b) & ~std::uint64_t{1});  // free according to its header only
    CHECK(!n.heap.check());
}

TEST(3, "binned: check notices a use-after-free scribble on the list links") {
    Neighbours<BinnedAllocator> n;
    n.heap.free(n.b);
    CHECK(n.heap.check());
    std::memset(n.b, 0xAB, 16);  // the program wrote through a dangling pointer
    CHECK(!n.heap.check());
}

TEST(3, "binned: check rejects list links that look valid but leave the heap") {
    // Either encoding of a link (block or payload address) sees one of these as well-aligned, and
    // scribbling one link at a time means the other link still looks consistent.
    for (const std::uint64_t bogus : {std::uint64_t{0x1000'0008}, std::uint64_t{0x1000'0010}}) {
        for (const std::size_t word : {std::size_t{0}, std::size_t{1}}) {
            INFO("bogus ", bogus, " word ", word);
            Neighbours<BinnedAllocator> n;
            n.heap.free(n.a);
            n.heap.free(n.c);
            CHECK(n.heap.check());
            for (void* freed : {n.a, n.c}) {
                std::memcpy(static_cast<std::byte*>(freed) + word * sizeof bogus, &bogus, sizeof bogus);
            }
            CHECK(!n.heap.check());
        }
    }
}

TEST(3, "binned: every workload shape validates") { all_shapes_validate<BinnedAllocator>(1, 16); }
TEST(3, "binned: larger workloads validate") { scale_trace<BinnedAllocator>(4); }

// ---------------------------------------------------------------------------------------------
// Stage 4 — scoring against the baselines
// ---------------------------------------------------------------------------------------------

namespace {

struct ShapeFloor {
    const char* name;
    double implicit;
    double binned;
};

// Deterministic: these are ratios of byte counts, no clock involved.
constexpr ShapeFloor kFloors[] = {
    {"small-churn", 0.55, 0.65},    {"mixed-sizes", 0.60, 0.65}, {"sawtooth", 0.75, 0.80},
    {"checkerboard", 0.65, 0.70},   {"realloc-growth", 0.40, 0.40}, {"stack-lifo", 0.80, 0.85},
    {"long-lived", 0.80, 0.80},
};

const ShapeFloor& floor_for(const std::string& name) {
    for (const ShapeFloor& floor : kFloors) {
        if (name == floor.name) {
            return floor;
        }
    }
    throw std::logic_error("unknown trace shape: " + name);
}

// libc's allocator behind the same interface, for the throughput baseline.
class LibcAllocator {
public:
    void* malloc(std::size_t size) { return std::malloc(size); }
    void free(void* p) { std::free(p); }
    void* realloc(void* p, std::size_t size) { return std::realloc(p, size); }
};

template <typename Make>
double timed_run(Make&& make, const trace::Trace& t, int rounds) {
    double best = std::numeric_limits<double>::infinity();
    std::vector<void*> scratch;
    for (int round = 0; round < rounds; ++round) {
        auto allocator = make();
        const practice::Stopwatch watch;
        trace::run_ops(*allocator, t, scratch);
        best = std::min(best, watch.elapsed_ms());
    }
    return best;
}

}  // namespace

TEST(2, "implicit: space utilization clears the floor on every workload shape") {
    for (const trace::Trace& t : trace::all_shapes(1234, 3)) {
        ImplicitAllocator implicit(kHeapBytes);
        const trace::Report report = trace::validate(implicit, t, 0);
        INFO("trace ", t.name, " peak ", report.peak_live, " heap ", report.heap_bytes);
        CHECK_EQ(report.error, "");
        CHECK_GE(report.utilization(), floor_for(t.name).implicit);
    }
}

TEST(4, "binned: space utilization clears the floor on every workload shape") {
    for (const trace::Trace& t : trace::all_shapes(1234, 3)) {
        BinnedAllocator binned(kHeapBytes);
        const trace::Report report = trace::validate(binned, t, 4096);
        INFO("trace ", t.name, " peak ", report.peak_live, " heap ", report.heap_bytes);
        CHECK_EQ(report.error, "");
        CHECK_GE(report.utilization(), floor_for(t.name).binned);
    }
}

TEST(4, "bins do not give away the space the implicit list achieved") {
    double binned_product = 1.0;
    double implicit_product = 1.0;
    const auto traces = trace::all_shapes(77, 2);
    for (const trace::Trace& t : traces) {
        BinnedAllocator binned(kHeapBytes);
        ImplicitAllocator implicit(kHeapBytes);
        const auto binned_report = trace::validate(binned, t, 0);
        const auto implicit_report = trace::validate(implicit, t, 0);
        REQUIRE_EQ(binned_report.error, "");
        REQUIRE_EQ(implicit_report.error, "");
        binned_product *= binned_report.utilization();
        implicit_product *= implicit_report.utilization();
    }
    const double n = static_cast<double>(traces.size());
    const double binned_mean = std::pow(binned_product, 1.0 / n);
    const double implicit_mean = std::pow(implicit_product, 1.0 / n);
    INFO("geometric mean utilization: binned ", binned_mean, ", implicit ", implicit_mean);
    CHECK_GE(binned_mean, implicit_mean * 0.92);
}

TEST(4, "reusing memory roughly doubles utilization over never reusing it") {
    const auto traces = trace::all_shapes(5, 2);
    double bump_product = 1.0;
    double binned_product = 1.0;
    for (const trace::Trace& t : traces) {
        BumpAllocator bump(kHeapBytes);
        BinnedAllocator binned(kHeapBytes);
        const auto bump_report = trace::validate(bump, t, 0);
        const auto binned_report = trace::validate(binned, t, 0);
        REQUIRE_EQ(bump_report.error, "");
        REQUIRE_EQ(binned_report.error, "");
        bump_product *= bump_report.utilization();
        binned_product *= binned_report.utilization();
    }
    const double n = static_cast<double>(traces.size());
    const double bump_mean = std::pow(bump_product, 1.0 / n);
    const double binned_mean = std::pow(binned_product, 1.0 / n);
    INFO("geometric mean utilization: bump ", bump_mean, ", binned ", binned_mean);
    CHECK_GT(binned_mean, bump_mean * 2.0);
}

PERF(3, "two million small mallocs and frees", 1500) {
    BinnedAllocator a(kHeapBytes);
    std::vector<void*> blocks(200'000);
    std::size_t sink = 0;
    for (int round = 0; round < 5; ++round) {
        for (void*& block : blocks) {
            block = a.malloc(48);
            sink += reinterpret_cast<std::uintptr_t>(block) % 16;
        }
        for (void* block : blocks) {
            a.free(block);
        }
    }
    CHECK_EQ(sink, 0);
}

PERF(3, "malloc cost does not grow with the number of free blocks", 8000) {
    // Many free blocks that are all too small for the requests that follow: a linear scan of the
    // free blocks would visit every one of them on every malloc.
    const auto cost_for = [](std::size_t holes) {
        BinnedAllocator a(kHeapBytes);
        std::vector<void*> keep(holes);
        std::vector<void*> freed(holes);
        for (std::size_t i = 0; i < holes; ++i) {
            freed[i] = a.malloc(64);
            keep[i] = a.malloc(64);
        }
        for (void* p : freed) {
            a.free(p);
        }
        const practice::Stopwatch watch;
        for (int i = 0; i < 40'000; ++i) {
            void* p = a.malloc(4096);
            REQUIRE(p != nullptr);
            a.free(p);
        }
        return watch.elapsed_ms();
    };
    double small_cost = std::numeric_limits<double>::infinity();
    double large_cost = std::numeric_limits<double>::infinity();
    for (int round = 0; round < 3; ++round) {
        small_cost = std::min(small_cost, cost_for(10'000));
        large_cost = std::min(large_cost, cost_for(80'000));
    }
    INFO("small ", small_cost, " large ", large_cost);
    CHECK_LT(large_cost, small_cost * 3.0);
}

PERF(4, "binned is at least 5x faster than the implicit list on a fragmenting workload", 25'000) {
    const trace::Trace t = trace::long_lived(3, 8);
    const auto make_binned = [] { return std::make_unique<BinnedAllocator>(kHeapBytes); };
    const auto make_implicit = [] { return std::make_unique<ImplicitAllocator>(kHeapBytes); };
    double binned = timed_run(make_binned, t, 3);
    double implicit = timed_run(make_implicit, t, 2);
    INFO("binned ", binned, " ms, implicit ", implicit, " ms");
    CHECK_GE(implicit / binned, 5.0);
}

PERF(4, "throughput stays within a factor of libc on every workload shape", 25'000) {
    const auto make_binned = [] { return std::make_unique<BinnedAllocator>(kHeapBytes); };
    const auto make_libc = [] { return std::make_unique<LibcAllocator>(); };
    for (const trace::Trace& t : trace::all_shapes(11, 10)) {
        double binned = std::numeric_limits<double>::infinity();
        double libc = std::numeric_limits<double>::infinity();
        for (int round = 0; round < 4; ++round) {
            binned = std::min(binned, timed_run(make_binned, t, 1));
            libc = std::min(libc, timed_run(make_libc, t, 1));
        }
        INFO("trace ", t.name, " binned ", binned, " ms, libc ", libc, " ms");
        CHECK_GE(libc / binned, 0.33);
    }
}

// Prints one line per workload shape; run the binary by hand with --perf-only --stage 4 --verbose.
PERF(4, "scorecard", 60'000) {
    std::cout << "\n  shape            util: bump  implicit  binned   throughput vs libc, vs implicit\n";
    std::vector<void*> scratch;
    for (const trace::Trace& t : trace::all_shapes(31, 6)) {
        const auto utilization = [&](auto&& allocator) {
            trace::run_ops(allocator, t, scratch);
            return static_cast<double>(t.peak_live) / static_cast<double>(allocator.heap_bytes());
        };
        const double bump_util = utilization(BumpAllocator(kHeapBytes));
        const double implicit_util = utilization(ImplicitAllocator(kHeapBytes));
        const double binned_util = utilization(BinnedAllocator(kHeapBytes));
        const double libc_ms = timed_run([] { return std::make_unique<LibcAllocator>(); }, t, 2);
        const double implicit_ms =
            timed_run([] { return std::make_unique<ImplicitAllocator>(kHeapBytes); }, t, 1);
        const double binned_ms = timed_run([] { return std::make_unique<BinnedAllocator>(kHeapBytes); }, t, 2);
        std::cout << "  " << t.name << ": " << bump_util << ", " << implicit_util << ", " << binned_util << "   "
                  << libc_ms / binned_ms << "x, " << implicit_ms / binned_ms << "x\n";
    }
}

PERF(4, "the combined index beats the implicit-list baseline", 25'000) {
    // Score each trace by the geometric mean of utilization and throughput ratio against libc,
    // the same formula the original project uses, and compare the two allocators overall.
    std::vector<void*> scratch;
    double binned_score = 0;
    double implicit_score = 0;
    int counted = 0;
    for (const trace::Trace& t : trace::all_shapes(21, 6)) {
        double libc_ms = std::numeric_limits<double>::infinity();
        double binned_ms = std::numeric_limits<double>::infinity();
        double implicit_ms = std::numeric_limits<double>::infinity();
        double binned_util = 0;
        double implicit_util = 0;
        for (int round = 0; round < 2; ++round) {
            libc_ms = std::min(libc_ms, timed_run([] { return std::make_unique<LibcAllocator>(); }, t, 1));
            auto binned = std::make_unique<BinnedAllocator>(kHeapBytes);
            auto implicit = std::make_unique<ImplicitAllocator>(kHeapBytes);
            {
                const practice::Stopwatch watch;
                trace::run_ops(*binned, t, scratch);
                binned_ms = std::min(binned_ms, watch.elapsed_ms());
            }
            {
                const practice::Stopwatch watch;
                trace::run_ops(*implicit, t, scratch);
                implicit_ms = std::min(implicit_ms, watch.elapsed_ms());
            }
            binned_util = static_cast<double>(t.peak_live) / static_cast<double>(binned->heap_bytes());
            implicit_util = static_cast<double>(t.peak_live) / static_cast<double>(implicit->heap_bytes());
        }
        binned_score += std::sqrt(binned_util * std::min(1.0, libc_ms / binned_ms));
        implicit_score += std::sqrt(implicit_util * std::min(1.0, libc_ms / implicit_ms));
        ++counted;
    }
    binned_score /= counted;
    implicit_score /= counted;
    INFO("average index: binned ", binned_score, ", implicit ", implicit_score);
    CHECK_GE(binned_score, implicit_score);
    CHECK_GE(binned_score, 0.6);
}
