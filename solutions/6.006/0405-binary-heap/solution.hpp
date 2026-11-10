#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace detail {

// Restores the heap property for the subtree at `index` of the implicit tree over [first, first + count).
template <typename It, typename Compare>
void sift_down(It first, std::size_t count, std::size_t index, Compare& compare) {
    while (true) {
        const std::size_t left = 2 * index + 1;
        if (left >= count) {
            return;
        }
        std::size_t child = left;
        const std::size_t right = left + 1;
        if (right < count && compare(first[static_cast<std::ptrdiff_t>(left)], first[static_cast<std::ptrdiff_t>(right)])) {
            child = right;
        }
        if (!compare(first[static_cast<std::ptrdiff_t>(index)], first[static_cast<std::ptrdiff_t>(child)])) {
            return;
        }
        std::iter_swap(first + static_cast<std::ptrdiff_t>(index), first + static_cast<std::ptrdiff_t>(child));
        index = child;
    }
}

template <typename It, typename Compare>
void sift_up(It first, std::size_t index, Compare& compare) {
    while (index > 0) {
        const std::size_t parent = (index - 1) / 2;
        if (!compare(first[static_cast<std::ptrdiff_t>(parent)], first[static_cast<std::ptrdiff_t>(index)])) {
            return;
        }
        std::iter_swap(first + static_cast<std::ptrdiff_t>(parent), first + static_cast<std::ptrdiff_t>(index));
        index = parent;
    }
}

template <typename It, typename Compare>
void build_heap(It first, std::size_t count, Compare& compare) {
    for (std::size_t index = count / 2; index-- > 0;) {
        sift_down(first, count, index, compare);
    }
}

}  // namespace detail

template <typename T, typename Compare = std::less<T>>
class BinaryHeap {
public:
    explicit BinaryHeap(Compare compare = Compare{}) : compare_(std::move(compare)) {}

    explicit BinaryHeap(std::vector<T> items, Compare compare = Compare{})
        : items_(std::move(items)), compare_(std::move(compare)) {
        detail::build_heap(items_.begin(), items_.size(), compare_);
    }

    void push(T value) {
        items_.push_back(std::move(value));
        detail::sift_up(items_.begin(), items_.size() - 1, compare_);
    }

    const T& top() const {
        if (items_.empty()) {
            throw std::out_of_range("top of empty heap");
        }
        return items_.front();
    }

    T pop() {
        if (items_.empty()) {
            throw std::out_of_range("pop from empty heap");
        }
        std::iter_swap(items_.begin(), items_.end() - 1);
        T value = std::move(items_.back());
        items_.pop_back();
        detail::sift_down(items_.begin(), items_.size(), 0, compare_);
        return value;
    }

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }
    std::span<const T> items() const { return items_; }

private:
    std::vector<T> items_;
    Compare compare_;
};

template <std::random_access_iterator It, typename Compare = std::less<>>
void heapsort(It first, It last, Compare compare = Compare{}) {
    auto count = static_cast<std::size_t>(last - first);
    detail::build_heap(first, count, compare);
    while (count > 1) {
        --count;
        std::iter_swap(first, first + static_cast<std::ptrdiff_t>(count));
        detail::sift_down(first, count, 0, compare);
    }
}

template <typename T, typename Compare = std::less<T>>
class ReorderBuffer {
public:
    explicit ReorderBuffer(std::size_t lateness, Compare compare = Compare{})
        : lateness_(lateness), heap_(Flipped{std::move(compare)}) {}

    std::optional<T> push(T item) {
        heap_.push(std::move(item));
        if (heap_.size() > lateness_) {
            return heap_.pop();
        }
        return std::nullopt;
    }

    std::vector<T> drain() {
        std::vector<T> rest;
        rest.reserve(heap_.size());
        while (!heap_.empty()) {
            rest.push_back(heap_.pop());
        }
        return rest;
    }

    std::size_t buffered() const { return heap_.size(); }

private:
    struct Flipped {
        Compare compare;
        bool operator()(const T& a, const T& b) const { return compare(b, a); }
    };

    std::size_t lateness_;
    BinaryHeap<T, Flipped> heap_;
};
