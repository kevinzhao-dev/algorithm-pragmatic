#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "bit_array.hpp"

// A fixed-capacity column of `width`-bit integers.
class BitPackedVector {
public:
    BitPackedVector(unsigned width, std::size_t capacity)
        : width_(checked_width(width)), capacity_(capacity), bits_(capacity * width) {}

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    unsigned width() const { return width_; }

    std::uint64_t get(std::size_t index) const {
        check_index(index);
        return bits_.read_bits(index * width_, width_);
    }

    void set(std::size_t index, std::uint64_t value) {
        check_index(index);
        check_fits(value);
        bits_.write_bits(index * width_, width_, value);
    }

    void push_back(std::uint64_t value) {
        check_fits(value);
        if (size_ == capacity_) {
            throw std::length_error("BitPackedVector is full");
        }
        bits_.write_bits(size_ * width_, width_, value);
        ++size_;
    }

    void insert(std::size_t index, std::uint64_t value) {
        check_fits(value);
        if (index > size_) {
            throw std::out_of_range("insert position out of range");
        }
        if (size_ == capacity_) {
            throw std::length_error("BitPackedVector is full");
        }
        // Rotating one slot right moves the free slot just past the end down to `index`.
        const std::size_t begin = index * width_;
        bits_.rotate(begin, (size_ + 1) * width_ - begin, static_cast<std::ptrdiff_t>(width_));
        bits_.write_bits(begin, width_, value);
        ++size_;
    }

    void erase(std::size_t index) {
        check_index(index);
        const std::size_t begin = index * width_;
        bits_.rotate(begin, size_ * width_ - begin, -static_cast<std::ptrdiff_t>(width_));
        --size_;
    }

private:
    static unsigned checked_width(unsigned width) {
        if (width == 0 || width > 64) {
            throw std::invalid_argument("width must be in 1..64");
        }
        return width;
    }

    void check_index(std::size_t index) const {
        if (index >= size_) {
            throw std::out_of_range("index out of range");
        }
    }

    void check_fits(std::uint64_t value) const {
        if (width_ < 64 && (value >> width_) != 0) {
            throw std::invalid_argument("value does not fit in the column width");
        }
    }

    unsigned width_;
    std::size_t capacity_;
    std::size_t size_ = 0;
    BitArray bits_;
};
