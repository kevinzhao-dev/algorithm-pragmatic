#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
inline bool subset_sum(std::span<const int> values, int target) { PRACTICE_TODO(); }

// Stage 2
inline std::optional<std::vector<std::size_t>> subset_sum_witness(std::span<const int> values,
                                                                  int target) {
    PRACTICE_TODO();
}

// Stage 3
class SumSet {
public:
    explicit SumSet(std::size_t limit) { PRACTICE_TODO(); }

    void add(std::size_t value) { PRACTICE_TODO(); }
    bool contains(std::size_t sum) const { PRACTICE_TODO(); }
    std::size_t count() const { PRACTICE_TODO(); }
};

// Stage 4
struct Partition {
    std::int64_t difference;
    std::vector<std::size_t> left;
};

inline Partition closest_partition(std::span<const int> values) { PRACTICE_TODO(); }

// Stage 5
inline bool subset_sum_signed(std::span<const int> values, std::int64_t target) {
    PRACTICE_TODO();
}
