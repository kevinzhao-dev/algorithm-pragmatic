#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <stdexcept>
#include <utility>

#include "practice.hpp"

// Stage 1
template <typename T>
class StaticArray {
public:
    explicit StaticArray(std::size_t n = 0) { PRACTICE_TODO(); }

    // Free every element here.
    ~StaticArray() {}

    StaticArray(const StaticArray& other) { PRACTICE_TODO(); }
    StaticArray& operator=(const StaticArray& other) { PRACTICE_TODO(); }

    // These should be noexcept once implemented.
    StaticArray(StaticArray&& other) { PRACTICE_TODO(); }
    StaticArray& operator=(StaticArray&& other) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }

    const T& get_at(std::size_t i) const { PRACTICE_TODO(); }
    void set_at(std::size_t i, T value) { PRACTICE_TODO(); }

    T& operator[](std::size_t i) { PRACTICE_TODO(); }
    const T& operator[](std::size_t i) const { PRACTICE_TODO(); }

    T* begin() { PRACTICE_TODO(); }
    T* end() { PRACTICE_TODO(); }
    const T* begin() const { PRACTICE_TODO(); }
    const T* end() const { PRACTICE_TODO(); }
};

// Stage 2
template <typename T>
class ArraySequence {
public:
    ArraySequence() { PRACTICE_TODO(); }
    explicit ArraySequence(std::span<const T> items) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    const T& get_at(std::size_t i) const { PRACTICE_TODO(); }
    void set_at(std::size_t i, T value) { PRACTICE_TODO(); }

    void insert_at(std::size_t i, T value) { PRACTICE_TODO(); }
    T delete_at(std::size_t i) { PRACTICE_TODO(); }

    void insert_first(T value) { PRACTICE_TODO(); }
    void insert_last(T value) { PRACTICE_TODO(); }
    T delete_first() { PRACTICE_TODO(); }
    T delete_last() { PRACTICE_TODO(); }

    T* begin() { PRACTICE_TODO(); }
    T* end() { PRACTICE_TODO(); }
    const T* begin() const { PRACTICE_TODO(); }
    const T* end() const { PRACTICE_TODO(); }
};

// Stage 3
template <typename Record, typename KeyOf>
std::optional<std::pair<std::size_t, std::size_t>> first_collision(std::span<const Record> records,
                                                                   KeyOf key_of) {
    PRACTICE_TODO();
}
