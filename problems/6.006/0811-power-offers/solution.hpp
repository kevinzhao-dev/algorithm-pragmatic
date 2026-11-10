#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

struct Order {
    std::int64_t amount;
    std::int64_t price;
};

// Stage 1, tightened in Stage 3
inline std::int64_t max_profit(std::span<const Order> orders, std::int64_t capacity,
                               std::int64_t storage_cost) {
    PRACTICE_TODO();
}

// Stage 2
inline std::vector<std::size_t> chosen_orders(std::span<const Order> orders, std::int64_t capacity,
                                              std::int64_t storage_cost) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<std::int64_t> profit_curve(std::span<const Order> orders, std::int64_t capacity,
                                              std::int64_t storage_cost) {
    PRACTICE_TODO();
}
