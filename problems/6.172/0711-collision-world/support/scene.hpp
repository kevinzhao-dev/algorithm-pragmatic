// scene.hpp — deterministic scene generators. The same seed always produces the same scene,
// independent of the standard library, so a failing scene can be reproduced from its seed.
//
// Every generator assigns id == index and keeps endpoints inside the box.
#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "world_types.hpp"

namespace scene {

struct Config {
    Box box{1 << 20, 1 << 20};
    std::size_t count = 1000;
    std::int64_t max_length = 1 << 12;
    std::int64_t max_speed = 1 << 8;
    std::uint64_t seed = 1;
};

class SplitMix {
public:
    explicit SplitMix(std::uint64_t seed) : state_(seed) {}

    std::uint64_t next() {
        std::uint64_t z = (state_ += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }

    // Uniform in the closed interval [low, high]; the slight modulo bias does not matter here.
    std::int64_t range(std::int64_t low, std::int64_t high) {
        const auto span = static_cast<std::uint64_t>(high - low) + 1;
        return low + static_cast<std::int64_t>(next() % span);
    }

private:
    std::uint64_t state_;
};

namespace detail {

inline Segment clamp_to_box(Segment s, Box box) {
    const auto clamp = [&](Vec a) {
        return Vec{std::clamp<std::int64_t>(a.x, 0, box.width),
                   std::clamp<std::int64_t>(a.y, 0, box.height)};
    };
    s.p = clamp(s.p);
    s.q = clamp(s.q);
    return s;
}

inline Segment random_at(SplitMix& rng, const Config& config, Vec anchor, std::uint32_t id) {
    const Vec delta{rng.range(-config.max_length, config.max_length),
                    rng.range(-config.max_length, config.max_length)};
    const Vec velocity{rng.range(-config.max_speed, config.max_speed),
                       rng.range(-config.max_speed, config.max_speed)};
    return clamp_to_box({id, anchor, anchor + delta, velocity}, config.box);
}

}  // namespace detail

// Short bars spread evenly over the whole box.
inline std::vector<Segment> uniform(const Config& config) {
    SplitMix rng(config.seed);
    std::vector<Segment> out;
    out.reserve(config.count);
    for (std::size_t i = 0; i < config.count; ++i) {
        const Vec anchor{rng.range(0, config.box.width), rng.range(0, config.box.height)};
        out.push_back(detail::random_at(rng, config, anchor, static_cast<std::uint32_t>(i)));
    }
    return out;
}

// Bars packed around a few centres: most of the box is empty, the rest is crowded.
inline std::vector<Segment> clustered(const Config& config, std::size_t clusters,
                                      std::int64_t radius) {
    SplitMix rng(config.seed);
    std::vector<Vec> centres(std::max<std::size_t>(clusters, 1));
    for (Vec& c : centres) {
        c = {rng.range(radius, config.box.width - radius),
             rng.range(radius, config.box.height - radius)};
    }
    std::vector<Segment> out;
    out.reserve(config.count);
    for (std::size_t i = 0; i < config.count; ++i) {
        const Vec c = centres[static_cast<std::size_t>(
            rng.range(0, static_cast<std::int64_t>(centres.size()) - 1))];
        const Vec anchor = c + Vec{rng.range(-radius, radius), rng.range(-radius, radius)};
        out.push_back(detail::random_at(rng, config, anchor, static_cast<std::uint32_t>(i)));
    }
    return out;
}

// Mostly tiny bars plus a fixed share of bars long enough to cross many cells.
inline std::vector<Segment> long_bars(const Config& config, std::size_t long_every) {
    SplitMix rng(config.seed);
    std::vector<Segment> out;
    out.reserve(config.count);
    for (std::size_t i = 0; i < config.count; ++i) {
        const Vec anchor{rng.range(0, config.box.width), rng.range(0, config.box.height)};
        auto s = detail::random_at(rng, config, anchor, static_cast<std::uint32_t>(i));
        if (i % long_every == 0) {
            s.q = {rng.range(0, config.box.width), rng.range(0, config.box.height)};
        }
        out.push_back(detail::clamp_to_box(s, config.box));
    }
    return out;
}

// Axis-aligned bars on a grid of powers of two, so many touches happen exactly on the lines a
// quadtree splits on.
inline std::vector<Segment> lattice(const Config& config, std::int64_t pitch) {
    SplitMix rng(config.seed);
    std::vector<Segment> out;
    out.reserve(config.count);
    const std::int64_t columns = config.box.width / pitch;
    const std::int64_t rows = config.box.height / pitch;
    for (std::size_t i = 0; i < config.count; ++i) {
        const Vec a{rng.range(0, columns) * pitch, rng.range(0, rows) * pitch};
        const bool horizontal = rng.range(0, 1) == 0;
        const Vec b = horizontal ? Vec{std::min(a.x + pitch, columns * pitch), a.y}
                                 : Vec{a.x, std::min(a.y + pitch, rows * pitch)};
        const Vec velocity{rng.range(-1, 1) * rng.range(0, pitch / 2),
                           rng.range(-1, 1) * rng.range(0, pitch / 2)};
        out.push_back({static_cast<std::uint32_t>(i), a, b, velocity});
    }
    return out;
}

// Unit-length bars laid end to end along one axis. Every integer on that axis is a contact
// point, so whichever split lines a spatial index picks, they fall exactly on a touching pair.
inline std::vector<Segment> chain(const Config& config, bool along_x) {
    std::vector<Segment> out;
    out.reserve(config.count);
    const auto across = static_cast<std::int64_t>(config.seed % 97);
    for (std::size_t i = 0; i < config.count; ++i) {
        const auto t = static_cast<std::int64_t>(i);
        const Vec a = along_x ? Vec{t, across} : Vec{across, t};
        const Vec b = along_x ? Vec{t + 1, across} : Vec{across, t + 1};
        out.push_back({static_cast<std::uint32_t>(i), a, b, Vec{}});
    }
    return out;
}

// Every bar shares one of a few exact placements: subdivision can never separate them.
inline std::vector<Segment> pile(const Config& config, std::size_t placements) {
    SplitMix rng(config.seed);
    std::vector<Segment> templates;
    for (std::size_t i = 0; i < std::max<std::size_t>(placements, 1); ++i) {
        const Vec anchor{rng.range(0, config.box.width), rng.range(0, config.box.height)};
        templates.push_back(detail::random_at(rng, config, anchor, 0));
    }
    std::vector<Segment> out;
    out.reserve(config.count);
    for (std::size_t i = 0; i < config.count; ++i) {
        Segment s = templates[static_cast<std::size_t>(
            rng.range(0, static_cast<std::int64_t>(templates.size()) - 1))];
        s.id = static_cast<std::uint32_t>(i);
        out.push_back(s);
    }
    return out;
}

// Tiny bars hugging the four walls with velocities pointing out of the box, so swept regions
// leave the box and coordinates in a spatial index can go negative.
inline std::vector<Segment> walls(const Config& config) {
    SplitMix rng(config.seed);
    std::vector<Segment> out;
    out.reserve(config.count);
    for (std::size_t i = 0; i < config.count; ++i) {
        const std::int64_t along = rng.range(0, 1);
        const std::int64_t side = rng.range(0, 1);
        const std::int64_t limit = along == 0 ? config.box.height : config.box.width;
        const std::int64_t t = rng.range(0, limit);
        Vec anchor;
        Vec velocity;
        const std::int64_t speed = rng.range(1, config.max_speed);
        const std::int64_t drift = rng.range(-config.max_speed, config.max_speed);
        if (along == 0) {
            anchor = {side == 0 ? 0 : config.box.width, t};
            velocity = {side == 0 ? -speed : speed, drift};
        } else {
            anchor = {t, side == 0 ? 0 : config.box.height};
            velocity = {drift, side == 0 ? -speed : speed};
        }
        Segment s = detail::random_at(rng, config, anchor, static_cast<std::uint32_t>(i));
        s.v = velocity;
        out.push_back(s);
    }
    return out;
}

}  // namespace scene
