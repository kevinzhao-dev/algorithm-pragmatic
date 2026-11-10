#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class DynamicArray {
public:
    DynamicArray() = default;

    ~DynamicArray() {
        destroy_elements();
        release(data_, capacity_);
    }

    DynamicArray(const DynamicArray& other)
        : data_(acquire(other.size_)), capacity_(other.size_) {
        try {
            for (; size_ < other.size_; ++size_) {
                std::construct_at(data_ + size_, other.data_[size_]);
            }
        } catch (...) {
            destroy_elements();
            release(data_, capacity_);
            throw;
        }
    }

    DynamicArray& operator=(const DynamicArray& other) {
        if (this != &other) {
            DynamicArray copy(other);
            swap(copy);
        }
        return *this;
    }

    DynamicArray(DynamicArray&& other) noexcept
        : data_(std::exchange(other.data_, nullptr)),
          size_(std::exchange(other.size_, 0)),
          capacity_(std::exchange(other.capacity_, 0)) {}

    DynamicArray& operator=(DynamicArray&& other) noexcept {
        if (this != &other) {
            destroy_elements();
            release(data_, capacity_);
            data_ = std::exchange(other.data_, nullptr);
            size_ = std::exchange(other.size_, 0);
            capacity_ = std::exchange(other.capacity_, 0);
        }
        return *this;
    }

    void swap(DynamicArray& other) noexcept {
        std::swap(data_, other.data_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    std::size_t size() const { return size_; }
    std::size_t capacity() const { return capacity_; }
    bool empty() const { return size_ == 0; }

    const T& get_at(std::size_t i) const {
        check_index(i);
        return data_[i];
    }
    void set_at(std::size_t i, T value) {
        check_index(i);
        data_[i] = std::move(value);
    }

    T& operator[](std::size_t i) { return data_[i]; }
    const T& operator[](std::size_t i) const { return data_[i]; }

    void insert_last(T value) {
        if (size_ < capacity_) {
            std::construct_at(data_ + size_, std::move(value));
            ++size_;
            return;
        }
        const std::size_t new_capacity = capacity_ == 0 ? 1 : capacity_ * 2;
        T* fresh = acquire(new_capacity);
        std::size_t relocated = 0;
        bool new_element_built = false;
        try {
            // Build the new element first: any failure below leaves *this untouched.
            std::construct_at(fresh + size_, std::move(value));
            new_element_built = true;
            for (; relocated < size_; ++relocated) {
                std::construct_at(fresh + relocated, std::move_if_noexcept(data_[relocated]));
            }
        } catch (...) {
            std::destroy(fresh, fresh + relocated);
            if (new_element_built) {
                std::destroy_at(fresh + size_);
            }
            release(fresh, new_capacity);
            throw;
        }
        commit(fresh, new_capacity, size_ + 1);
    }

    T delete_last() {
        if (empty()) {
            throw std::out_of_range("delete_last on empty array");
        }
        T value = std::move(data_[size_ - 1]);
        std::destroy_at(data_ + size_ - 1);
        --size_;
        maybe_shrink();
        return value;
    }

    T* begin() { return data_; }
    T* end() { return data_ + size_; }
    const T* begin() const { return data_; }
    const T* end() const { return data_ + size_; }

    void insert_at(std::size_t i, T value) {
        if (i > size_) {
            throw std::out_of_range("insert_at index out of range");
        }
        if (i == size_) {
            insert_last(std::move(value));
            return;
        }
        if (size_ == capacity_) {
            relocate(capacity_ * 2);
        }
        std::construct_at(data_ + size_, std::move_if_noexcept(data_[size_ - 1]));
        ++size_;
        std::move_backward(data_ + i, data_ + size_ - 2, data_ + size_ - 1);
        data_[i] = std::move(value);
    }

    T delete_at(std::size_t i) {
        check_index(i);
        T removed = std::move_if_noexcept(data_[i]);
        std::move(data_ + i + 1, data_ + size_, data_ + i);
        std::destroy_at(data_ + size_ - 1);
        --size_;
        maybe_shrink();
        return removed;
    }

private:
    static T* acquire(std::size_t capacity) {
        return capacity == 0 ? nullptr : std::allocator<T>{}.allocate(capacity);
    }

    static void release(T* data, std::size_t capacity) {
        if (data != nullptr) {
            std::allocator<T>{}.deallocate(data, capacity);
        }
    }

    void check_index(std::size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("index out of range");
        }
    }

    void destroy_elements() {
        std::destroy(data_, data_ + size_);
        size_ = 0;
    }

    // Swaps in a fully built buffer, destroying the old elements.
    void commit(T* fresh, std::size_t new_capacity, std::size_t new_size) {
        destroy_elements();
        release(data_, capacity_);
        data_ = fresh;
        capacity_ = new_capacity;
        size_ = new_size;
    }

    void relocate(std::size_t new_capacity) {
        T* fresh = acquire(new_capacity);
        std::size_t constructed = 0;
        try {
            for (; constructed < size_; ++constructed) {
                std::construct_at(fresh + constructed, std::move_if_noexcept(data_[constructed]));
            }
        } catch (...) {
            std::destroy(fresh, fresh + constructed);
            release(fresh, new_capacity);
            throw;
        }
        commit(fresh, new_capacity, size_);
    }

    void maybe_shrink() {
        if (capacity_ > 1 && size_ * 4 <= capacity_) {
            relocate(capacity_ / 2);
        }
    }

    T* data_ = nullptr;
    std::size_t size_ = 0;
    std::size_t capacity_ = 0;
};
