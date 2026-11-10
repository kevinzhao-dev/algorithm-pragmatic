#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "practice.hpp"

enum class Tilt { Up, Down, Left, Right };

using Board = std::vector<std::string>;

// Stage 1
inline Board apply_tilt(const Board& board, Tilt direction) { PRACTICE_TODO(); }

// Stage 2
inline std::optional<std::vector<Tilt>> solve_board(const Board& start, int row, int col) {
    PRACTICE_TODO();
}

// Stage 3
inline std::size_t reachable_configurations(const Board& start) { PRACTICE_TODO(); }

inline std::uint64_t count_predecessors(const Board& board, Tilt direction) { PRACTICE_TODO(); }
