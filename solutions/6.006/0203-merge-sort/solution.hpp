#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T, typename Less = std::less<T>>
std::vector<T> merge_sorted(std::span<const T> a, std::span<const T> b, Less less = {}) {
    std::vector<T> out;
    out.reserve(a.size() + b.size());
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < a.size() && j < b.size()) {
        if (less(b[j], a[i])) {
            out.push_back(b[j++]);
        } else {
            out.push_back(a[i++]);
        }
    }
    out.insert(out.end(), a.begin() + static_cast<std::ptrdiff_t>(i), a.end());
    out.insert(out.end(), b.begin() + static_cast<std::ptrdiff_t>(j), b.end());
    return out;
}

namespace detail {

template <typename T, typename Less>
void merge_sort_range(std::span<T> items, std::vector<T>& buffer, Less& less) {
    const std::size_t n = items.size();
    if (n < 2) {
        return;
    }
    const std::size_t middle = n / 2;
    merge_sort_range(items.first(middle), buffer, less);
    merge_sort_range(items.subspan(middle), buffer, less);
    if (!less(items[middle], items[middle - 1])) {
        return;  // already in order: skip the merge
    }

    buffer.clear();
    for (std::size_t i = 0; i < middle; ++i) {
        buffer.push_back(std::move(items[i]));
    }
    std::size_t left = 0;
    std::size_t right = middle;
    std::size_t out = 0;
    while (left < buffer.size() && right < n) {
        if (less(items[right], buffer[left])) {
            items[out++] = std::move(items[right++]);
        } else {
            items[out++] = std::move(buffer[left++]);
        }
    }
    while (left < buffer.size()) {
        items[out++] = std::move(buffer[left++]);
    }
    // Any elements left in the right half are already in place.
}

// Sorts items and returns the inversion count; `buffer` is a single reusable scratch array.
inline std::uint64_t sort_and_count(std::span<int> items, std::vector<int>& buffer) {
    const std::size_t n = items.size();
    if (n < 2) {
        return 0;
    }
    const std::size_t middle = n / 2;
    std::uint64_t inversions = sort_and_count(items.first(middle), buffer);
    inversions += sort_and_count(items.subspan(middle), buffer);

    buffer.assign(items.begin(), items.begin() + static_cast<std::ptrdiff_t>(middle));
    std::size_t left = 0;
    std::size_t right = middle;
    std::size_t out = 0;
    while (left < buffer.size() && right < n) {
        if (items[right] < buffer[left]) {
            // items[right] jumps over every element remaining on the left.
            inversions += buffer.size() - left;
            items[out++] = items[right++];
        } else {
            items[out++] = buffer[left++];
        }
    }
    while (left < buffer.size()) {
        items[out++] = buffer[left++];
    }
    return inversions;
}

}  // namespace detail

template <typename T, typename Less = std::less<T>>
void merge_sort(std::span<T> items, Less less = {}) {
    std::vector<T> buffer;
    buffer.reserve(items.size() / 2 + 1);
    detail::merge_sort_range(items, buffer, less);
}

inline std::uint64_t count_inversions(std::span<const int> values) {
    std::vector<int> copy(values.begin(), values.end());
    std::vector<int> buffer;
    buffer.reserve(copy.size() / 2 + 1);
    return detail::sort_and_count(copy, buffer);
}

inline std::uint64_t kendall_tau_distance(std::span<const int> rank_a, std::span<const int> rank_b) {
    if (rank_a.size() != rank_b.size()) {
        throw std::invalid_argument("rankings have different lengths");
    }
    const std::size_t n = rank_a.size();
    const auto positions_of = [n](std::span<const int> rank) {
        std::vector<int> position(n, -1);
        for (std::size_t i = 0; i < n; ++i) {
            const int item = rank[i];
            if (item < 0 || static_cast<std::size_t>(item) >= n ||
                position[static_cast<std::size_t>(item)] != -1) {
                throw std::invalid_argument("not a permutation of 0..n-1");
            }
            position[static_cast<std::size_t>(item)] = static_cast<int>(i);
        }
        return position;
    };
    const std::vector<int> position_in_b = positions_of(rank_b);
    positions_of(rank_a);
    std::vector<int> relabeled(n);
    for (std::size_t i = 0; i < n; ++i) {
        relabeled[i] = position_in_b[static_cast<std::size_t>(rank_a[i])];
    }
    return count_inversions(relabeled);
}
