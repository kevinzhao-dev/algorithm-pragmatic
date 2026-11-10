#pragma once

#include <algorithm>
#include <cstddef>
#include <random>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sorting_detail {

// Dutch national flag partition: [0, less_end) < pivot, [less_end, greater_begin) == pivot.
template <typename T>
std::pair<std::size_t, std::size_t> partition3(std::span<T> values, const T& pivot) {
    std::size_t less_end = 0;
    std::size_t index = 0;
    std::size_t greater_begin = values.size();
    while (index < greater_begin) {
        if (values[index] < pivot) {
            std::swap(values[less_end], values[index]);
            ++less_end;
            ++index;
        } else if (pivot < values[index]) {
            --greater_begin;
            std::swap(values[index], values[greater_begin]);
        } else {
            ++index;
        }
    }
    return {less_end, greater_begin};
}

template <typename Rng>
std::size_t random_index(std::size_t n, Rng& rng) {
    return std::uniform_int_distribution<std::size_t>(0, n - 1)(rng);
}

}  // namespace sorting_detail

template <typename T, std::uniform_random_bit_generator Rng>
void randomized_quicksort(std::span<T> values, Rng& rng) {
    while (values.size() > 1) {
        const T pivot = values[sorting_detail::random_index(values.size(), rng)];
        const auto [less_end, greater_begin] = sorting_detail::partition3(values, pivot);
        // Recursing only into the smaller side keeps the stack at O(log n).
        if (less_end < values.size() - greater_begin) {
            randomized_quicksort(values.first(less_end), rng);
            values = values.subspan(greater_begin);
        } else {
            randomized_quicksort(values.subspan(greater_begin), rng);
            values = values.first(less_end);
        }
    }
}

template <typename T, std::uniform_random_bit_generator Rng>
int paranoid_quicksort(std::span<T> values, Rng& rng) {
    const std::size_t n = values.size();
    if (n <= 1) {
        return 0;
    }
    std::size_t less_end = 0;
    std::size_t greater_begin = 0;
    while (true) {
        const T pivot = values[sorting_detail::random_index(n, rng)];
        std::tie(less_end, greater_begin) = sorting_detail::partition3(values, pivot);
        if (4 * less_end <= 3 * n && 4 * (n - greater_begin) <= 3 * n) {
            break;
        }
    }
    return 1 + std::max(paranoid_quicksort(values.first(less_end), rng),
                        paranoid_quicksort(values.subspan(greater_begin), rng));
}

template <typename T, std::uniform_random_bit_generator Rng>
T randomized_select(std::span<T> values, std::size_t k, Rng& rng) {
    if (k >= values.size()) {
        throw std::out_of_range("randomized_select: k out of range");
    }
    std::span<T> range = values;
    while (true) {
        const T pivot = range[sorting_detail::random_index(range.size(), rng)];
        const auto [less_end, greater_begin] = sorting_detail::partition3(range, pivot);
        if (k < less_end) {
            range = range.first(less_end);
        } else if (k < greater_begin) {
            return pivot;
        } else {
            k -= greater_begin;
            range = range.subspan(greater_begin);
        }
    }
}

template <typename T, std::uniform_random_bit_generator Rng>
std::vector<T> top_k(std::span<T> values, std::size_t k, Rng& rng) {
    if (k > values.size()) {
        throw std::out_of_range("top_k: k out of range");
    }
    if (k == 0) {
        return {};
    }
    randomized_select(values, values.size() - k, rng);
    std::vector<T> result(values.end() - static_cast<std::ptrdiff_t>(k), values.end());
    std::sort(result.begin(), result.end(), [](const T& a, const T& b) { return b < a; });
    return result;
}
