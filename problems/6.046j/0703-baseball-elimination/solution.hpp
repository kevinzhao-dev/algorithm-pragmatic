#pragma once

#include <vector>

#include "practice.hpp"

struct Standings {
    std::vector<int> wins;
    std::vector<std::vector<int>> games_left;
};

// Stage 1
inline bool trivially_eliminated(const Standings& s, int team) { PRACTICE_TODO(); }

// Stage 2
inline bool is_eliminated(const Standings& s, int team) { PRACTICE_TODO(); }

// Stage 3
inline std::vector<int> elimination_certificate(const Standings& s, int team) { PRACTICE_TODO(); }

// Stage 4
class SeasonTracker {
public:
    explicit SeasonTracker(Standings initial) { PRACTICE_TODO(); }

    void record_game(int winner, int loser) { PRACTICE_TODO(); }
    std::vector<int> contenders() const { PRACTICE_TODO(); }
    bool is_contender(int team) const { PRACTICE_TODO(); }
};
