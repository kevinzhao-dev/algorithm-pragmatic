#pragma once

#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

struct Matching {
    int size = 0;
    std::vector<int> match_left;
    std::vector<int> match_right;
};
struct VertexCover {
    std::vector<int> left;
    std::vector<int> right;
};

// Stage 1
inline Matching max_matching(int left, int right,
                             std::span<const std::pair<int, int>> edges) {
    PRACTICE_TODO();
}

// Stage 2
inline VertexCover min_vertex_cover(int left, int right,
                                    std::span<const std::pair<int, int>> edges) {
    PRACTICE_TODO();
}

// Stage 3
inline Matching hopcroft_karp(int left, int right, std::span<const std::pair<int, int>> edges) {
    PRACTICE_TODO();
}

// Stage 4
inline std::vector<int> hall_violator(int left, int right,
                                      std::span<const std::pair<int, int>> edges) {
    PRACTICE_TODO();
}
