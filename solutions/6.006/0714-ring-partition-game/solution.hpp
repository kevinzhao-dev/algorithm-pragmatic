#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <numeric>
#include <span>
#include <stdexcept>
#include <vector>

// Every table below stores diff: the best (mover's total) - (other's total) on a segment.
// Since the game is zero-sum, the mover's total is (segment sum + diff) / 2.

inline std::int64_t line_game_value(std::span<const int> values) {
    const std::size_t n = values.size();
    if (n == 0) {
        return 0;
    }
    std::vector<std::int64_t> prefix(n + 1, 0);
    for (std::size_t i = 0; i < n; ++i) {
        prefix[i + 1] = prefix[i] + values[i];
    }
    const auto sum = [&](std::size_t first, std::size_t length) {
        return prefix[first + length] - prefix[first];
    };
    std::vector<std::int64_t> diff((n + 1) * (n + 1), 0);
    const auto cell = [&](std::size_t first, std::size_t length) -> std::int64_t& {
        return diff[first * (n + 1) + length];
    };
    for (std::size_t length = 1; length <= n; ++length) {
        for (std::size_t first = 0; first + length <= n; ++first) {
            if (length == 1) {
                cell(first, 1) = values[first];
                continue;
            }
            std::int64_t best = std::numeric_limits<std::int64_t>::min();
            for (std::size_t cut = 1; cut < length; ++cut) {
                best = std::max(best, sum(first, cut) - cell(first + cut, length - cut));
                best = std::max(best, sum(first + cut, length - cut) - cell(first, cut));
            }
            cell(first, length) = best;
        }
    }
    return (prefix[n] + cell(0, n)) / 2;
}

struct Arc {
    std::size_t start;
    std::size_t length;
};

class RingGame {
public:
    explicit RingGame(std::span<const int> slices)
        : slices_(slices.begin(), slices.end()),
          size_(slices.size()),
          half_(slices.size() / 2),
          prefix_(2 * slices.size() + 1, 0),
          diff_(size_ * (half_ + 1), 0) {
        if (size_ == 0 || size_ % 2 != 0) {
            throw std::invalid_argument("ring size must be positive and even");
        }
        for (std::size_t i = 0; i < 2 * size_; ++i) {
            prefix_[i + 1] = prefix_[i] + slices_[i % size_];
        }
        for (std::size_t length = 1; length <= half_; ++length) {
            for (std::size_t start = 0; start < size_; ++start) {
                diff(start, length) = length == 1 ? slices_[start] : best_gain(start, length);
            }
        }
        best_opening_gain_ = std::numeric_limits<std::int64_t>::min();
        for (std::size_t start = 0; start < size_; ++start) {
            const std::int64_t gain = opening_gain(start);
            if (gain > best_opening_gain_) {
                best_opening_gain_ = gain;
                best_opening_start_ = start;
            }
        }
    }

    std::int64_t value() const { return (sum(0, size_) + best_opening_gain_) / 2; }

    Arc best_claim(Arc remaining) const {
        if (remaining.length == size_) {
            return {best_opening_start_, half_};
        }
        if (remaining.length == 1) {
            return remaining;
        }
        Arc best{remaining.start, 1};
        std::int64_t best_gain_so_far = std::numeric_limits<std::int64_t>::min();
        const std::size_t start = remaining.start;
        const std::size_t length = remaining.length;
        for (std::size_t cut = 1; cut < length; ++cut) {
            const std::int64_t left = sum(start, cut) - diff(start + cut, length - cut);
            if (left > best_gain_so_far) {
                best_gain_so_far = left;
                best = {start, cut};
            }
            const std::int64_t right = sum(start + cut, length - cut) - diff(start, cut);
            if (right > best_gain_so_far) {
                best_gain_so_far = right;
                best = {(start + cut) % size_, length - cut};
            }
        }
        return best;
    }

private:
    std::int64_t sum(std::size_t start, std::size_t length) const {
        start %= size_;
        return prefix_[start + length] - prefix_[start];
    }
    std::int64_t& diff(std::size_t start, std::size_t length) {
        return diff_[(start % size_) * (half_ + 1) + length];
    }
    std::int64_t diff(std::size_t start, std::size_t length) const {
        return diff_[(start % size_) * (half_ + 1) + length];
    }
    std::int64_t best_gain(std::size_t start, std::size_t length) const {
        std::int64_t best = std::numeric_limits<std::int64_t>::min();
        for (std::size_t cut = 1; cut < length; ++cut) {
            best = std::max(best, sum(start, cut) - diff(start + cut, length - cut));
            best = std::max(best, sum(start + cut, length - cut) - diff(start, cut));
        }
        return best;
    }
    std::int64_t opening_gain(std::size_t start) const {
        return sum(start, half_) - diff(start + half_, half_);
    }

    std::vector<int> slices_;
    std::size_t size_;
    std::size_t half_;
    std::vector<std::int64_t> prefix_;  // over the ring written out twice
    std::vector<std::int64_t> diff_;
    std::int64_t best_opening_gain_ = 0;
    std::size_t best_opening_start_ = 0;
};

inline std::int64_t ring_game_value(std::span<const int> slices) {
    return RingGame(slices).value();
}
