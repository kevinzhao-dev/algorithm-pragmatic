#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Less = std::less<T>>
void selection_sort(std::span<T> items, Less less = {}) {
    for (std::size_t last = items.size(); last > 1; --last) {
        std::size_t largest = 0;
        for (std::size_t j = 1; j < last; ++j) {
            if (less(items[largest], items[j])) {
                largest = j;
            }
        }
        if (largest != last - 1) {
            using std::swap;
            swap(items[largest], items[last - 1]);
        }
    }
}

template <typename T, typename Less = std::less<T>>
void insertion_sort(std::span<T> items, Less less = {}) {
    for (std::size_t i = 1; i < items.size(); ++i) {
        std::size_t j = i;
        if (!less(items[j], items[j - 1])) {
            continue;
        }
        T value = std::move(items[j]);
        do {
            items[j] = std::move(items[j - 1]);
            --j;
        } while (j > 0 && less(value, items[j - 1]));
        items[j] = std::move(value);
    }
}

template <typename T, typename Less = std::less<T>>
class TopK {
public:
    explicit TopK(std::size_t k, Less less = {}) : k_(k), less_(std::move(less)) {
        if (k == 0) {
            throw std::invalid_argument("k must be at least 1");
        }
        items_.reserve(k);
    }

    void push(T value) {
        if (items_.size() == k_) {
            if (!less_(items_.back(), value)) {
                return;  // not better than the current k-th item
            }
            items_.pop_back();
        }
        // Items sit in descending order; walk from the end past everything smaller than value,
        // so equal items keep their arrival order.
        std::size_t position = items_.size();
        items_.emplace_back(std::move(value));
        T incoming = std::move(items_.back());
        while (position > 0 && less_(items_[position - 1], incoming)) {
            items_[position] = std::move(items_[position - 1]);
            --position;
        }
        items_[position] = std::move(incoming);
    }

    std::span<const T> items() const { return items_; }
    std::size_t size() const { return items_.size(); }

private:
    std::size_t k_;
    Less less_;
    std::vector<T> items_;
};
