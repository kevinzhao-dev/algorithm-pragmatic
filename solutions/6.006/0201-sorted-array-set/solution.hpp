#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <utility>
#include <vector>

template <typename T>
std::size_t lower_bound_index(std::span<const T> sorted, const T& key) {
    std::size_t low = 0;
    std::size_t high = sorted.size();
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        if (sorted[middle] < key) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

template <typename T>
std::size_t upper_bound_index(std::span<const T> sorted, const T& key) {
    std::size_t low = 0;
    std::size_t high = sorted.size();
    while (low < high) {
        const std::size_t middle = low + (high - low) / 2;
        if (key < sorted[middle]) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return low;
}

template <typename Key, typename Value>
class SortedArraySet {
public:
    using Item = std::pair<Key, Value>;

    SortedArraySet() = default;

    explicit SortedArraySet(std::vector<Item> items) : items_(std::move(items)) {
        std::stable_sort(items_.begin(), items_.end(), [](const Item& a, const Item& b) {
            return a.first < b.first;
        });
        // Keep the last occurrence of each key.
        std::size_t kept = 0;
        for (std::size_t i = 0; i < items_.size(); ++i) {
            if (i + 1 == items_.size() || items_[i].first < items_[i + 1].first) {
                if (kept != i) {
                    items_[kept] = std::move(items_[i]);
                }
                ++kept;
            }
        }
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(kept), items_.end());
    }

    std::size_t size() const { return items_.size(); }

    const Item* find(const Key& key) const {
        const std::size_t index = position(key);
        return index < items_.size() && !(key < items_[index].first) ? &items_[index] : nullptr;
    }

    const Item* find_min() const { return items_.empty() ? nullptr : &items_.front(); }
    const Item* find_max() const { return items_.empty() ? nullptr : &items_.back(); }

    const Item* find_next(const Key& key) const {
        const std::size_t index = position_after(key);
        return index < items_.size() ? &items_[index] : nullptr;
    }

    const Item* find_prev(const Key& key) const {
        const std::size_t index = position(key);
        return index > 0 ? &items_[index - 1] : nullptr;
    }

    std::span<const Item> items() const { return items_; }

    bool insert(Item item) {
        const std::size_t index = position(item.first);
        if (index < items_.size() && !(item.first < items_[index].first)) {
            items_[index].second = std::move(item.second);
            return false;
        }
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(index), std::move(item));
        return true;
    }

    std::optional<Item> erase(const Key& key) {
        const std::size_t index = position(key);
        if (index == items_.size() || key < items_[index].first) {
            return std::nullopt;
        }
        Item removed = std::move(items_[index]);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(index));
        return removed;
    }

    std::span<const Item> range(const Key& lo, const Key& hi) const {
        if (hi < lo) {
            return {};
        }
        const std::size_t first = position(lo);
        const std::size_t last = position(hi);
        return std::span<const Item>(items_).subspan(first, last - first);
    }

    const Item* as_of(const Key& t) const {
        const std::size_t index = position_after(t);
        return index > 0 ? &items_[index - 1] : nullptr;
    }

private:
    // First index whose key is not less than `key`.
    std::size_t position(const Key& key) const {
        std::size_t low = 0;
        std::size_t high = items_.size();
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2;
            if (items_[middle].first < key) {
                low = middle + 1;
            } else {
                high = middle;
            }
        }
        return low;
    }

    // First index whose key is greater than `key`.
    std::size_t position_after(const Key& key) const {
        std::size_t low = 0;
        std::size_t high = items_.size();
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2;
            if (key < items_[middle].first) {
                high = middle;
            } else {
                low = middle + 1;
            }
        }
        return low;
    }

    std::vector<Item> items_;
};
