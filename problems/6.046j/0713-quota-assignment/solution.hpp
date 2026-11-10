#pragma once

#include <span>
#include <vector>

#include "practice.hpp"

// Stage 1
struct Plan {
    std::vector<int> choice;
    int rejected = 0;
};
inline Plan best_plan(std::span<const std::vector<int>> accepts, std::span<const int> quota) {
    PRACTICE_TODO();
}

// Stage 2
struct Shortage {
    std::vector<int> requests;
    std::vector<int> resources;
};
inline Shortage shortage_report(std::span<const std::vector<int>> accepts,
                                std::span<const int> quota) {
    PRACTICE_TODO();
}

// Stage 3
class Allocator {
public:
    explicit Allocator(std::vector<int> quota) { PRACTICE_TODO(); }

    int add_request(std::vector<int> accepts) { PRACTICE_TODO(); }
    void cancel(int id) { PRACTICE_TODO(); }
    int choice_of(int id) const { PRACTICE_TODO(); }
    int rejected() const { PRACTICE_TODO(); }
};
