#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1, tightened in Stage 3
inline std::int64_t coin_game_value(std::span<const int> coins) { PRACTICE_TODO(); }

// Stage 2
enum class Side { Front, Back };

class CoinGame {
public:
    explicit CoinGame(std::span<const int> coins) { PRACTICE_TODO(); }

    std::int64_t value() const { PRACTICE_TODO(); }
    Side best_move(std::size_t first, std::size_t last) const { PRACTICE_TODO(); }
};
