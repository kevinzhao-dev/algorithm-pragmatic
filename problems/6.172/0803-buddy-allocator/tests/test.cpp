#include <algorithm>
#include <map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::size_t total_free_blocks(const BuddyAllocator& allocator, int max_order) {
    std::size_t total = 0;
    for (int order = 0; order <= max_order; ++order) {
        total += allocator.free_blocks(order);
    }
    return total;
}

std::size_t blocks_on_lowest_orders(const BuddyAllocator& allocator) {
    return allocator.free_blocks(0) + allocator.free_blocks(1);
}

}  // namespace

TEST(1, "a fresh arena is one max-order block") {
    BuddyAllocator allocator(4);
    CHECK_EQ(allocator.free_blocks(4), 1);
    CHECK_EQ(allocator.free_blocks(3), 0);
    CHECK_EQ(allocator.free_units(), 16);
}

TEST(1, "the first allocation splits and leaves one block per smaller order") {
    BuddyAllocator allocator(4);
    const auto offset = allocator.allocate(1);
    REQUIRE(offset.has_value());
    CHECK_EQ(*offset, 0);  // the low half is kept while splitting
    CHECK_EQ(allocator.free_blocks(4), 0);
    CHECK_EQ(allocator.free_blocks(3), 1);
    CHECK_EQ(allocator.free_blocks(2), 1);
    CHECK_EQ(allocator.free_blocks(1), 1);
    CHECK_EQ(allocator.free_blocks(0), 1);
    CHECK_EQ(allocator.free_units(), 15);
}

TEST(1, "a request rounds up to its order and stays aligned") {
    BuddyAllocator allocator(8);
    const auto a = allocator.allocate(5);  // order 3, 8 units
    REQUIRE(a.has_value());
    CHECK_EQ(*a % 8, 0);
    CHECK_EQ(allocator.free_units(), 256 - 8);
    const auto b = allocator.allocate(8);
    REQUIRE(b.has_value());
    CHECK_EQ(*b % 8, 0);
    CHECK_EQ(allocator.free_units(), 256 - 16);
}

TEST(1, "an existing block of the right order is used without splitting") {
    BuddyAllocator allocator(3);
    const auto a = allocator.allocate(1);  // splits into orders 2, 1, 0
    REQUIRE(a.has_value());
    CHECK_EQ(blocks_on_lowest_orders(allocator), 2);
    const auto b = allocator.allocate(2);
    REQUIRE(b.has_value());
    CHECK_EQ(allocator.free_blocks(1), 0);
    CHECK_EQ(allocator.free_blocks(2), 1);
}

TEST(1, "too-large requests and exhaustion return nullopt") {
    BuddyAllocator allocator(2);
    CHECK(!allocator.allocate(5).has_value());
    CHECK(allocator.allocate(4).has_value());
    CHECK(!allocator.allocate(1).has_value());
}

TEST(1, "a zero-unit request is an error") {
    BuddyAllocator allocator(2);
    CHECK_THROWS_AS(allocator.allocate(0), std::invalid_argument);
}

TEST(1, "a max-order request takes the whole arena") {
    BuddyAllocator allocator(4);
    CHECK_EQ(allocator.allocate(16), std::optional<std::size_t>(0));
    CHECK_EQ(allocator.free_units(), 0);
}

TEST(2, "buddies merge back when freed") {
    BuddyAllocator allocator(3);
    const auto a = allocator.allocate(4);
    const auto b = allocator.allocate(4);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    allocator.free(*a);
    CHECK_EQ(allocator.free_blocks(2), 1);
    allocator.free(*b);
    CHECK_EQ(allocator.free_blocks(3), 1);
    CHECK_EQ(allocator.free_blocks(2), 0);
    CHECK_EQ(allocator.free_units(), 8);
}

TEST(2, "non-buddy neighbours do not merge") {
    BuddyAllocator allocator(3);
    std::vector<std::size_t> offsets;
    for (int i = 0; i < 8; ++i) {
        const auto offset = allocator.allocate(1);
        REQUIRE(offset.has_value());
        offsets.push_back(*offset);
    }
    std::sort(offsets.begin(), offsets.end());
    // Offsets 1 and 2 are adjacent but are not buddies (they sit in different halves).
    allocator.free(offsets[1]);
    allocator.free(offsets[2]);
    CHECK_EQ(allocator.free_blocks(1), 0);
    CHECK_EQ(allocator.free_blocks(0), 2);
}

TEST(2, "freeing cascades up through several orders") {
    BuddyAllocator allocator(4);
    std::vector<std::size_t> offsets;
    for (int i = 0; i < 16; ++i) {
        const auto offset = allocator.allocate(1);
        REQUIRE(offset.has_value());
        offsets.push_back(*offset);
    }
    for (std::size_t i = 0; i + 1 < offsets.size(); ++i) {
        allocator.free(offsets[i]);
    }
    allocator.free(offsets.back());
    CHECK_EQ(allocator.free_blocks(4), 1);
    CHECK_EQ(total_free_blocks(allocator, 4), 1);
}

TEST(2, "any free order restores one max-order block") {
    Rng rng;
    for (int trial = 0; trial < 20; ++trial) {
        INFO("trial ", trial);
        BuddyAllocator allocator(8);
        std::vector<std::size_t> offsets;
        while (true) {
            const auto offset = allocator.allocate(static_cast<std::size_t>(rng.uniform(1, 12)));
            if (!offset) {
                break;
            }
            offsets.push_back(*offset);
        }
        rng.shuffle(offsets);
        for (std::size_t offset : offsets) {
            allocator.free(offset);
        }
        REQUIRE_EQ(allocator.free_blocks(8), 1);
        REQUIRE_EQ(total_free_blocks(allocator, 8), 1);
    }
}

namespace {

// Exact model of the eagerly-coalesced buddy state, one cell per unit.
class BuddyModel {
public:
    explicit BuddyModel(int max_order) : max_order_(max_order), used_(std::size_t{1} << max_order) {}

    static int order_of(std::size_t units) {
        int order = 0;
        while ((std::size_t{1} << order) < units) {
            ++order;
        }
        return order;
    }

    // Allocation succeeds exactly when some aligned region of that order is entirely free.
    bool can_allocate(std::size_t units) const {
        const int order = order_of(units);
        if (order > max_order_) {
            return false;
        }
        const std::size_t size = std::size_t{1} << order;
        for (std::size_t start = 0; start < used_.size(); start += size) {
            if (region_free(start, size)) {
                return true;
            }
        }
        return false;
    }

    void mark(std::size_t offset, int order, bool used) {
        const std::size_t size = std::size_t{1} << order;
        for (std::size_t i = offset; i < offset + size; ++i) {
            used_[i] = used;
        }
    }

    bool region_free(std::size_t start, std::size_t size) const {
        for (std::size_t i = start; i < start + size; ++i) {
            if (used_[i]) {
                return false;
            }
        }
        return true;
    }

    std::size_t free_units() const {
        return static_cast<std::size_t>(std::count(used_.begin(), used_.end(), false));
    }

    // A free block of this order exists where a region is free but its parent is not.
    std::size_t free_blocks_at(int order) const {
        const std::size_t size = std::size_t{1} << order;
        std::size_t count = 0;
        for (std::size_t start = 0; start < used_.size(); start += size) {
            if (!region_free(start, size)) {
                continue;
            }
            if (order == max_order_) {
                ++count;
            } else {
                const std::size_t parent = start & ~(2 * size - 1);
                if (!region_free(parent, 2 * size)) {
                    ++count;
                }
            }
        }
        return count;
    }

    std::size_t largest_free() const {
        for (int order = max_order_; order >= 0; --order) {
            if (free_blocks_at(order) > 0) {
                return std::size_t{1} << order;
            }
        }
        return 0;
    }

private:
    int max_order_;
    std::vector<bool> used_;
};

}  // namespace

TEST(2, "random workload matches an exact model of coalesced state") {
    Rng rng;
    const int max_order = 9;
    BuddyAllocator allocator(max_order);
    BuddyModel model(max_order);
    std::vector<std::pair<std::size_t, int>> live;

    for (int step = 0; step < 4000; ++step) {
        INFO("step ", step);
        if (live.empty() || rng.coin(0.55)) {
            const auto units = static_cast<std::size_t>(rng.uniform(1, 40));
            const auto result = allocator.allocate(units);
            REQUIRE_EQ(result.has_value(), model.can_allocate(units));
            if (result) {
                const int order = BuddyModel::order_of(units);
                const std::size_t size = std::size_t{1} << order;
                REQUIRE_EQ(*result % size, 0);
                REQUIRE(model.region_free(*result, size));
                model.mark(*result, order, true);
                live.emplace_back(*result, order);
            }
        } else {
            const auto i = static_cast<std::size_t>(
                rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            allocator.free(live[i].first);
            model.mark(live[i].first, live[i].second, false);
            live[i] = live.back();
            live.pop_back();
        }
        REQUIRE_EQ(allocator.free_units(), model.free_units());
        if (step % 16 == 0) {
            for (int order = 0; order <= max_order; ++order) {
                INFO("order ", order);
                REQUIRE_EQ(allocator.free_blocks(order), model.free_blocks_at(order));
            }
        }
    }
}

TEST(3, "fragmentation is zero for a single free block") {
    BuddyAllocator allocator(4);
    CHECK_EQ(allocator.largest_free_block(), 16);
    CHECK_NEAR(allocator.external_fragmentation(), 0.0, 1e-12);
    allocator.allocate(16);
    CHECK_EQ(allocator.largest_free_block(), 0);
    CHECK_NEAR(allocator.external_fragmentation(), 0.0, 1e-12);
}

TEST(3, "a checkerboard is maximally fragmented") {
    BuddyAllocator allocator(4);
    std::vector<std::size_t> offsets;
    for (int i = 0; i < 16; ++i) {
        const auto offset = allocator.allocate(1);
        REQUIRE(offset.has_value());
        offsets.push_back(*offset);
    }
    std::sort(offsets.begin(), offsets.end());
    for (std::size_t i = 0; i < offsets.size(); i += 2) {
        allocator.free(offsets[i]);
    }
    CHECK_EQ(allocator.free_units(), 8);
    CHECK_EQ(allocator.largest_free_block(), 1);
    CHECK_NEAR(allocator.external_fragmentation(), 1.0 - 1.0 / 8.0, 1e-12);
    CHECK(!allocator.allocate(2).has_value());
}

TEST(3, "fragmentation tracks a mixed layout") {
    BuddyAllocator allocator(4);
    const auto a = allocator.allocate(8);
    const auto b = allocator.allocate(1);
    REQUIRE(a.has_value());
    REQUIRE(b.has_value());
    // Free: orders 2, 1, 0 → 4 + 2 + 1 = 7 units.
    CHECK_EQ(allocator.free_units(), 7);
    CHECK_EQ(allocator.largest_free_block(), 4);
    CHECK_NEAR(allocator.external_fragmentation(), 1.0 - 4.0 / 7.0, 1e-12);
}

// A linear scan of the arena or of the free blocks takes minutes at this scale.
PERF(2, "allocate and free stay logarithmic on a huge arena", 6000) {
    Rng rng;
    BuddyAllocator allocator(40);
    std::vector<std::size_t> live;
    live.reserve(400'000);
    for (int step = 0; step < 1'200'000; ++step) {
        if (live.empty() || (live.size() < 400'000 && rng.coin(0.6))) {
            const auto offset = allocator.allocate(static_cast<std::size_t>(rng.uniform(1, 64)));
            REQUIRE(offset.has_value());
            live.push_back(*offset);
        } else {
            const auto i = static_cast<std::size_t>(
                rng.uniform(0, static_cast<std::int64_t>(live.size()) - 1));
            allocator.free(live[i]);
            live[i] = live.back();
            live.pop_back();
        }
    }
    for (std::size_t offset : live) {
        allocator.free(offset);
    }
    CHECK_EQ(allocator.free_units(), std::size_t{1} << 40);
}
