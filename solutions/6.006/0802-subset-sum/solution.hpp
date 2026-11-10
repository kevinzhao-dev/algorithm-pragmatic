#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <optional>
#include <span>
#include <vector>

inline bool subset_sum(std::span<const int> values, int target) {
    std::vector<char> reachable(static_cast<std::size_t>(target) + 1, 0);
    reachable[0] = 1;
    for (const int value : values) {
        for (int sum = target; sum >= value; --sum) {
            reachable[static_cast<std::size_t>(sum)] |=
                reachable[static_cast<std::size_t>(sum - value)];
        }
    }
    return reachable[static_cast<std::size_t>(target)] != 0;
}

namespace detail {

// For each sum in [0, limit], the 1-based index of the first item that makes it reachable, or -1
// if it never becomes reachable; sum 0 is reachable before any item.
inline std::vector<int> first_item_reaching(std::span<const int> values, std::size_t limit) {
    std::vector<int> first(limit + 1, -1);
    first[0] = 0;
    for (std::size_t k = 0; k < values.size(); ++k) {
        const auto value = static_cast<std::size_t>(values[k]);
        // Descending, so that this item cannot be used twice.
        for (std::size_t sum = limit; sum + 1 > value; --sum) {
            if (first[sum] == -1 && first[sum - value] != -1) {
                first[sum] = static_cast<int>(k) + 1;
            }
        }
    }
    return first;
}

// Item `k` made `sum` newly reachable, so `sum - values[k]` was reachable from earlier items only.
inline std::vector<std::size_t> indices_for_sum(const std::vector<int>& first,
                                                std::span<const int> values, std::size_t sum) {
    std::vector<std::size_t> indices;
    while (sum > 0) {
        const auto k = static_cast<std::size_t>(first[sum]) - 1;
        indices.push_back(k);
        sum -= static_cast<std::size_t>(values[k]);
    }
    std::ranges::reverse(indices);
    return indices;
}

}  // namespace detail

inline std::optional<std::vector<std::size_t>> subset_sum_witness(std::span<const int> values,
                                                                  int target) {
    const auto first = detail::first_item_reaching(values, static_cast<std::size_t>(target));
    if (first[static_cast<std::size_t>(target)] == -1) {
        return std::nullopt;
    }
    return detail::indices_for_sum(first, values, static_cast<std::size_t>(target));
}

class SumSet {
public:
    explicit SumSet(std::size_t limit)
        : limit_(limit), words_(limit / kWordBits + 1, 0), top_mask_(make_top_mask(limit)) {
        words_[0] = 1;
    }

    void add(std::size_t value) {
        if (value == 0 || value > limit_) {
            return;
        }
        const std::size_t word_shift = value / kWordBits;
        const std::size_t bit_shift = value % kWordBits;
        for (std::size_t i = words_.size(); i-- > word_shift;) {
            std::uint64_t shifted = words_[i - word_shift] << bit_shift;
            if (bit_shift != 0 && i > word_shift) {
                shifted |= words_[i - word_shift - 1] >> (kWordBits - bit_shift);
            }
            words_[i] |= shifted;
        }
        words_.back() &= top_mask_;
    }

    bool contains(std::size_t sum) const {
        if (sum > limit_) {
            return false;
        }
        return (words_[sum / kWordBits] >> (sum % kWordBits)) & 1;
    }

    std::size_t count() const {
        std::size_t total = 0;
        for (const std::uint64_t word : words_) {
            total += static_cast<std::size_t>(std::popcount(word));
        }
        return total;
    }

private:
    static constexpr std::size_t kWordBits = 64;

    static std::uint64_t make_top_mask(std::size_t limit) {
        const std::size_t used = limit % kWordBits;
        return used == kWordBits - 1 ? ~std::uint64_t{0} : (std::uint64_t{1} << (used + 1)) - 1;
    }

    std::size_t limit_;
    std::vector<std::uint64_t> words_;
    std::uint64_t top_mask_;
};

struct Partition {
    std::int64_t difference;
    std::vector<std::size_t> left;
};

inline Partition closest_partition(std::span<const int> values) {
    const std::int64_t total = std::accumulate(values.begin(), values.end(), std::int64_t{0});
    const auto half = static_cast<std::size_t>(total / 2);
    const auto first = detail::first_item_reaching(values, half);
    std::size_t best = half;
    while (first[best] == -1) {
        --best;
    }
    return {total - 2 * static_cast<std::int64_t>(best), detail::indices_for_sum(first, values, best)};
}

inline bool subset_sum_signed(std::span<const int> values, std::int64_t target) {
    std::int64_t low = 0;
    std::int64_t high = 0;
    for (const int value : values) {
        (value < 0 ? low : high) += value;
    }
    if (target < low || target > high) {
        return false;
    }
    const auto offset = [&](std::int64_t sum) { return static_cast<std::size_t>(sum - low); };
    std::vector<char> reachable(offset(high) + 1, 0);
    reachable[offset(0)] = 1;
    for (const int value : values) {
        if (value > 0) {
            for (std::int64_t sum = high; sum >= low + value; --sum) {
                reachable[offset(sum)] |= reachable[offset(sum - value)];
            }
        } else if (value < 0) {
            for (std::int64_t sum = low; sum <= high + value; ++sum) {
                reachable[offset(sum)] |= reachable[offset(sum - value)];
            }
        }
    }
    return reachable[offset(target)] != 0;
}
