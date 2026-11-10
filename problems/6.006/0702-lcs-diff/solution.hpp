#pragma once

#include <cstddef>
#include <ranges>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "practice.hpp"

using Alignment = std::vector<std::pair<std::size_t, std::size_t>>;

// Stage 1 and Stage 3
template <std::ranges::random_access_range A, std::ranges::random_access_range B>
std::size_t lcs_length(const A& a, const B& b) {
    PRACTICE_TODO();
}

// Stage 2
template <std::ranges::random_access_range A, std::ranges::random_access_range B>
Alignment lcs_alignment(const A& a, const B& b) {
    PRACTICE_TODO();
}

// Stage 4
template <std::ranges::random_access_range A, std::ranges::random_access_range B>
Alignment lcs_alignment_linear(const A& a, const B& b) {
    PRACTICE_TODO();
}

// Stage 5
enum class DiffKind { Keep, Remove, Add };

struct DiffLine {
    DiffKind kind;
    std::string_view text;
};

inline std::vector<DiffLine> diff_lines(std::span<const std::string> before,
                                        std::span<const std::string> after) {
    PRACTICE_TODO();
}
