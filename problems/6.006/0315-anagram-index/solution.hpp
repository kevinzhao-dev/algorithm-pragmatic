#pragma once

#include <cstddef>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"

class AnagramIndex {
public:
    // Stage 1
    AnagramIndex(std::string_view text, std::size_t k) { PRACTICE_TODO(); }
    std::size_t count(std::string_view query) const { PRACTICE_TODO(); }

    // Stage 3
    void append(char ch) { PRACTICE_TODO(); }
    std::size_t length() const { PRACTICE_TODO(); }
};

// Stage 2
inline std::vector<std::size_t> count_anagram_substrings(std::string_view text,
                                                         std::span<const std::string> queries) {
    PRACTICE_TODO();
}
