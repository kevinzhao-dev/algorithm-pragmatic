#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename HashFn>
std::vector<std::vector<std::int64_t>> chain_layout(std::span<const std::int64_t> keys,
                                                    std::size_t buckets, HashFn hash) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<std::int64_t> smallest_injective_modulus(std::span<const std::int64_t> keys,
                                                              std::int64_t a, std::int64_t b,
                                                              std::int64_t buckets,
                                                              std::int64_t limit) {
    PRACTICE_TODO();
}

// Stage 3
class PerfectHashSet {
public:
    explicit PerfectHashSet(std::span<const std::int64_t> keys) { PRACTICE_TODO(); }

    bool contains(std::int64_t key) const { PRACTICE_TODO(); }
    std::size_t table_size() const { PRACTICE_TODO(); }
    std::size_t slot_of(std::int64_t key) const { PRACTICE_TODO(); }
};
