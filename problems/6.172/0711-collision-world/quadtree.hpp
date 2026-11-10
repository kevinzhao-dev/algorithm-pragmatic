// quadtree.hpp — the broad phase: brute force (Stage 1), quadtree (Stage 2), fork-join (Stage 3).
#pragma once

#include <cstddef>
#include <span>
#include <vector>

#include "collide.hpp"
#include "practice.hpp"
#include "scale.hpp"
#include "world_types.hpp"

// Stage 2 tuning knobs; change the defaults to whatever measures best.
struct QuadtreeOptions {
    std::size_t leaf_capacity = 12;
    int max_depth = 16;
};

// Stage 1. Calls pred(s, t) exactly once per unordered pair.
template <typename Pred = ExactPredicate>
std::vector<Hit> detect_brute(std::span<const Segment> segments, Pred pred = {}) {
    PRACTICE_TODO();
}

// Stage 2. Calls pred on each unordered pair at most once.
template <typename Pred = ExactPredicate>
std::vector<Hit> detect_quadtree(std::span<const Segment> segments, Pred pred = {},
                                 const QuadtreeOptions& options = {}) {
    PRACTICE_TODO();
}

// Stage 3. Spawns only through scale::fork2. May call pred from several threads at once.
template <typename Pred = ExactPredicate>
std::vector<Hit> detect_parallel(std::span<const Segment> segments, Pred pred = {},
                                 const QuadtreeOptions& options = {}, std::size_t grain = 256) {
    PRACTICE_TODO();
}
