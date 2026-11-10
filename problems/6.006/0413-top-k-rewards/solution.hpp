#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <vector>

#include "practice.hpp"

template <typename Score>
struct Garden {
    Score score;
    int id;
};

// Stage 1
template <typename Score>
std::vector<int> top_k_ids(std::span<const Garden<Score>> gardens, std::size_t k) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Score>
std::vector<int> ids_above(std::span<const Garden<Score>> heap, const Score& threshold) {
    PRACTICE_TODO();
}

// Stage 3
template <typename T, typename Compare = std::less<T>>
class TopKTracker {
public:
    explicit TopKTracker(std::size_t k, Compare compare = Compare{}) { PRACTICE_TODO(); }

    void offer(T value) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    const T* threshold() const { PRACTICE_TODO(); }
    std::vector<T> sorted_largest() const { PRACTICE_TODO(); }
};
