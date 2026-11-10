#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <span>
#include <utility>
#include <vector>

#include "01_exec.hpp"

// Stage 1
template <typename T, typename Compare = std::less<>>
std::size_t partition_around(std::span<T> data, std::size_t pivot_index, Compare cmp = {}) {
    using std::swap;
    swap(data[pivot_index], data[data.size() - 1]);
    const T& pivot = data[data.size() - 1];
    std::size_t store = 0;
    for (std::size_t i = 0; i + 1 < data.size(); ++i) {
        if (cmp(data[i], pivot)) {
            swap(data[store], data[i]);
            ++store;
        }
    }
    swap(data[store], data[data.size() - 1]);
    return store;
}

namespace detail_0511 {

template <typename T, typename Compare>
std::size_t median_of_three(std::span<T> data, Compare& cmp) {
    const std::size_t a = 0;
    const std::size_t b = data.size() / 2;
    const std::size_t c = data.size() - 1;
    if (cmp(data[a], data[b])) {
        if (cmp(data[b], data[c])) {
            return b;
        }
        return cmp(data[a], data[c]) ? c : a;
    }
    if (cmp(data[a], data[c])) {
        return a;
    }
    return cmp(data[b], data[c]) ? c : b;
}

template <typename Exec, typename T, typename Compare>
void insertion_sort(Exec& exec, std::span<T> data, Compare& cmp) {
    for (std::size_t i = 1; i < data.size(); ++i) {
        T value = std::move(data[i]);
        std::size_t hole = i;
        while (hole > 0) {
            exec.charge();
            if (!cmp(value, data[hole - 1])) {
                break;
            }
            data[hole] = std::move(data[hole - 1]);
            --hole;
        }
        data[hole] = std::move(value);
    }
}

}  // namespace detail_0511

// Stage 3 (Stage 2 forwards here with cutoff 1).
template <typename Exec, typename T, typename Compare>
void quicksort(Exec& exec, std::span<T> data, Compare cmp, std::size_t cutoff) {
    if (data.size() < 2) {
        return;
    }
    if (data.size() <= cutoff) {
        detail_0511::insertion_sort(exec, data, cmp);
        return;
    }
    exec.charge(static_cast<std::int64_t>(data.size()));
    const std::size_t pivot = detail_0511::median_of_three(data, cmp);
    const std::size_t split = partition_around(data, pivot, cmp);
    // The pivot is already in place, so the two halves are disjoint.
    exec.fork2([&] { quicksort(exec, data.first(split), cmp, cutoff); },
               [&] { quicksort(exec, data.subspan(split + 1), cmp, cutoff); });
}

// Stage 2
template <typename Exec, typename T, typename Compare = std::less<>>
void quicksort(Exec& exec, std::span<T> data, Compare cmp = {}) {
    quicksort(exec, data, cmp, std::size_t{1});
}
