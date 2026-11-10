#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <vector>

// diff over a range is the best (mover's total) - (other's total) on that range.
inline std::int64_t coin_game_value(std::span<const int> coins) {
    const std::size_t n = coins.size();
    if (n == 0) {
        return 0;
    }
    std::vector<std::int64_t> diff(coins.begin(), coins.end());
    for (std::size_t length = 2; length <= n; ++length) {
        for (std::size_t first = 0; first + length <= n; ++first) {
            // Before this assignment diff[first] and diff[first + 1] hold ranges of length - 1.
            diff[first] = std::max(coins[first] - diff[first + 1],
                                   coins[first + length - 1] - diff[first]);
        }
    }
    const std::int64_t total = std::accumulate(coins.begin(), coins.end(), std::int64_t{0});
    return (total + diff[0]) / 2;
}

enum class Side { Front, Back };

class CoinGame {
public:
    explicit CoinGame(std::span<const int> coins)
        : coins_(coins.begin(), coins.end()), n_(coins.size()), diff_(n_ * n_, 0) {
        for (std::size_t first = n_; first-- > 0;) {
            diff(first, first) = coins_[first];
            for (std::size_t last = first + 1; last < n_; ++last) {
                diff(first, last) = std::max(front_gain(first, last), back_gain(first, last));
            }
        }
    }

    std::int64_t value() const {
        if (n_ == 0) {
            return 0;
        }
        const std::int64_t total = std::accumulate(coins_.begin(), coins_.end(), std::int64_t{0});
        return (total + diff(0, n_ - 1)) / 2;
    }

    Side best_move(std::size_t first, std::size_t last) const {
        if (first == last || front_gain(first, last) >= back_gain(first, last)) {
            return Side::Front;
        }
        return Side::Back;
    }

private:
    std::int64_t front_gain(std::size_t first, std::size_t last) const {
        return coins_[first] - diff(first + 1, last);
    }
    std::int64_t back_gain(std::size_t first, std::size_t last) const {
        return coins_[last] - diff(first, last - 1);
    }
    std::int64_t diff(std::size_t first, std::size_t last) const {
        return diff_[first * n_ + last];
    }
    std::int64_t& diff(std::size_t first, std::size_t last) { return diff_[first * n_ + last]; }

    std::vector<int> coins_;
    std::size_t n_;
    std::vector<std::int64_t> diff_;
};
