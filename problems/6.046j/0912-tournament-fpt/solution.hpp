#pragma once

#include <array>
#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"

using Tournament = std::vector<std::vector<char>>;

// Stage 1
inline bool is_acyclic(const Tournament& t) { PRACTICE_TODO(); }
inline std::optional<std::array<int, 3>> find_triangle(const Tournament& t) { PRACTICE_TODO(); }

// Stage 2
struct TournamentKernel {
    bool no_instance = false;
    Tournament reduced;
    int k = 0;
    std::vector<int> to_original;
    std::vector<std::pair<int, int>> reversed;
};
inline TournamentKernel kernelize(const Tournament& t, int k) { PRACTICE_TODO(); }

// Stage 3
inline std::optional<std::vector<std::pair<int, int>>> feedback_arcs(const Tournament& t, int k) {
    PRACTICE_TODO();
}

// Stage 4
inline std::optional<std::vector<int>> repair_ranking(const Tournament& t, int max_upsets) {
    PRACTICE_TODO();
}
