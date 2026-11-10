#include <algorithm>
#include <array>
#include <cstring>
#include <map>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

TEST(1, "large allocations use each line once") {
    TwoSizeHeap heap(3, Coalesce::Never);
    CHECK_EQ(heap.free_bytes(), 3 * 64);
    std::set<std::size_t> seen;
    for (int i = 0; i < 3; ++i) {
        const auto offset = heap.alloc_large();
        REQUIRE(offset.has_value());
        CHECK_EQ(*offset % 64, 0);
        CHECK_LT(*offset, 3 * 64);
        CHECK(seen.insert(*offset).second);
    }
    CHECK(!heap.alloc_large().has_value());
    CHECK_EQ(heap.free_bytes(), 0);
}

TEST(1, "a small allocation splits a line and keeps the other half") {
    TwoSizeHeap heap(2, Coalesce::Never);
    const auto a = heap.alloc_small();
    REQUIRE(a.has_value());
    CHECK_EQ(*a % 64, 0);  // the low half is returned
    CHECK_EQ(heap.free_bytes(), 2 * 64 - 32);
    const auto b = heap.alloc_small();
    REQUIRE(b.has_value());
    CHECK_EQ(*b, *a + 32);  // the high half of the same line comes next
    CHECK_EQ(heap.free_bytes(), 2 * 64 - 64);
}

TEST(1, "small requests fail only when there is nothing left to split") {
    TwoSizeHeap heap(1, Coalesce::Never);
    CHECK(heap.alloc_small().has_value());
    CHECK(heap.alloc_small().has_value());
    CHECK(!heap.alloc_small().has_value());
}

TEST(1, "freed blocks return to their own list") {
    TwoSizeHeap heap(2, Coalesce::Never);
    const auto a = heap.alloc_large();
    const auto b = heap.alloc_small();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    heap.free_large(*a);
    heap.free_small(*b);
    CHECK_EQ(heap.free_bytes(), 2 * 64);
}

TEST(1, "without coalescing, free space can no longer serve a large request") {
    TwoSizeHeap heap(4, Coalesce::Never);
    std::vector<std::size_t> smalls;
    for (int i = 0; i < 8; ++i) {
        const auto offset = heap.alloc_small();
        REQUIRE(offset.has_value());
        smalls.push_back(*offset);
    }
    for (std::size_t offset : smalls) {
        heap.free_small(offset);
    }
    CHECK_EQ(heap.free_bytes(), 4 * 64);    // all of it is free...
    CHECK(!heap.alloc_large().has_value());  // ...but none of it is contiguous 64 bytes
}

TEST(2, "two free halves of one line merge back") {
    TwoSizeHeap heap(1, Coalesce::Adjacent);
    const auto a = heap.alloc_small();
    const auto b = heap.alloc_small();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    CHECK(!heap.alloc_large().has_value());
    heap.free_small(*a);
    CHECK(!heap.alloc_large().has_value());
    heap.free_small(*b);
    const auto merged = heap.alloc_large();
    REQUIRE(merged.has_value());
    CHECK_EQ(*merged, *a);
    CHECK_EQ(heap.free_bytes(), 0);
}

TEST(2, "halves of different lines do not merge") {
    TwoSizeHeap heap(2, Coalesce::Adjacent);
    const auto a = heap.alloc_small();
    const auto b = heap.alloc_small();
    const auto c = heap.alloc_small();
    const auto d = heap.alloc_small();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    REQUIRE(d.has_value());
    heap.free_small(*b);  // high half of line 0
    heap.free_small(*c);  // low half of line 1
    CHECK(!heap.alloc_large().has_value());
    CHECK_EQ(heap.free_bytes(), 64);
}

TEST(2, "after full release every line is large again") {
    Rng rng;
    TwoSizeHeap heap(16, Coalesce::Adjacent);
    std::vector<std::size_t> offsets;
    for (int i = 0; i < 32; ++i) {
        const auto offset = heap.alloc_small();
        REQUIRE(offset.has_value());
        offsets.push_back(*offset);
    }
    rng.shuffle(offsets);
    for (std::size_t offset : offsets) {
        heap.free_small(offset);
    }
    for (int i = 0; i < 16; ++i) {
        REQUIRE(heap.alloc_large().has_value());
    }
    CHECK(!heap.alloc_large().has_value());
}

TEST(2, "random mixed workload matches a line-occupancy model") {
    Rng rng;
    const std::size_t lines = 32;
    TwoSizeHeap heap(lines, Coalesce::Adjacent);
    std::map<std::size_t, bool> live;  // offset -> is_small
    const auto model_free = [&] {
        std::size_t used = 0;
        for (const auto& [offset, small] : live) {
            used += small ? 32 : 64;
        }
        return lines * 64 - used;
    };
    for (int step = 0; step < 20'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 2);
        if (op == 0) {
            const auto offset = heap.alloc_small();
            if (offset) {
                REQUIRE_EQ(*offset % 32, 0);
                REQUIRE(!live.contains(*offset));
                live[*offset] = true;
            }
        } else if (op == 1) {
            const auto offset = heap.alloc_large();
            if (offset) {
                REQUIRE_EQ(*offset % 64, 0);
                REQUIRE(!live.contains(*offset) && !live.contains(*offset + 32));
                live[*offset] = false;
            }
        } else if (!live.empty()) {
            auto it = live.begin();
            std::advance(it, rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            if (it->second) {
                heap.free_small(it->first);
            } else {
                heap.free_large(it->first);
            }
            live.erase(it);
        }
        REQUIRE_EQ(heap.free_bytes(), model_free());
    }
}

namespace {

void fill(std::span<std::byte> bytes, unsigned char value) {
    std::memset(bytes.data(), value, bytes.size());
}

bool holds(std::span<const std::byte> bytes, unsigned char value) {
    return std::all_of(bytes.begin(), bytes.end(),
                       [&](std::byte b) { return static_cast<unsigned char>(b) == value; });
}

}  // namespace

TEST(3, "objects keep their contents through deref") {
    CompactingHeap heap(4);
    const auto h = heap.alloc_small();
    REQUIRE(h.has_value());
    CHECK_EQ(heap.deref(*h).size(), 32);
    fill(heap.deref(*h), 0x5A);
    CHECK(holds(heap.deref(*h), 0x5A));
    const auto large = heap.alloc_large();
    REQUIRE(large.has_value());
    CHECK_EQ(heap.deref(*large).size(), 64);
    CHECK_EQ(heap.free_bytes(), 4 * 64 - 32 - 64);
}

TEST(3, "compaction turns scattered halves into free lines") {
    CompactingHeap heap(6);
    std::vector<CompactingHeap::Handle> kept;
    std::vector<CompactingHeap::Handle> dropped;
    for (int line = 0; line < 6; ++line) {
        const auto a = heap.alloc_small();
        const auto b = heap.alloc_small();
        REQUIRE(a.has_value());
        REQUIRE(b.has_value());
        (line % 2 == 0 ? kept : dropped).push_back(*a);
        (line % 2 == 0 ? dropped : kept).push_back(*b);
    }
    for (std::size_t i = 0; i < kept.size(); ++i) {
        fill(heap.deref(kept[i]), static_cast<unsigned char>(0x10 + i));
    }
    for (auto handle : dropped) {
        heap.free(handle);
    }
    // Six half-empty lines: 192 bytes free, none of it a whole line.
    CHECK_EQ(heap.free_bytes(), 192);

    heap.compact();
    for (std::size_t i = 0; i < kept.size(); ++i) {
        INFO("object ", i);
        CHECK(holds(heap.deref(kept[i]), static_cast<unsigned char>(0x10 + i)));
    }
    // Now whole lines exist: three large objects fit, a fourth does not.
    CHECK_EQ(heap.free_bytes(), 192);
    REQUIRE(heap.alloc_large().has_value());
    REQUIRE(heap.alloc_large().has_value());
    REQUIRE(heap.alloc_large().has_value());
    CHECK(!heap.alloc_large().has_value());
}

TEST(3, "alloc_large compacts by itself when it has to") {
    CompactingHeap heap(2);
    const auto a = heap.alloc_small();
    const auto b = heap.alloc_small();
    const auto c = heap.alloc_small();
    const auto d = heap.alloc_small();
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    REQUIRE(c.has_value());
    REQUIRE(d.has_value());
    fill(heap.deref(*a), 0xAA);
    fill(heap.deref(*d), 0xDD);
    heap.free(*b);  // high half of line 0
    heap.free(*c);  // low half of line 1
    const auto large = heap.alloc_large();
    REQUIRE(large.has_value());
    fill(heap.deref(*large), 0x11);
    CHECK(holds(heap.deref(*a), 0xAA));
    CHECK(holds(heap.deref(*d), 0xDD));
    CHECK(holds(heap.deref(*large), 0x11));
}

TEST(3, "alloc_large fails when too little space is free in total") {
    CompactingHeap heap(1);
    const auto a = heap.alloc_small();
    REQUIRE(a.has_value());
    CHECK(!heap.alloc_large().has_value());
    CHECK_EQ(heap.free_bytes(), 32);
}

TEST(3, "compaction leaves at most one half-empty line") {
    Rng rng;
    CompactingHeap heap(40);
    std::vector<CompactingHeap::Handle> live;
    for (int i = 0; i < 80; ++i) {
        const auto h = heap.alloc_small();
        REQUIRE(h.has_value());
        live.push_back(*h);
    }
    rng.shuffle(live);
    for (std::size_t i = 0; i < 37; ++i) {
        heap.free(live.back());
        live.pop_back();
    }
    heap.compact();
    // 43 live halves need 22 lines at best; every other line must now be free.
    int large_count = 0;
    while (heap.alloc_large().has_value()) {
        ++large_count;
    }
    CHECK_EQ(large_count, 40 - 22);
}

TEST(3, "random churn with periodic compaction preserves every object") {
    Rng rng;
    CompactingHeap heap(64);
    struct Live {
        CompactingHeap::Handle handle;
        unsigned char tag;
    };
    std::vector<Live> live;
    for (int step = 0; step < 15'000; ++step) {
        INFO("step ", step);
        const auto op = rng.uniform(0, 9);
        if (op <= 3) {
            if (const auto h = heap.alloc_small()) {
                const auto tag = static_cast<unsigned char>(rng.uniform(0, 255));
                fill(heap.deref(*h), tag);
                live.push_back({*h, tag});
            }
        } else if (op <= 5) {
            if (const auto h = heap.alloc_large()) {
                const auto tag = static_cast<unsigned char>(rng.uniform(0, 255));
                fill(heap.deref(*h), tag);
                live.push_back({*h, tag});
            }
        } else if (op <= 8 && !live.empty()) {
            const auto i = static_cast<std::size_t>(
                rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            REQUIRE(holds(heap.deref(live[i].handle), live[i].tag));
            heap.free(live[i].handle);
            live[i] = live.back();
            live.pop_back();
        } else {
            heap.compact();
            for (const Live& object : live) {
                REQUIRE(holds(heap.deref(object.handle), object.tag));
            }
        }
    }
}
