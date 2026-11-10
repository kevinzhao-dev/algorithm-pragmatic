#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::int64_t rod_cut_value(std::span<const std::int64_t> prices) { PRACTICE_TODO(); }

// Stage 2
inline std::vector<std::size_t> rod_cut_pieces(std::span<const std::int64_t> prices) {
    PRACTICE_TODO();
}

// Stage 3
struct Item {
    std::size_t size;
    std::int64_t value;
};

inline std::int64_t unbounded_knapsack(std::span<const Item> items, std::size_t capacity) {
    PRACTICE_TODO();
}

// Stage 4
inline std::optional<std::int64_t> min_coins(std::span<const int> coins, int amount) {
    PRACTICE_TODO();
}

inline std::uint64_t count_change_ways(std::span<const int> coins, int amount) { PRACTICE_TODO(); }

inline std::uint64_t count_ordered_ways(std::span<const int> coins, int amount) { PRACTICE_TODO(); }

// Stage 5
struct Offer {
    std::int64_t units;
    std::int64_t price;
};

struct Purchase {
    std::int64_t cost;
    std::vector<std::int64_t> counts;
};

inline std::optional<Purchase> cheapest_capacity(std::span<const Offer> offers,
                                                 std::int64_t demand) {
    PRACTICE_TODO();
}
