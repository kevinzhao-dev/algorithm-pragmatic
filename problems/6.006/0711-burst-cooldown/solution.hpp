#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

struct Slot {
    std::int64_t base;
    std::int64_t burst;
};

// Stage 1, generalized in Stage 3
inline std::int64_t max_throughput(std::span<const Slot> slots, std::size_t cooldown = 2) {
    PRACTICE_TODO();
}

inline bool can_meet_quota(std::span<const Slot> slots, std::int64_t quota,
                           std::size_t cooldown = 2) {
    PRACTICE_TODO();
}

// Stage 2, generalized in Stage 3
inline std::vector<std::size_t> burst_plan(std::span<const Slot> slots,
                                           std::size_t cooldown = 2) {
    PRACTICE_TODO();
}

// Stage 4
inline std::int64_t max_throughput_limited(std::span<const Slot> slots, std::size_t cooldown,
                                           std::size_t max_bursts) {
    PRACTICE_TODO();
}
