#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::optional<std::int64_t> min_badness(std::span<const int> widths, int line_width) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<std::vector<std::size_t>> line_starts(std::span<const int> widths,
                                                           int line_width) {
    PRACTICE_TODO();
}

// Stage 3
inline std::optional<std::vector<std::string>> wrap_text(std::string_view text, int line_width) {
    PRACTICE_TODO();
}
