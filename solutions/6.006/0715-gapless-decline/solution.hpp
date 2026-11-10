#pragma once

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

namespace detail {

inline constexpr std::size_t kNone = std::numeric_limits<std::size_t>::max();

// Best continuation found so far: its length, and where it starts.
struct Best {
    std::size_t length = 0;
    std::size_t at = kNone;
};

// Prefix maximum over positions; each update only raises a slot.
class PrefixMax {
public:
    explicit PrefixMax(std::size_t size) : tree_(size + 1) {}

    void raise(std::size_t position, Best value) {
        for (std::size_t i = position + 1; i < tree_.size(); i += i & (0 - i)) {
            if (value.length > tree_[i].length) {
                tree_[i] = value;
            }
        }
    }

    // Max over positions [0, end).
    Best query(std::size_t end) const {
        Best best;
        for (std::size_t i = end; i > 0; i -= i & (0 - i)) {
            if (tree_[i].length > best.length) {
                best = tree_[i];
            }
        }
        return best;
    }

private:
    std::vector<Best> tree_;
};

struct DeclineTable {
    std::vector<std::size_t> length;  // longest trend starting at each sample
    std::vector<std::size_t> next;    // following sample in that trend, or kNone
};

inline void check_shape(std::size_t size, std::size_t per_bucket) {
    if (per_bucket == 0 || size % per_bucket != 0) {
        throw std::invalid_argument("samples must fill a whole number of buckets");
    }
}

inline DeclineTable build_table(std::span<const int> samples, std::size_t per_bucket) {
    check_shape(samples.size(), per_bucket);
    const std::size_t total = samples.size();
    DeclineTable table{std::vector<std::size_t>(total, 1),
                       std::vector<std::size_t>(total, kNone)};

    // Per-bucket scratch, reused to avoid an allocation for every bucket.
    std::vector<std::size_t> order(per_bucket);
    std::vector<int> sorted_values;
    std::vector<int> following_values;  // values of the next bucket, ascending
    std::vector<Best> following_best;   // best trend among the smallest i+1 of those

    for (std::size_t bucket = total / per_bucket; bucket-- > 0;) {
        const std::size_t begin = bucket * per_bucket;
        const auto value = [&](std::size_t j) { return samples[begin + j]; };

        std::iota(order.begin(), order.end(), std::size_t{0});
        std::ranges::sort(order, [&](std::size_t a, std::size_t b) { return value(a) < value(b); });
        sorted_values.clear();
        for (const std::size_t j : order) {
            if (sorted_values.empty() || sorted_values.back() < value(j)) {
                sorted_values.push_back(value(j));
            }
        }

        PrefixMax same_bucket(sorted_values.size());
        for (std::size_t j = per_bucket; j-- > 0;) {
            const std::size_t at = begin + j;
            const std::size_t rank = static_cast<std::size_t>(
                std::ranges::lower_bound(sorted_values, value(j)) - sorted_values.begin());
            Best best = same_bucket.query(rank);
            if (!following_values.empty()) {
                const auto smaller = static_cast<std::size_t>(
                    std::ranges::lower_bound(following_values, value(j)) -
                    following_values.begin());
                if (smaller > 0 && following_best[smaller - 1].length > best.length) {
                    best = following_best[smaller - 1];
                }
            }
            table.length[at] = best.length + 1;
            table.next[at] = best.at;
            same_bucket.raise(rank, {table.length[at], at});
        }

        // Publish this bucket for the bucket before it.
        following_values.clear();
        following_best.clear();
        Best running;
        for (const std::size_t j : order) {
            const std::size_t at = begin + j;
            if (table.length[at] > running.length) {
                running = {table.length[at], at};
            }
            following_values.push_back(value(j));
            following_best.push_back(running);
        }
    }
    return table;
}

}  // namespace detail

inline std::vector<std::size_t> gapless_decline(std::span<const int> samples,
                                                std::size_t per_bucket) {
    const auto table = detail::build_table(samples, per_bucket);
    std::size_t start = detail::kNone;
    std::size_t longest = 0;
    for (std::size_t j = 0; j < samples.size(); ++j) {
        if (table.length[j] > longest) {
            longest = table.length[j];
            start = j;
        }
    }
    std::vector<std::size_t> indices;
    for (std::size_t at = start; at != detail::kNone; at = table.next[at]) {
        indices.push_back(at);
    }
    return indices;
}

inline std::size_t gapless_decline_length(std::span<const int> samples, std::size_t per_bucket) {
    return gapless_decline(samples, per_bucket).size();
}

struct WorstSeries {
    std::size_t series;
    std::vector<std::size_t> indices;
};

inline WorstSeries worst_series(std::span<const std::vector<int>> series, std::size_t per_bucket) {
    if (series.empty()) {
        throw std::invalid_argument("no series given");
    }
    WorstSeries worst{0, {}};
    for (std::size_t s = 0; s < series.size(); ++s) {
        auto indices = gapless_decline(series[s], per_bucket);
        if (s == 0 || indices.size() > worst.indices.size()) {
            worst = {s, std::move(indices)};
        }
    }
    return worst;
}
