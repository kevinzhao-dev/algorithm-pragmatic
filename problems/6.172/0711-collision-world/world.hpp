// world.hpp — Stage 1: fixed-step simulation of bars in a walled box.
#pragma once

#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "world_types.hpp"

class World {
public:
    World(Box box, std::vector<Segment> segments) : box_(box), segments_(std::move(segments)) {}

    // Runs one tick. `detect` is any callable taking std::span<const Segment> and returning
    // std::vector<Hit>, such as a lambda around detect_brute.
    template <typename Detect>
    void step(Detect&& detect) {
        PRACTICE_TODO();
    }

    std::span<const Segment> segments() const { return segments_; }
    std::uint64_t segment_hits() const { return segment_hits_; }
    std::uint64_t wall_hits() const { return wall_hits_; }

private:
    Box box_;
    std::vector<Segment> segments_;
    std::uint64_t segment_hits_ = 0;
    std::uint64_t wall_hits_ = 0;
};
