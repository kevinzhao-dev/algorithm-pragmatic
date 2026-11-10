#pragma once

#include <arm_neon.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

#include "practice.hpp"

// Stage 1
[[gnu::noinline]] inline std::size_t find_byte(std::span<const std::uint8_t> haystack,
                                               std::uint8_t needle) {
    PRACTICE_TODO();
}

// Stage 2
[[gnu::noinline]] inline std::size_t count_byte(std::span<const std::uint8_t> haystack,
                                                std::uint8_t needle) {
    PRACTICE_TODO();
}

// Stage 3
[[gnu::noinline]] inline void to_lower_ascii(std::span<char> text) { PRACTICE_TODO(); }

// Stage 4
[[gnu::noinline]] inline std::size_t find_any_of3(std::span<const std::uint8_t> haystack,
                                                  std::uint8_t a, std::uint8_t b, std::uint8_t c) {
    PRACTICE_TODO();
}

// Stage 5
inline std::vector<std::string_view> split_fields(std::string_view line, char separator) {
    PRACTICE_TODO();
}
