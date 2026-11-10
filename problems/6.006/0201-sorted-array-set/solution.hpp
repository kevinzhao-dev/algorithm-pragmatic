#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

// Stage 1
template <typename T>
std::size_t lower_bound_index(std::span<const T> sorted, const T& key) {
    PRACTICE_TODO();
}

template <typename T>
std::size_t upper_bound_index(std::span<const T> sorted, const T& key) {
    PRACTICE_TODO();
}

// Stages 2-4
template <typename Key, typename Value>
class SortedArraySet {
public:
    using Item = std::pair<Key, Value>;

    // Stage 2
    SortedArraySet() { PRACTICE_TODO(); }
    explicit SortedArraySet(std::vector<Item> items) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }

    const Item* find(const Key& key) const { PRACTICE_TODO(); }
    const Item* find_min() const { PRACTICE_TODO(); }
    const Item* find_max() const { PRACTICE_TODO(); }
    const Item* find_next(const Key& key) const { PRACTICE_TODO(); }
    const Item* find_prev(const Key& key) const { PRACTICE_TODO(); }

    std::span<const Item> items() const { PRACTICE_TODO(); }

    // Stage 3
    bool insert(Item item) { PRACTICE_TODO(); }
    std::optional<Item> erase(const Key& key) { PRACTICE_TODO(); }

    // Stage 4
    std::span<const Item> range(const Key& lo, const Key& hi) const { PRACTICE_TODO(); }
    const Item* as_of(const Key& t) const { PRACTICE_TODO(); }
};
