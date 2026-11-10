#pragma once

#include <cstddef>
#include <stdexcept>
#include <utility>

#include "practice.hpp"

template <typename T>
class DynamicArray {
public:
    DynamicArray() { PRACTICE_TODO(); }

    // Destroy live elements and free the storage.
    ~DynamicArray() {}

    // Stage 3. The moves should be noexcept once implemented.
    DynamicArray(const DynamicArray& other) { PRACTICE_TODO(); }
    DynamicArray& operator=(const DynamicArray& other) { PRACTICE_TODO(); }
    DynamicArray(DynamicArray&& other) { PRACTICE_TODO(); }
    DynamicArray& operator=(DynamicArray&& other) { PRACTICE_TODO(); }

    // Stage 1
    std::size_t size() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }

    const T& get_at(std::size_t i) const { PRACTICE_TODO(); }
    void set_at(std::size_t i, T value) { PRACTICE_TODO(); }

    T& operator[](std::size_t i) { PRACTICE_TODO(); }
    const T& operator[](std::size_t i) const { PRACTICE_TODO(); }

    void insert_last(T value) { PRACTICE_TODO(); }
    T delete_last() { PRACTICE_TODO(); }

    T* begin() { PRACTICE_TODO(); }
    T* end() { PRACTICE_TODO(); }
    const T* begin() const { PRACTICE_TODO(); }
    const T* end() const { PRACTICE_TODO(); }

    // Stage 3
    void insert_at(std::size_t i, T value) { PRACTICE_TODO(); }
    T delete_at(std::size_t i) { PRACTICE_TODO(); }
};
