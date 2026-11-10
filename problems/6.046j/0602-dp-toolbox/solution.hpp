#pragma once

#include <span>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"

// Stage 1
inline std::string longest_palindromic_subsequence(std::string_view s) { PRACTICE_TODO(); }

// Stage 2
struct BstPlan {
    long long cost = 0;
    int root = -1;
    std::vector<int> left, right;
};
inline BstPlan optimal_bst(std::span<const long long> weights) { PRACTICE_TODO(); }

// Stage 3
inline long long first_player_value(std::span<const long long> values) { PRACTICE_TODO(); }

// Stage 4
struct Block {
    long long length = 0;
    long long width = 0;
    long long height = 0;
};
inline long long tallest_tower(std::span<const Block> blocks) { PRACTICE_TODO(); }
inline long long count_true(std::string_view expr) { PRACTICE_TODO(); }
