#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

#include "practice.hpp"

// Stage 5
class BitPackedVector {
public:
    BitPackedVector(unsigned width, std::size_t capacity) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    std::size_t capacity() const { PRACTICE_TODO(); }
    unsigned width() const { PRACTICE_TODO(); }

    std::uint64_t get(std::size_t index) const { PRACTICE_TODO(); }
    void set(std::size_t index, std::uint64_t value) { PRACTICE_TODO(); }
    void push_back(std::uint64_t value) { PRACTICE_TODO(); }
    void insert(std::size_t index, std::uint64_t value) { PRACTICE_TODO(); }
    void erase(std::size_t index) { PRACTICE_TODO(); }
};
