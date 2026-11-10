#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename T>
class DynamicArray {
public:
    DynamicArray() = default;

    DynamicArray(const DynamicArray& other) : data_(allocate(other.capacity_)), capacity_(other.capacity_) {
        try {
            for (; size_ < other.size_; ++size_) {
                std::construct_at(data_ + size_, other.data_[size_]);
            }
        } catch (...) {
            std::destroy_n(data_, size_);
            allocator().deallocate(data_, capacity_);
            throw;
        }
    }

    DynamicArray(DynamicArray&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)) {}

    // By value: the copy (which may throw) happens before this object changes.
    DynamicArray& operator=(DynamicArray other) noexcept {
        swap(other);
        return *this;
    }

    ~DynamicArray() {
        std::destroy_n(data_, size_);
        if (data_ != nullptr) {
            allocator().deallocate(data_, capacity_);
        }
    }

    void swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    T& operator[](std::size_t index) { return data_[index]; }
    const T& operator[](std::size_t index) const { return data_[index]; }

    void push_back(T value) {
        if (size_ == capacity_) {
            reallocate(capacity_ == 0 ? 1 : capacity_ * 2);
        }
        std::construct_at(data_ + size_, std::move(value));
        ++size_;
    }

    void pop_back() {
        if (size_ == 0) {
            throw std::out_of_range("pop_back on an empty array");
        }
        std::destroy_at(data_ + size_ - 1);
        --size_;
        if (size_ <= capacity_ / 4) {
            try {
                reallocate(size_ == 0 ? 0 : capacity_ / 2);
            } catch (...) {
                // Shrinking is only an optimization, so a failed copy leaves the old table.
            }
        }
    }

private:
    static std::allocator<T>& allocator() {
        static std::allocator<T> instance;
        return instance;
    }

    static T* allocate(std::size_t capacity) {
        return capacity == 0 ? nullptr : allocator().allocate(capacity);
    }

    void reallocate(std::size_t new_capacity) {
        T* fresh = allocate(new_capacity);
        std::size_t built = 0;
        try {
            for (; built < size_; ++built) {
                std::construct_at(fresh + built, std::move_if_noexcept(data_[built]));
            }
        } catch (...) {
            std::destroy_n(fresh, built);
            if (fresh != nullptr) {
                allocator().deallocate(fresh, new_capacity);
            }
            throw;
        }
        std::destroy_n(data_, size_);
        if (data_ != nullptr) {
            allocator().deallocate(data_, capacity_);
        }
        data_ = fresh;
        capacity_ = new_capacity;
    }

    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};

template <typename T>
class SortedRuns {
public:
    void insert(T value) {
        std::vector<T> carry;
        carry.push_back(std::move(value));
        for (std::size_t level = 0;; ++level) {
            if (level == runs_.size()) {
                runs_.emplace_back();
            }
            if (runs_[level].empty()) {
                runs_[level] = std::move(carry);
                break;
            }
            carry = merge(runs_[level], carry);
            runs_[level].clear();
        }
        ++size_;
    }

    bool contains(const T& value) const {
        for (const std::vector<T>& run : runs_) {
            const auto it = std::lower_bound(run.begin(), run.end(), value);
            if (it != run.end() && !(value < *it)) {
                return true;
            }
        }
        return false;
    }

    std::size_t size() const { return size_; }

    std::size_t run_count() const {
        return static_cast<std::size_t>(
            std::count_if(runs_.begin(), runs_.end(), [](const auto& run) { return !run.empty(); }));
    }

    std::vector<T> sorted() const {
        std::vector<T> result;
        for (const std::vector<T>& run : runs_) {
            if (!run.empty()) {
                result = merge(result, run);
            }
        }
        return result;
    }

private:
    static std::vector<T> merge(const std::vector<T>& a, const std::vector<T>& b) {
        std::vector<T> out;
        out.reserve(a.size() + b.size());
        std::size_t i = 0;
        std::size_t j = 0;
        while (i < a.size() && j < b.size()) {
            if (b[j] < a[i]) {
                out.push_back(b[j++]);
            } else {
                out.push_back(a[i++]);
            }
        }
        out.insert(out.end(), a.begin() + static_cast<std::ptrdiff_t>(i), a.end());
        out.insert(out.end(), b.begin() + static_cast<std::ptrdiff_t>(j), b.end());
        return out;
    }

    std::vector<std::vector<T>> runs_;
    std::size_t size_ = 0;
};
