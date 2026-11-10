#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

enum class Direction { Less, Here, Greater };

template <typename Oracle>
std::int64_t locate_in_range(std::int64_t low, std::int64_t high, Oracle oracle) {
    while (low < high) {
        const std::int64_t middle = low + (high - low) / 2;
        const Direction direction = oracle(middle);
        if (direction == Direction::Here) {
            return middle;
        }
        if (direction == Direction::Less) {
            high = middle;
        } else {
            low = middle + 1;
        }
    }
    return low;
}

template <typename Oracle>
std::int64_t locate_from_start(std::int64_t n, Oracle oracle) {
    std::int64_t low = 0;  // everything before low is known to be before the target
    std::int64_t step = 1;
    for (;;) {
        const std::int64_t probe = std::min(low + step - 1, n - 1);
        const Direction direction = oracle(probe);
        if (direction == Direction::Here) {
            return probe;
        }
        if (direction == Direction::Less) {
            return locate_in_range(low, probe, oracle);
        }
        low = probe + 1;
        step *= 2;
    }
}

template <typename Oracle>
std::int64_t locate_two_sided(std::int64_t n, Oracle oracle) {
    std::int64_t low = 0;   // target >= low
    std::int64_t high = n;  // target < high
    std::int64_t step = 1;
    for (;;) {
        const std::int64_t left = std::min(low + step - 1, high - 1);
        Direction direction = oracle(left);
        if (direction == Direction::Here) {
            return left;
        }
        if (direction == Direction::Less) {
            return locate_in_range(low, left, oracle);
        }
        low = left + 1;

        const std::int64_t right = std::max(high - step, low);
        direction = oracle(right);
        if (direction == Direction::Here) {
            return right;
        }
        if (direction == Direction::Greater) {
            return locate_in_range(right + 1, high, oracle);
        }
        high = right;
        step *= 2;
    }
}

template <typename T>
class FingerCursor {
public:
    explicit FingerCursor(std::span<const T> sorted) : sorted_(sorted) {}

    std::size_t seek(const T& key) {
        const std::size_t n = sorted_.size();
        if (position_ < n && sorted_[position_] < key) {
            gallop_right(key, n);
        } else if (position_ > 0 && !(sorted_[position_ - 1] < key)) {
            gallop_left(key);
        }
        return position_;
    }

    std::size_t position() const { return position_; }

private:
    // Precondition: sorted_[position_] < key, so the answer is to the right.
    void gallop_right(const T& key, std::size_t n) {
        std::size_t low = position_ + 1;
        std::size_t step = 1;
        std::size_t high = n;
        for (;;) {
            const std::size_t probe = low + step - 1;
            if (probe >= n) {
                break;
            }
            if (sorted_[probe] < key) {
                low = probe + 1;
                step *= 2;
            } else {
                high = probe;
                break;
            }
        }
        position_ = search(key, low, high);
    }

    // Precondition: sorted_[position_ - 1] is not below key, so the answer is to the left.
    void gallop_left(const T& key) {
        std::size_t high = position_ - 1;
        std::size_t step = 1;
        std::size_t low = 0;
        for (;;) {
            if (high < step) {
                break;
            }
            const std::size_t probe = high - step;
            if (sorted_[probe] < key) {
                low = probe + 1;
                break;
            }
            high = probe;
            step *= 2;
        }
        position_ = search(key, low, high);
    }

    std::size_t search(const T& key, std::size_t low, std::size_t high) const {
        while (low < high) {
            const std::size_t middle = low + (high - low) / 2;
            if (sorted_[middle] < key) {
                low = middle + 1;
            } else {
                high = middle;
            }
        }
        return low;
    }

    std::span<const T> sorted_;
    std::size_t position_ = 0;
};
