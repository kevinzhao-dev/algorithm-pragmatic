#pragma once

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

enum class HeapKind { Max, Min, Both, Neither };

using SwapStep = std::pair<std::size_t, std::size_t>;

// Stage 1
inline HeapKind classify_heap(std::span<const int> values) { PRACTICE_TODO(); }

// Stage 2
inline std::vector<SwapStep> min_heapify_trace(std::span<int> values) { PRACTICE_TODO(); }

// Stage 3
template <typename T>
const T& min_of_max_heap(std::span<const T> heap) {
    PRACTICE_TODO();
}

template <typename T>
void max_heap_to_min_heap(std::span<T> heap) {
    PRACTICE_TODO();
}
