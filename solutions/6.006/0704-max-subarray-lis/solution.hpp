#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <ranges>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <vector>

struct Subarray {
    std::int64_t sum;
    std::size_t first;
    std::size_t last;
};

inline Subarray max_subarray(std::span<const std::int64_t> values) {
    if (values.empty()) {
        throw std::invalid_argument("max_subarray: empty input");
    }
    Subarray best{values[0], 0, 0};
    std::int64_t ending_here = values[0];
    std::size_t start = 0;
    for (std::size_t k = 1; k < values.size(); ++k) {
        if (ending_here < 0) {
            ending_here = values[k];
            start = k;
        } else {
            ending_here += values[k];
        }
        if (ending_here > best.sum) {
            best = {ending_here, start, k};
        }
    }
    return best;
}

template <std::ranges::random_access_range R>
std::vector<std::size_t> lis_indices(const R& values) {
    const std::size_t n = std::ranges::size(values);
    constexpr std::size_t kNone = std::numeric_limits<std::size_t>::max();
    // tails[k]: index of the smallest element that ends an increasing subsequence of length k + 1.
    std::vector<std::size_t> tails;
    std::vector<std::size_t> previous(n, kNone);
    for (std::size_t i = 0; i < n; ++i) {
        const auto slot = std::lower_bound(
            tails.begin(), tails.end(), i,
            [&](std::size_t a, std::size_t b) { return values[a] < values[b]; });
        previous[i] = slot == tails.begin() ? kNone : *(slot - 1);
        if (slot == tails.end()) {
            tails.push_back(i);
        } else {
            *slot = i;
        }
    }

    std::vector<std::size_t> indices(tails.size());
    std::size_t at = tails.empty() ? kNone : tails.back();
    for (std::size_t k = tails.size(); k-- > 0;) {
        indices[k] = at;
        at = previous[at];
    }
    return indices;
}

template <std::ranges::random_access_range R>
std::size_t lis_length(const R& values) {
    return lis_indices(values).size();
}

template <std::ranges::random_access_range R>
std::vector<std::size_t> keep_in_place(const R& before, const R& after) {
    std::unordered_map<std::ranges::range_value_t<R>, std::size_t> position;
    position.reserve(std::ranges::size(before));
    for (std::size_t i = 0; i < std::ranges::size(before); ++i) {
        position.emplace(before[i], i);
    }
    std::vector<std::size_t> ranks;
    ranks.reserve(std::ranges::size(after));
    for (std::size_t i = 0; i < std::ranges::size(after); ++i) {
        ranks.push_back(position.at(after[i]));
    }
    return lis_indices(ranks);
}
