#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"

struct EditCosts {
    int insert = 1;
    int erase = 1;
    int replace = 1;
};

// Stage 1 and Stage 3
inline int edit_distance(std::string_view from, std::string_view to, EditCosts costs = {}) {
    PRACTICE_TODO();
}

// Stage 2
enum class EditOp { Insert, Erase, Replace };

struct Edit {
    EditOp op;
    std::size_t at;
    char ch;
};

inline std::vector<Edit> edit_script(std::string_view from, std::string_view to,
                                     EditCosts costs = {}) {
    PRACTICE_TODO();
}

// Stage 4
inline std::optional<int> edit_distance_within(std::string_view from, std::string_view to,
                                               int limit) {
    PRACTICE_TODO();
}

// Stage 5
struct Suggestion {
    std::string_view word;
    int distance;
};

inline std::vector<Suggestion> suggest(std::string_view query,
                                       std::span<const std::string> dictionary, int limit) {
    PRACTICE_TODO();
}
