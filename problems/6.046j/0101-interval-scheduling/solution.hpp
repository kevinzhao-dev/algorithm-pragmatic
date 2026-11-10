#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

struct Request {
    std::int64_t start;
    std::int64_t finish;
    std::int64_t value;
};

struct Schedule {
    std::int64_t total = 0;
    std::vector<int> chosen;
};

// Stage 1
inline std::vector<int> max_compatible(std::span<const Request> requests) { PRACTICE_TODO(); }

// Stage 2
inline Schedule max_value_schedule(std::span<const Request> requests) { PRACTICE_TODO(); }

// Stage 3
class WhatIfPlanner {
public:
    explicit WhatIfPlanner(std::span<const Request> requests) { PRACTICE_TODO(); }

    std::int64_t best_total() const { PRACTICE_TODO(); }
    std::int64_t best_with(std::size_t index) const { PRACTICE_TODO(); }
    std::int64_t regret(std::size_t index) const { PRACTICE_TODO(); }
};
