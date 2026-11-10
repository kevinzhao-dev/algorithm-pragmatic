#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "../0405-binary-heap/solution.hpp"

template <typename Score>
struct Garden {
    Score score;
    int id;
};

namespace detail {

template <typename Score>
struct ByScore {
    bool operator()(const Garden<Score>& a, const Garden<Score>& b) const { return a.score < b.score; }
};

}  // namespace detail

// Heapify once (O(n)) and pop k times (O(k log n)) instead of sorting everything.
template <typename Score>
std::vector<int> top_k_ids(std::span<const Garden<Score>> gardens, std::size_t k) {
    BinaryHeap<Garden<Score>, detail::ByScore<Score>> heap(
        std::vector<Garden<Score>>(gardens.begin(), gardens.end()));
    std::vector<int> ids;
    ids.reserve(k);
    for (std::size_t i = 0; i < k; ++i) {
        ids.push_back(heap.pop().id);
    }
    return ids;
}

// Every answer is reachable from the root through answers, so only their children get visited.
template <typename Score>
std::vector<int> ids_above(std::span<const Garden<Score>> heap, const Score& threshold) {
    std::vector<int> ids;
    std::vector<std::size_t> stack;
    if (!heap.empty()) {
        stack.push_back(0);
    }
    while (!stack.empty()) {
        const std::size_t index = stack.back();
        stack.pop_back();
        if (!(threshold < heap[index].score)) {
            continue;
        }
        ids.push_back(heap[index].id);
        for (std::size_t child : {2 * index + 1, 2 * index + 2}) {
            if (child < heap.size()) {
                stack.push_back(child);
            }
        }
    }
    return ids;
}

// Keeps the k largest elements seen; the kept set lives in a heap whose top is the weakest.
template <typename T, typename Compare = std::less<T>>
class TopKTracker {
public:
    explicit TopKTracker(std::size_t k, Compare compare = Compare{})
        : k_(k), compare_(std::move(compare)), kept_(Flipped{compare_}) {}

    void offer(T value) {
        if (k_ == 0) {
            return;
        }
        if (kept_.size() < k_) {
            kept_.push(std::move(value));
        } else if (compare_(kept_.top(), value)) {
            kept_.pop();
            kept_.push(std::move(value));
        }
    }

    std::size_t size() const { return kept_.size(); }

    const T* threshold() const {
        return kept_.size() == k_ && k_ != 0 ? &kept_.top() : nullptr;
    }

    std::vector<T> sorted_largest() const {
        std::vector<T> out(kept_.items().begin(), kept_.items().end());
        std::sort(out.begin(), out.end(), [this](const T& a, const T& b) { return compare_(b, a); });
        return out;
    }

private:
    struct Flipped {
        Compare compare;
        bool operator()(const T& a, const T& b) const { return compare(b, a); }
    };

    std::size_t k_;
    Compare compare_;
    BinaryHeap<T, Flipped> kept_;
};
