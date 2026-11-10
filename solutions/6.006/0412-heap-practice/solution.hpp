#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

enum class HeapKind { Max, Min, Both, Neither };

inline HeapKind classify_heap(std::span<const int> values) {
    bool is_max = true;
    bool is_min = true;
    for (std::size_t i = 1; i < values.size(); ++i) {
        const int parent = values[(i - 1) / 2];
        is_max = is_max && parent >= values[i];
        is_min = is_min && parent <= values[i];
    }
    if (is_max && is_min) {
        return HeapKind::Both;
    }
    if (is_max) {
        return HeapKind::Max;
    }
    return is_min ? HeapKind::Min : HeapKind::Neither;
}

using SwapStep = std::pair<std::size_t, std::size_t>;

// Bottom-up min-heapify that records every swap.
inline std::vector<SwapStep> min_heapify_trace(std::span<int> values) {
    std::vector<SwapStep> trace;
    const std::size_t n = values.size();
    for (std::size_t start = n / 2; start-- > 0;) {
        std::size_t index = start;
        while (2 * index + 1 < n) {
            std::size_t child = 2 * index + 1;
            if (child + 1 < n && values[child + 1] < values[child]) {
                ++child;
            }
            if (values[child] >= values[index]) {
                break;
            }
            std::swap(values[index], values[child]);
            trace.emplace_back(index, child);
            index = child;
        }
    }
    return trace;
}

// The minimum of a max-heap can only be a leaf, and the leaves are the back half of the array.
template <typename T>
const T& min_of_max_heap(std::span<const T> heap) {
    if (heap.empty()) {
        throw std::invalid_argument("empty heap");
    }
    const T* best = &heap[heap.size() / 2];
    for (std::size_t i = heap.size() / 2 + 1; i < heap.size(); ++i) {
        if (heap[i] < *best) {
            best = &heap[i];
        }
    }
    return *best;
}

template <typename T>
void max_heap_to_min_heap(std::span<T> heap) {
    const std::size_t n = heap.size();
    for (std::size_t start = n / 2; start-- > 0;) {
        std::size_t index = start;
        while (2 * index + 1 < n) {
            std::size_t child = 2 * index + 1;
            if (child + 1 < n && heap[child + 1] < heap[child]) {
                ++child;
            }
            if (!(heap[child] < heap[index])) {
                break;
            }
            std::swap(heap[index], heap[child]);
            index = child;
        }
    }
}
