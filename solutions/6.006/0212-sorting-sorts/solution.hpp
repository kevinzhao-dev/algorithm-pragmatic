#pragma once

#include <cstddef>
#include <functional>
#include <span>
#include <utility>
#include <vector>

// Selection sort: at most n-1 swaps, i.e. 2(n-1) writes.
template <typename Seq>
void sort_expensive_writes(Seq& seq) {
    for (std::size_t last = seq.size(); last > 1; --last) {
        std::size_t largest = 0;
        for (std::size_t j = 1; j < last; ++j) {
            if (seq.get_at(largest) < seq.get_at(j)) {
                largest = j;
            }
        }
        if (largest != last - 1) {
            auto moved = seq.get_at(largest);
            seq.set_at(largest, seq.get_at(last - 1));
            seq.set_at(last - 1, std::move(moved));
        }
    }
}

namespace detail {

template <typename T, typename Less>
void merge_sort_pointers(std::span<const T*> items, std::vector<const T*>& buffer, Less& less) {
    const std::size_t n = items.size();
    if (n < 2) {
        return;
    }
    const std::size_t middle = n / 2;
    merge_sort_pointers(items.first(middle), buffer, less);
    merge_sort_pointers(items.subspan(middle), buffer, less);
    buffer.assign(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(middle));
    std::size_t left = 0;
    std::size_t right = middle;
    std::size_t out = 0;
    while (left < buffer.size() && right < n) {
        if (less(*items[right], *buffer[left])) {
            items[out++] = items[right++];
        } else {
            items[out++] = buffer[left++];
        }
    }
    while (left < buffer.size()) {
        items[out++] = buffer[left++];
    }
}

}  // namespace detail

template <typename T, typename Less>
void sort_pointers(std::span<const T*> pointers, Less less) {
    std::vector<const T*> buffer;
    buffer.reserve(pointers.size() / 2 + 1);
    detail::merge_sort_pointers(pointers, buffer, less);
}

// Insertion sort: O(n + inversions) comparisons, and k adjacent swaps create at most k inversions.
template <typename T, typename Less = std::less<T>>
void repair_nearly_sorted(std::span<T> items, Less less = {}) {
    for (std::size_t i = 1; i < items.size(); ++i) {
        if (!less(items[i], items[i - 1])) {
            continue;
        }
        T value = std::move(items[i]);
        std::size_t j = i;
        do {
            items[j] = std::move(items[j - 1]);
            --j;
        } while (j > 0 && less(value, items[j - 1]));
        items[j] = std::move(value);
    }
}
