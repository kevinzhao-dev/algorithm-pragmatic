#pragma once

#include <cstddef>
#include <vector>

#include "practice.hpp"

// Stages 1 and 2
template <typename T>
class DynamicArray {
public:
    DynamicArray() { PRACTICE_TODO(); }
    DynamicArray(const DynamicArray& other) { PRACTICE_TODO(); }
    // Once implemented, this should be noexcept.
    DynamicArray(DynamicArray&& other) { PRACTICE_TODO(); }
    DynamicArray& operator=(DynamicArray other) { PRACTICE_TODO(); }
    ~DynamicArray() {}

    std::size_t size() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }

    T& operator[](std::size_t index) { PRACTICE_TODO(); }
    const T& operator[](std::size_t index) const { PRACTICE_TODO(); }

    void push_back(T value) { PRACTICE_TODO(); }
    void pop_back() { PRACTICE_TODO(); }
};

// Stage 3
template <typename T>
class SortedRuns {
public:
    void insert(T value) { PRACTICE_TODO(); }
    bool contains(const T& value) const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    std::size_t run_count() const { PRACTICE_TODO(); }
    std::vector<T> sorted() const { PRACTICE_TODO(); }
};
