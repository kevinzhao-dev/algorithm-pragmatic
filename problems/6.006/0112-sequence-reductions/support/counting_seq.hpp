// Sequence used by the tests. It exposes only the operations the reductions may use and counts
// them, standing in for "each operation costs O(log n)".
#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
class CountingSeq {
public:
    CountingSeq() = default;
    explicit CountingSeq(std::vector<T> items) : items_(std::move(items)) {}

    std::size_t size() const { return items_.size(); }

    void insert_at(std::size_t i, T value) {
        if (i > items_.size()) {
            throw std::out_of_range("insert_at past the end");
        }
        ++inserts_;
        items_.insert(items_.begin() + static_cast<std::ptrdiff_t>(i), std::move(value));
    }

    T delete_at(std::size_t i) {
        if (i >= items_.size()) {
            throw std::out_of_range("delete_at out of range");
        }
        ++deletes_;
        T out = std::move(items_[i]);
        items_.erase(items_.begin() + static_cast<std::ptrdiff_t>(i));
        return out;
    }

    std::uint64_t operations() const { return inserts_ + deletes_; }
    void reset_operations() { inserts_ = deletes_ = 0; }
    const std::vector<T>& items() const { return items_; }

private:
    std::vector<T> items_;
    std::uint64_t inserts_ = 0;
    std::uint64_t deletes_ = 0;
};
