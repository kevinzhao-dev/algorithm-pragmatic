#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <span>
#include <stdexcept>

template <typename T, typename Less = std::less<T>>
class RunTracker {
public:
    void push(const T& value) {
        if (previous_ && Less{}(*previous_, value)) {
            ++current_;
        } else {
            current_ = 1;
        }
        previous_ = value;
        if (current_ > longest_) {
            longest_ = current_;
            count_ = 1;
        } else if (current_ == longest_) {
            ++count_;
        }
    }

    std::size_t longest() const { return longest_; }
    std::size_t count() const { return count_; }
    std::size_t current() const { return current_; }

private:
    std::optional<T> previous_;
    std::size_t current_ = 0;
    std::size_t longest_ = 0;
    std::size_t count_ = 0;
};

template <typename T>
std::size_t count_longest_runs(std::span<const T> values) {
    RunTracker<T> tracker;
    for (const T& value : values) {
        tracker.push(value);
    }
    return tracker.count();
}

template <typename T, typename Less = std::less<T>>
class StreakAlarm {
public:
    explicit StreakAlarm(std::size_t threshold) : threshold_(threshold) {
        if (threshold == 0) {
            throw std::invalid_argument("threshold must be at least 1");
        }
    }

    bool observe(const T& value) {
        if (previous_ && Less{}(*previous_, value)) {
            ++current_;
        } else {
            current_ = 1;
        }
        previous_ = value;
        return current_ == threshold_;
    }

private:
    std::size_t threshold_;
    std::size_t current_ = 0;
    std::optional<T> previous_;
};
