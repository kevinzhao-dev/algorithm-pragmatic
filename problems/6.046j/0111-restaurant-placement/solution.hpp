#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "practice.hpp"

struct Network {
    std::vector<std::int64_t> value;
    std::vector<std::pair<int, int>> edges;

    std::size_t size() const { return value.size(); }
};

// Stage 1
inline std::vector<int> greedy_by_value(const Network& network) { PRACTICE_TODO(); }
inline Network greedy_counterexample() { PRACTICE_TODO(); }

// Stage 2
inline std::vector<int> max_value_placement(const Network& tree) { PRACTICE_TODO(); }

// Stage 3
inline std::vector<int> max_count_placement(const Network& tree) { PRACTICE_TODO(); }

// Stage 4
inline std::vector<int> max_value_placement_general(const Network& network) { PRACTICE_TODO(); }
