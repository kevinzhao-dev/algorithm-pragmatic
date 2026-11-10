#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
class RingDeque {
public:
    RingDeque() = default;

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    std::size_t capacity() const { return slots_.size(); }

    const T& get_at(std::size_t i) const {
        check_index(i);
        return slots_[slot_of(i)];
    }
    void set_at(std::size_t i, T value) {
        check_index(i);
        slots_[slot_of(i)] = std::move(value);
    }

    void insert_first(T value) {
        grow_if_full();
        head_ = head_ == 0 ? slots_.size() - 1 : head_ - 1;
        slots_[head_] = std::move(value);
        ++size_;
    }

    void insert_last(T value) {
        grow_if_full();
        slots_[slot_of(size_)] = std::move(value);
        ++size_;
    }

    T delete_first() {
        if (empty()) {
            throw std::out_of_range("delete_first on empty deque");
        }
        T value = std::move(slots_[head_]);
        head_ = head_ + 1 == slots_.size() ? 0 : head_ + 1;
        --size_;
        maybe_shrink();
        return value;
    }

    T delete_last() {
        if (empty()) {
            throw std::out_of_range("delete_last on empty deque");
        }
        T value = std::move(slots_[slot_of(size_ - 1)]);
        --size_;
        maybe_shrink();
        return value;
    }

private:
    std::size_t slot_of(std::size_t i) const {
        const std::size_t position = head_ + i;
        return position >= slots_.size() ? position - slots_.size() : position;
    }

    void check_index(std::size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("index out of range");
        }
    }

    void grow_if_full() {
        if (size_ == slots_.size()) {
            resize_slots(slots_.empty() ? 1 : slots_.size() * 2);
        }
    }

    void maybe_shrink() {
        if (slots_.size() > 1 && size_ * 4 <= slots_.size()) {
            resize_slots(slots_.size() / 2);
        }
    }

    void resize_slots(std::size_t capacity) {
        std::vector<T> fresh(capacity);
        for (std::size_t i = 0; i < size_; ++i) {
            fresh[i] = std::move(slots_[slot_of(i)]);
        }
        slots_ = std::move(fresh);
        head_ = 0;
    }

    std::vector<T> slots_;
    std::size_t head_ = 0;
    std::size_t size_ = 0;
};

class SlidingWindowLimiter {
public:
    SlidingWindowLimiter(std::size_t limit, std::int64_t window) : limit_(limit), window_(window) {
        if (limit == 0 || window <= 0) {
            throw std::invalid_argument("limit and window must be positive");
        }
    }

    bool allow(std::int64_t t) {
        expire(t);
        if (times_.size() >= limit_) {
            return false;
        }
        times_.insert_last(t);
        return true;
    }

    std::size_t active(std::int64_t t) {
        expire(t);
        return times_.size();
    }

private:
    void expire(std::int64_t t) {
        if (has_time_ && t < last_time_) {
            throw std::invalid_argument("timestamps must not decrease");
        }
        has_time_ = true;
        last_time_ = t;
        while (!times_.empty() && times_.get_at(0) <= t - window_) {
            times_.delete_first();
        }
    }

    std::size_t limit_;
    std::int64_t window_;
    bool has_time_ = false;
    std::int64_t last_time_ = 0;
    RingDeque<std::int64_t> times_;
};
