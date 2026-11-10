#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Slots = std::vector<Slot>;

std::vector<Slot> random_slots(Rng& rng, std::size_t max_length) {
    Slots slots(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_length))));
    for (Slot& slot : slots) {
        slot.base = rng.uniform(0, 12);
        slot.burst = slot.base + rng.uniform(1, 20);
    }
    return slots;
}

// Throughput of a burst choice, or -1 when two bursts are closer than the cooldown allows.
std::int64_t plan_value(const Slots& slots, const std::vector<std::size_t>& plan,
                        std::size_t cooldown) {
    std::vector<bool> is_burst(slots.size(), false);
    for (std::size_t k = 0; k < plan.size(); ++k) {
        REQUIRE(plan[k] < slots.size());
        if (k > 0) {
            REQUIRE(plan[k - 1] < plan[k]);
            if (plan[k] - plan[k - 1] <= cooldown) {
                return -1;
            }
        }
        is_burst[plan[k]] = true;
    }
    std::int64_t total = 0;
    for (std::size_t i = 0; i < slots.size(); ++i) {
        total += is_burst[i] ? slots[i].burst : slots[i].base;
    }
    return total;
}

// Tries every subset of slots to burst.
std::int64_t exhaustive(const Slots& slots, std::size_t cooldown,
                        std::size_t max_bursts = std::numeric_limits<std::size_t>::max()) {
    std::int64_t best = -1;
    for (std::size_t mask = 0; mask < (std::size_t{1} << slots.size()); ++mask) {
        if (static_cast<std::size_t>(std::popcount(mask)) > max_bursts) {
            continue;
        }
        std::vector<std::size_t> plan;
        for (std::size_t i = 0; i < slots.size(); ++i) {
            if ((mask >> i) & 1) {
                plan.push_back(i);
            }
        }
        best = std::max(best, plan_value(slots, plan, cooldown));
    }
    return best;
}

}  // namespace

TEST(1, "no slots yield nothing") {
    CHECK_EQ(max_throughput({}), 0);
    CHECK(can_meet_quota({}, 0));
    CHECK(!can_meet_quota({}, 1));
}

TEST(1, "a single slot should burst") {
    const Slots slots{{2, 9}};
    CHECK_EQ(max_throughput(slots), 9);
}

TEST(1, "two slots can only burst once") {
    const Slots slots{{1, 10}, {4, 6}};
    CHECK_EQ(max_throughput(slots), 14);  // burst the first, the second keeps its base of 4
}

TEST(1, "slots three apart can both burst") {
    const Slots slots{{1, 5}, {1, 5}, {1, 5}, {1, 5}};
    CHECK_EQ(max_throughput(slots), 5 + 1 + 1 + 5);
}

TEST(1, "bursting everywhere early can be worse than waiting for the peak") {
    const Slots slots{{1, 2}, {1, 2}, {1, 100}};
    CHECK_EQ(max_throughput(slots), 1 + 1 + 100);
}

TEST(1, "quota uses the exact maximum as the boundary") {
    const Slots slots{{3, 7}, {2, 9}, {4, 5}};
    const std::int64_t best = max_throughput(slots);
    CHECK(can_meet_quota(slots, best));
    CHECK(!can_meet_quota(slots, best + 1));
}

TEST(1, "a majority of the total burst capacity is a quota like any other") {
    const Slots slots{{1, 10}, {1, 8}, {1, 10}, {1, 8}, {1, 10}};
    std::int64_t total_burst = 0;
    for (const Slot& slot : slots) {
        total_burst += slot.burst;
    }
    // The best schedule bursts slots 0 and 4 for 10 + 1 + 1 + 1 + 10 = 23, one short of 46 / 2 + 1.
    CHECK(!can_meet_quota(slots, total_burst / 2 + 1));
    CHECK(can_meet_quota(slots, total_burst / 2));
}

TEST(1, "random slots match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Slots slots = random_slots(rng, 12);
        INFO("trial ", trial);
        REQUIRE_EQ(max_throughput(slots), exhaustive(slots, 2));
    }
}

PERF(1, "five million slots in linear time", 800) {
    Rng rng;
    Slots slots(5'000'000);
    for (Slot& slot : slots) {
        slot.base = rng.uniform(0, 100);
        slot.burst = slot.base + rng.uniform(1, 100);
    }
    CHECK_GT(max_throughput(slots), 0);
}

TEST(2, "a lone slot is worth bursting") {
    const Slots slots{{0, 1}};
    CHECK_EQ(burst_plan(slots), std::vector<std::size_t>{0});
}

TEST(2, "no slots mean no plan") {
    CHECK(burst_plan({}).empty());
}

TEST(2, "plans respect the cooldown gap") {
    const Slots slots{{1, 5}, {1, 9}, {1, 5}, {1, 5}, {1, 9}};
    const auto plan = burst_plan(slots);
    CHECK_EQ(plan_value(slots, plan, 2), max_throughput(slots));
}

TEST(2, "random plans are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const Slots slots = random_slots(rng, 12);
        INFO("trial ", trial);
        REQUIRE_EQ(plan_value(slots, burst_plan(slots), 2), exhaustive(slots, 2));
    }
}

TEST(3, "cooldown zero bursts every slot") {
    const Slots slots{{1, 5}, {2, 6}, {3, 7}};
    CHECK_EQ(max_throughput(slots, 0), 18);
    CHECK_EQ(burst_plan(slots, 0).size(), 3);
}

TEST(3, "a cooldown longer than the schedule means a single burst") {
    const Slots slots{{1, 5}, {1, 10}, {1, 3}};
    CHECK_EQ(max_throughput(slots, 100), 10 + 1 + 1);
    CHECK_EQ(burst_plan(slots, 100), std::vector<std::size_t>{1});
}

TEST(3, "a large cooldown near the end of the schedule does not read past it") {
    const Slots slots{{1, 5}, {1, 5}};
    CHECK_EQ(max_throughput(slots, 1), 6);
    CHECK_EQ(max_throughput(slots, std::numeric_limits<std::size_t>::max() / 2), 6);
}

TEST(3, "random cooldowns match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const Slots slots = random_slots(rng, 12);
        const auto cooldown = static_cast<std::size_t>(rng.uniform(0, 6));
        INFO("trial ", trial, " cooldown ", cooldown);
        REQUIRE_EQ(max_throughput(slots, cooldown), exhaustive(slots, cooldown));
        REQUIRE_EQ(plan_value(slots, burst_plan(slots, cooldown), cooldown),
                   exhaustive(slots, cooldown));
    }
}

TEST(3, "extra memory depends on the cooldown, not on the length") {
    Rng rng;
    Slots slots(200'000);
    for (Slot& slot : slots) {
        slot.base = rng.uniform(0, 100);
        slot.burst = slot.base + rng.uniform(1, 100);
    }
    heap_meter::Scope scope;
    CHECK_GT(max_throughput(slots, 8), 0);
    CHECK_LE(scope.peak_bytes(), 4096);
}

PERF(3, "a long cooldown does not slow each step", 800) {
    Rng rng;
    Slots slots(1'000'000);
    for (Slot& slot : slots) {
        slot.base = rng.uniform(0, 100);
        slot.burst = slot.base + rng.uniform(1, 100);
    }
    CHECK_GT(max_throughput(slots, 200'000), 0);
    CHECK_GE(burst_plan(slots, 200'000).size(), 1);
}

TEST(4, "no budget means no bursts") {
    const Slots slots{{1, 5}, {2, 9}};
    CHECK_EQ(max_throughput_limited(slots, 0, 0), 3);
}

TEST(4, "the budget picks the most valuable bursts") {
    const Slots slots{{0, 3}, {0, 9}, {0, 4}, {0, 8}};
    CHECK_EQ(max_throughput_limited(slots, 0, 2), 17);
}

TEST(4, "a large enough budget behaves like the unlimited schedule") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const Slots slots = random_slots(rng, 14);
        INFO("trial ", trial);
        REQUIRE_EQ(max_throughput_limited(slots, 2, slots.size() + 5), max_throughput(slots));
    }
}

TEST(4, "random budgets match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Slots slots = random_slots(rng, 12);
        const auto cooldown = static_cast<std::size_t>(rng.uniform(0, 4));
        const auto budget = static_cast<std::size_t>(rng.uniform(0, 5));
        INFO("trial ", trial, " cooldown ", cooldown, " budget ", budget);
        REQUIRE_EQ(max_throughput_limited(slots, cooldown, budget),
                   exhaustive(slots, cooldown, budget));
    }
}

PERF(4, "thirty thousand slots and a budget of three thousand", 1000) {
    Rng rng;
    Slots slots(30'000);
    for (Slot& slot : slots) {
        slot.base = rng.uniform(0, 100);
        slot.burst = slot.base + rng.uniform(1, 100);
    }
    CHECK_GT(max_throughput_limited(slots, 3, 3'000), 0);
}
