// collide.hpp — Stage 1: the exact narrow phase.
#pragma once

#include "practice.hpp"
#include "world_types.hpp"

inline bool collides(const Segment& a, const Segment& b) { PRACTICE_TODO(); }

// The default predicate for every detector. Tests swap in their own to count calls.
struct ExactPredicate {
    bool operator()(const Segment& a, const Segment& b) const { return collides(a, b); }
};
