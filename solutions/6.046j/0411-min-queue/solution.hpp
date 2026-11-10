#pragma once

#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <stdexcept>
#include <utility>

template <typename T, typename Compare = std::less<T>>
class MinQueue {
public:
    explicit MinQueue(Compare compare = Compare{}) : compare_(std::move(compare)) {}

    std::size_t size() const { return items_.size(); }
    bool empty() const { return items_.empty(); }

    void enqueue(T value) {
        items_.push_back(std::move(value));
        // A newcomer outlives every earlier element, so anything it beats can never be the
        // minimum again. Equal elements stay, because the earlier one leaves first.
        while (!candidates_.empty() && compare_(items_.back(), item(candidates_.back()))) {
            candidates_.pop_back();
        }
        candidates_.push_back(next_id_++);
    }

    T dequeue() {
        require_non_empty();
        if (candidates_.front() == front_id_) {
            candidates_.pop_front();
        }
        T value = std::move(items_.front());
        items_.pop_front();
        ++front_id_;
        return value;
    }

    const T& front() const {
        require_non_empty();
        return items_.front();
    }

    const T& find_min() const {
        require_non_empty();
        return item(candidates_.front());
    }

private:
    void require_non_empty() const {
        if (items_.empty()) {
            throw std::out_of_range("the queue is empty");
        }
    }

    const T& item(std::uint64_t id) const { return items_[static_cast<std::size_t>(id - front_id_)]; }

    Compare compare_;
    std::deque<T> items_;
    std::deque<std::uint64_t> candidates_;
    std::uint64_t front_id_ = 0;
    std::uint64_t next_id_ = 0;
};

class PeakTracker {
public:
    explicit PeakTracker(std::int64_t window) : window_(window) {
        if (window <= 0) {
            throw std::invalid_argument("the window must be positive");
        }
    }

    void record(std::int64_t timestamp, std::int64_t value) {
        advance_to(timestamp);
        samples_.push_back(timestamp);
        peaks_.enqueue(value);
    }

    std::optional<std::int64_t> peak(std::int64_t now) {
        advance_to(now);
        if (peaks_.empty()) {
            return std::nullopt;
        }
        return peaks_.find_min();
    }

private:
    void advance_to(std::int64_t now) {
        if (now < latest_) {
            throw std::invalid_argument("time must not go backwards");
        }
        latest_ = now;
        while (!samples_.empty() && samples_.front() <= now - window_) {
            samples_.pop_front();
            peaks_.dequeue();
        }
    }

    std::int64_t window_;
    std::int64_t latest_ = INT64_MIN;
    std::deque<std::int64_t> samples_;
    MinQueue<std::int64_t, std::greater<std::int64_t>> peaks_;
};
