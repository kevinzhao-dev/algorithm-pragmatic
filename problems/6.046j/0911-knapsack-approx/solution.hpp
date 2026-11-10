#pragma once

#include <span>
#include <vector>

#include "practice.hpp"

struct Item {
    long long size = 0;
    long long value = 0;
};

struct Selection {
    std::vector<int> items;
    long long value = 0;
    long long size = 0;
};

// Stage 1
inline Selection greedy_by_density(std::span<const Item> items, long long capacity) {
    PRACTICE_TODO();
}
struct DensityTrap {
    std::vector<Item> items;
    long long capacity = 0;
};
inline DensityTrap density_trap(long long k) { PRACTICE_TODO(); }

// Stage 2
inline Selection greedy_two_approx(std::span<const Item> items, long long capacity) {
    PRACTICE_TODO();
}

// Stage 3
inline Selection exact_by_value(std::span<const Item> items, long long capacity) {
    PRACTICE_TODO();
}

// Stage 4
inline Selection knapsack_fptas(std::span<const Item> items, long long capacity, double eps) {
    PRACTICE_TODO();
}
