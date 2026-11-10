#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::int64_t max_compaction_score(std::span<const int> records) { PRACTICE_TODO(); }

// Stage 2
inline std::int64_t max_full_compaction_score(std::span<const int> records) { PRACTICE_TODO(); }

// Stage 3
struct Removal {
    std::size_t first;
    std::size_t second;  // first == second: removed alone; first < second: removed as a pair
};

struct CompactionPlan {
    std::int64_t score;
    std::vector<Removal> removals;
};

inline CompactionPlan compaction_plan(std::span<const int> records) { PRACTICE_TODO(); }

// Stage 4
inline std::vector<std::pair<std::size_t, std::size_t>> max_noncrossing_wires(
    std::span<const bool> is_source) {
    PRACTICE_TODO();
}
