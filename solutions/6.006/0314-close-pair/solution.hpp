#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <optional>
#include <span>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../0303-counting-radix-sort/solution.hpp"

using IndexPair = std::pair<std::size_t, std::size_t>;

namespace detail {

inline std::optional<IndexPair> close_or_nothing(std::size_t i, std::size_t j, std::size_t n) {
    if (i > j) {
        std::swap(i, j);
    }
    if ((j - i) * 10 < n) {
        return IndexPair{i, j};
    }
    return std::nullopt;
}

}  // namespace detail

inline std::optional<IndexPair> close_pair_expected(std::span<const std::uint64_t> b, std::uint64_t r) {
    std::unordered_map<std::uint64_t, std::size_t> where;
    where.reserve(b.size() * 2);
    for (std::size_t i = 0; i < b.size(); ++i) {
        where.emplace(b[i], i);
    }
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b[i] >= r) {
            continue;  // the partner would have to be non-positive
        }
        const auto partner = where.find(r - b[i]);
        if (partner != where.end() && partner->second != i) {
            if (auto pair = detail::close_or_nothing(i, partner->second, b.size())) {
                return pair;
            }
        }
    }
    return std::nullopt;
}

inline std::optional<IndexPair> close_pair_worst_case(std::span<const std::uint64_t> b, std::uint64_t r) {
    // Only values below r can take part, and r < n^2 keeps those polynomial in n.
    struct Entry {
        std::uint64_t value;
        std::size_t index;
    };
    std::vector<Entry> entries;
    entries.reserve(b.size());
    for (std::size_t i = 0; i < b.size(); ++i) {
        if (b[i] < r) {
            entries.push_back(Entry{b[i], i});
        }
    }
    radix_sort<Entry>(entries, [](const Entry& entry) { return entry.value; });

    std::size_t low = 0;
    std::size_t high = entries.size();
    while (low + 1 < high) {
        const std::uint64_t sum = entries[low].value + entries[high - 1].value;
        if (sum == r) {
            if (auto pair = detail::close_or_nothing(entries[low].index, entries[high - 1].index, b.size())) {
                return pair;
            }
            ++low;
        } else if (sum < r) {
            ++low;
        } else {
            --high;
        }
    }
    return std::nullopt;
}

class WindowPairMatcher {
public:
    WindowPairMatcher(std::size_t window, std::uint64_t target) : window_(window), target_(target) {
        if (window == 0) {
            throw std::invalid_argument("window must be positive");
        }
    }

    std::optional<std::size_t> offer(std::uint64_t value) {
        const std::size_t index = next_index_++;
        std::optional<std::size_t> match;
        if (value <= target_) {
            const auto partner = latest_.find(target_ - value);
            if (partner != latest_.end()) {
                match = partner->second;
            }
        }
        recent_.emplace_back(value, index);
        latest_[value] = index;
        // After this, recent_ holds exactly what the next offer is allowed to see.
        if (recent_.size() > window_) {
            forget_oldest();
        }
        return match;
    }

    std::size_t tracked() const { return latest_.size(); }

private:
    void forget_oldest() {
        const auto [value, index] = recent_.front();
        recent_.pop_front();
        const auto it = latest_.find(value);
        if (it != latest_.end() && it->second == index) {
            latest_.erase(it);
        }
    }

    std::size_t window_;
    std::uint64_t target_;
    std::size_t next_index_ = 0;
    std::deque<std::pair<std::uint64_t, std::size_t>> recent_;
    std::unordered_map<std::uint64_t, std::size_t> latest_;
};
