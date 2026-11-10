#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <vector>

#include "practice.hpp"

struct Assignment {
    std::int64_t makespan;
    std::vector<int> machine;
};

// Stage 1
inline Assignment balance_two(std::span<const int> jobs) { PRACTICE_TODO(); }

// Stage 2
inline std::int64_t min_makespan_four(std::span<const int> jobs) { PRACTICE_TODO(); }

// Stage 3
inline Assignment balance_four(std::span<const int> jobs) { PRACTICE_TODO(); }

// Stage 4
struct Plan {
    std::int64_t makespan;
    std::vector<int> machine;
    bool exact;
};

inline Plan plan_jobs(std::span<const int> jobs, std::size_t machines, std::uint64_t work_limit) {
    PRACTICE_TODO();
}
