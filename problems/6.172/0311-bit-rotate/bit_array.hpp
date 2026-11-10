#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

#include "practice.hpp"

class BitArray {
public:
    // Stage 1
    explicit BitArray(std::size_t bits) { PRACTICE_TODO(); }
    static BitArray from_string(std::string_view text) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool get(std::size_t index) const { PRACTICE_TODO(); }
    void set(std::size_t index, bool value) { PRACTICE_TODO(); }
    std::string to_string() const { PRACTICE_TODO(); }

    // Stage 2
    std::uint64_t read_bits(std::size_t offset, unsigned count) const { PRACTICE_TODO(); }
    void write_bits(std::size_t offset, unsigned count, std::uint64_t value) { PRACTICE_TODO(); }

    // Stage 3 and 4
    void rotate(std::size_t offset, std::size_t length, std::ptrdiff_t right_amount) {
        PRACTICE_TODO();
    }
};
