#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

struct Sample {
    std::int64_t value;
    std::int64_t weight;
};

template <typename T>
std::pair<std::size_t, std::size_t> partition3(std::span<T> values, T pivot) {
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

namespace select_detail {

template <typename T>
void insertion_sort(std::span<T> values) {
    for (std::size_t i = 1; i < values.size(); ++i) {
        for (std::size_t j = i; j > 0 && values[j] < values[j - 1]; --j) {
            std::swap(values[j], values[j - 1]);
        }
    }
}

template <typename T>
T select(std::span<T> values, std::size_t k);

template <typename T>
T median_of_medians(std::span<T> values) {
    std::vector<T> medians;
    medians.reserve(values.size() / 5 + 1);
    for (std::size_t start = 0; start < values.size(); start += 5) {
        const auto group = values.subspan(start, std::min<std::size_t>(5, values.size() - start));
        insertion_sort(group);
        medians.push_back(group[(group.size() - 1) / 2]);
    }
    return select(std::span<T>(medians), (medians.size() - 1) / 2);
}

template <typename T>
T select(std::span<T> values, std::size_t k) {
    while (true) {
        if (values.size() <= 5) {
            insertion_sort(values);
            return values[k];
        }
        T pivot = median_of_medians(values);
        const auto [less_end, greater_begin] = partition3(values, pivot);
        if (k < less_end) {
            values = values.first(less_end);
        } else if (k < greater_begin) {
            return pivot;
        } else {
            k -= greater_begin;
            values = values.subspan(greater_begin);
        }
    }
}

}  // namespace select_detail

template <typename T>
T select_kth(std::span<T> values, std::size_t k) {
    if (k >= values.size()) {
        throw std::out_of_range("select_kth: k out of range");
    }
    return select_detail::select(values, k);
}

inline std::int64_t weighted_median(std::span<const Sample> samples) {
    if (samples.empty()) {
        throw std::invalid_argument("weighted_median: no samples");
    }
    std::int64_t total = 0;
    for (const Sample& s : samples) {
        if (s.weight <= 0) {
            throw std::invalid_argument("weighted_median: weights must be positive");
        }
        total += s.weight;
    }

    std::vector<Sample> rest(samples.begin(), samples.end());
    std::int64_t needed = total / 2 + total % 2;
    while (true) {
        // The exact median halves the remaining samples, so the loop is geometric.
        std::vector<std::int64_t> keys;
        keys.reserve(rest.size());
        for (const Sample& s : rest) {
            keys.push_back(s.value);
        }
        const std::int64_t pivot = select_kth<std::int64_t>(keys, keys.size() / 2);

        std::size_t less_end = 0;
        std::size_t equal_end = 0;
        std::int64_t less_weight = 0;
        std::int64_t equal_weight = 0;
        for (std::size_t i = 0; i < rest.size(); ++i) {
            if (rest[i].value < pivot) {
                less_weight += rest[i].weight;
                std::swap(rest[equal_end], rest[i]);
                std::swap(rest[less_end], rest[equal_end]);
                ++less_end;
                ++equal_end;
            } else if (rest[i].value == pivot) {
                equal_weight += rest[i].weight;
                std::swap(rest[equal_end], rest[i]);
                ++equal_end;
            }
        }

        if (needed <= less_weight) {
            rest.resize(less_end);
        } else if (needed <= less_weight + equal_weight) {
            return pivot;
        } else {
            needed -= less_weight + equal_weight;
            rest.erase(rest.begin(), rest.begin() + static_cast<std::ptrdiff_t>(equal_end));
        }
    }
}
