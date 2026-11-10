// A sequence whose writes are expensive: tests count how many set_at calls a sort performs.
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
class ExpensiveWriteSeq {
public:
    explicit ExpensiveWriteSeq(std::vector<T> items) : items_(std::move(items)) {}

    std::size_t size() const { return items_.size(); }

    const T& get_at(std::size_t i) const {
        ++reads_;
        return items_.at(i);
    }

    void set_at(std::size_t i, T value) {
        ++writes_;
        items_.at(i) = std::move(value);
    }

    std::uint64_t reads() const { return reads_; }
    std::uint64_t writes() const { return writes_; }
    const std::vector<T>& items() const { return items_; }

private:
    std::vector<T> items_;
    mutable std::uint64_t reads_ = 0;
    std::uint64_t writes_ = 0;
};
