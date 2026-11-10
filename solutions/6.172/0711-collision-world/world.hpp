// world.hpp — fixed-step simulation of bars in a walled box.
#pragma once

#include <algorithm>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "world_types.hpp"

class World {
public:
    World(Box box, std::vector<Segment> segments) : box_(box), segments_(std::move(segments)) {}

    template <typename Detect>
    void step(Detect&& detect) {
        const std::vector<Hit> hits = detect(std::span<const Segment>(segments_));
        for (const Hit& hit : hits) {
            std::swap(segments_[hit.a].v, segments_[hit.b].v);
        }
        segment_hits_ += hits.size();
        for (Segment& segment : segments_) {
            advance(segment);
        }
    }

    std::span<const Segment> segments() const { return segments_; }
    std::uint64_t segment_hits() const { return segment_hits_; }
    std::uint64_t wall_hits() const { return wall_hits_; }

private:
    void advance(Segment& s) {
        move_axis(s.p.x, s.q.x, s.v.x, box_.width);
        move_axis(s.p.y, s.q.y, s.v.y, box_.height);
    }

    void move_axis(std::int64_t& a, std::int64_t& b, std::int64_t& velocity, std::int64_t limit) {
        const auto [low, high] = std::minmax(a, b);
        if (low + velocity < 0 || high + velocity > limit) {
            velocity = -velocity;
            ++wall_hits_;
        } else {
            a += velocity;
            b += velocity;
        }
    }

    Box box_;
    std::vector<Segment> segments_;
    std::uint64_t segment_hits_ = 0;
    std::uint64_t wall_hits_ = 0;
};
