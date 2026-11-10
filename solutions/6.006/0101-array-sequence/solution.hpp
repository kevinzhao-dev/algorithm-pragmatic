#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

template <typename T>
class StaticArray {
public:
    explicit StaticArray(std::size_t n = 0) : data_(std::make_unique<T[]>(n)), size_(n) {}

    StaticArray(const StaticArray& other) : StaticArray(other.size_) {
        for (std::size_t i = 0; i < size_; ++i) {
            data_[i] = other.data_[i];
        }
    }

    StaticArray& operator=(const StaticArray& other) {
        if (this != &other) {
            StaticArray copy(other);
            swap(copy);
        }
        return *this;
    }

    StaticArray(StaticArray&& other) noexcept
        : data_(std::move(other.data_)), size_(std::exchange(other.size_, 0)) {}

    StaticArray& operator=(StaticArray&& other) noexcept {
        if (this != &other) {
            data_ = std::move(other.data_);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    void swap(StaticArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
    }

    std::size_t size() const { return size_; }

    const T& get_at(std::size_t i) const {
        check(i);
        return data_[i];
    }
    void set_at(std::size_t i, T value) {
        check(i);
        data_[i] = std::move(value);
    }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    T* begin() { return data_.get(); }
    T* end() { return data_.get() + size_; }
    const T* begin() const { return data_.get(); }
    const T* end() const { return data_.get() + size_; }

private:
    void check(std::size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("StaticArray index out of range");
        }
    }

    std::unique_ptr<T[]> data_;
    std::size_t size_ = 0;
};

template <typename T>
class ArraySequence {
public:
    ArraySequence() = default;

    explicit ArraySequence(std::span<const T> items) : data_(items.size()) {
        std::copy(items.begin(), items.end(), data_.begin());
    }

    std::size_t size() const { return data_.size(); }
    const T& get_at(std::size_t i) const { return data_.get_at(i); }
    void set_at(std::size_t i, T value) { data_.set_at(i, std::move(value)); }

    void insert_at(std::size_t i, T value) {
        if (i > size()) {
            throw std::out_of_range("insert_at index out of range");
        }
        StaticArray<T> bigger(size() + 1);
        for (std::size_t k = 0; k < i; ++k) {
            bigger[k] = std::move_if_noexcept(data_[k]);
        }
        for (std::size_t k = i; k < size(); ++k) {
            bigger[k + 1] = std::move_if_noexcept(data_[k]);
        }
        bigger[i] = std::move(value);
        data_ = std::move(bigger);
    }

    T delete_at(std::size_t i) {
        if (i >= size()) {
            throw std::out_of_range("delete_at index out of range");
        }
        StaticArray<T> smaller(size() - 1);
        for (std::size_t k = 0; k < i; ++k) {
            smaller[k] = std::move_if_noexcept(data_[k]);
        }
        for (std::size_t k = i + 1; k < size(); ++k) {
            smaller[k - 1] = std::move_if_noexcept(data_[k]);
        }
        T removed = std::move_if_noexcept(data_[i]);
        data_ = std::move(smaller);
        return removed;
    }

    void insert_first(T value) { insert_at(0, std::move(value)); }
    void insert_last(T value) { insert_at(size(), std::move(value)); }
    T delete_first() { return delete_at(0); }
    T delete_last() {
        if (size() == 0) {
            throw std::out_of_range("delete_last on empty sequence");
        }
        return delete_at(size() - 1);
    }

    T* begin() { return data_.begin(); }
    T* end() { return data_.end(); }
    const T* begin() const { return data_.begin(); }
    const T* end() const { return data_.end(); }

private:
    StaticArray<T> data_;
};

template <typename Record, typename KeyOf>
std::optional<std::pair<std::size_t, std::size_t>> first_collision(std::span<const Record> records,
                                                                   KeyOf key_of) {
    using Key = std::decay_t<decltype(key_of(records[0]))>;
    StaticArray<Key> keys(records.size());
    for (std::size_t j = 0; j < records.size(); ++j) {
        const Key key = key_of(records[j]);
        for (std::size_t i = 0; i < j; ++i) {
            if (keys[i] == key) {
                return std::pair(i, j);
            }
        }
        keys[j] = key;
    }
    return std::nullopt;
}
