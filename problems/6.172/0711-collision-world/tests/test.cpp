#include <array>
#include <atomic>
#include <limits>
#include <mutex>
#include <numeric>
#include <set>
#include <stdexcept>
#include <thread>
#include <vector>

#include "forkjoin.hpp"
#include "practice.hpp"
#include "scale.hpp"
#include "scene.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

Segment bar(std::uint32_t id, Vec p, Vec q, Vec v = {}) { return {id, p, q, v}; }

// ---------------------------------------------------------------------------------------------
// An independent exact oracle: the bars touch iff the linear system
//     p_a + lambda * d_a + tau * w == p_b + mu * d_b,   lambda, mu, tau in [0, 1]
// is feasible over the rationals, decided by Fourier-Motzkin elimination on integers.
// It shares no logic with an orientation-based predicate.
// ---------------------------------------------------------------------------------------------

using Wide = __int128;

struct Inequality {
    std::array<Wide, 4> c{};  // c[0] + c[1]*x1 + c[2]*x2 + c[3]*x3 >= 0

    friend bool operator<(const Inequality& a, const Inequality& b) { return a.c < b.c; }
    friend bool operator==(const Inequality& a, const Inequality& b) { return a.c == b.c; }
};

Wide gcd_wide(Wide a, Wide b) {
    if (a < 0) {
        a = -a;
    }
    if (b < 0) {
        b = -b;
    }
    while (b != 0) {
        const Wide r = a % b;
        a = b;
        b = r;
    }
    return a;
}

void normalize(Inequality& q) {
    Wide g = 0;
    for (const Wide value : q.c) {
        g = gcd_wide(g, value);
    }
    if (g > 1) {
        for (Wide& value : q.c) {
            value /= g;
        }
    }
}

bool feasible(std::vector<Inequality> system) {
    for (int variable = 3; variable >= 1; --variable) {
        const auto var = static_cast<std::size_t>(variable);
        std::vector<Inequality> next;
        std::vector<Inequality> positive;
        std::vector<Inequality> negative;
        for (const Inequality& q : system) {
            (q.c[var] > 0 ? positive : q.c[var] < 0 ? negative : next).push_back(q);
        }
        for (const Inequality& p : positive) {
            for (const Inequality& n : negative) {
                Inequality combined;
                for (std::size_t i = 0; i < 4; ++i) {
                    combined.c[i] = -n.c[var] * p.c[i] + p.c[var] * n.c[i];
                }
                normalize(combined);
                next.push_back(combined);
            }
        }
        std::sort(next.begin(), next.end());
        next.erase(std::unique(next.begin(), next.end()), next.end());
        system = std::move(next);
    }
    return std::all_of(system.begin(), system.end(),
                       [](const Inequality& q) { return q.c[0] >= 0; });
}

bool oracle_touch(const Segment& a, const Segment& b) {
    const Vec da = a.q - a.p;
    const Vec db = b.q - b.p;
    const Vec w = a.v - b.v;
    const Vec r = a.p - b.p;
    std::vector<Inequality> system;
    for (std::size_t variable = 1; variable <= 3; ++variable) {
        Inequality low;
        low.c[variable] = 1;
        Inequality high;
        high.c[0] = 1;
        high.c[variable] = -1;
        system.push_back(low);
        system.push_back(high);
    }
    const auto add_equation = [&](Wide constant, Wide lambda, Wide mu, Wide tau) {
        Inequality ge{{constant, lambda, mu, tau}};
        Inequality le{{-constant, -lambda, -mu, -tau}};
        system.push_back(ge);
        system.push_back(le);
    };
    add_equation(r.x, da.x, -db.x, w.x);
    add_equation(r.y, da.y, -db.y, w.y);
    return feasible(std::move(system));
}

Segment random_small_bar(Rng& rng, std::uint32_t id, std::int64_t range, std::int64_t speed) {
    const auto coordinate = [&] { return rng.uniform(0, range); };
    const auto velocity = [&] { return rng.uniform(-speed, speed); };
    return {id, {coordinate(), coordinate()}, {coordinate(), coordinate()}, {velocity(), velocity()}};
}

// ---------------------------------------------------------------------------------------------
// Scene families used to compare detectors. Small enough for ThreadSanitizer.
// ---------------------------------------------------------------------------------------------

struct NamedScene {
    const char* name;
    std::vector<Segment> segments;
};

std::vector<NamedScene> scene_families(std::uint64_t seed, std::size_t count) {
    scene::Config base;
    base.seed = seed;
    base.count = count;

    std::vector<NamedScene> scenes;
    scene::Config sparse = base;
    sparse.max_length = 1 << 14;
    sparse.max_speed = 1 << 8;
    scenes.push_back({"uniform-sparse", scene::uniform(sparse)});

    scene::Config fast = base;
    fast.max_length = 1 << 8;
    fast.max_speed = 1 << 16;
    scenes.push_back({"uniform-fast-movers", scene::uniform(fast)});

    scene::Config crowded = base;
    crowded.max_length = 1 << 13;
    crowded.max_speed = 1 << 9;
    scenes.push_back({"clustered", scene::clustered(crowded, 3, 1 << 14)});

    scene::Config hairball = base;
    hairball.box = {1 << 22, 1 << 22};
    hairball.max_length = 1 << 6;
    hairball.max_speed = 1 << 5;
    scenes.push_back({"hairball-in-a-huge-box", scene::clustered(hairball, 1, 1 << 8)});

    scene::Config bars = base;
    bars.max_length = 1 << 9;
    bars.max_speed = 1 << 8;
    scenes.push_back({"long-bars", scene::long_bars(bars, 7)});

    scene::Config grid = base;
    grid.box = {1 << 16, 1 << 16};
    scenes.push_back({"lattice-on-split-lines", scene::lattice(grid, 1 << 12)});

    scene::Config chain = base;
    scenes.push_back({"chain-along-x", scene::chain(chain, true)});
    scenes.push_back({"chain-along-y", scene::chain(chain, false)});

    scene::Config identical = base;
    scenes.push_back({"pile-of-duplicates", scene::pile(identical, 2)});

    scene::Config edge = base;
    edge.max_length = 1 << 7;
    edge.max_speed = 1 << 9;
    scenes.push_back({"walls-moving-outward", scene::walls(edge)});
    return scenes;
}

std::vector<Hit> brute(std::span<const Segment> segments) { return detect_brute(segments); }

// A shuffled copy with new ids; hits must follow the ids, not the positions in the array.
std::vector<Segment> relabel_and_shuffle(std::vector<Segment> segments, Rng& rng) {
    std::vector<int> labels = rng.permutation(segments.size());
    for (std::size_t i = 0; i < segments.size(); ++i) {
        segments[i].id = static_cast<std::uint32_t>(labels[i]) * 3 + 11;
    }
    rng.shuffle(segments);
    return segments;
}

std::uint64_t state_hash(std::span<const Segment> segments) {
    std::uint64_t hash = 14695981039346656037ULL;
    const auto mix = [&](std::int64_t value) {
        hash ^= static_cast<std::uint64_t>(value);
        hash *= 1099511628211ULL;
    };
    for (const Segment& s : segments) {
        mix(s.p.x);
        mix(s.p.y);
        mix(s.q.x);
        mix(s.q.y);
        mix(s.v.x);
        mix(s.v.y);
    }
    return hash;
}

template <typename Detect>
void run_world(World& world, int ticks, Detect&& detect) {
    for (int tick = 0; tick < ticks; ++tick) {
        world.step(detect);
    }
}

template <typename Body>
double best_of(int rounds, Body&& body) {
    double best = std::numeric_limits<double>::infinity();
    for (int round = 0; round < rounds; ++round) {
        const practice::Stopwatch watch;
        body();
        best = std::min(best, watch.elapsed_ms());
    }
    return best;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Stage 1 — narrow phase, brute-force baseline, world step
// ---------------------------------------------------------------------------------------------

TEST(1, "crossing bars at rest touch") {
    CHECK(collides(bar(0, {0, 0}, {10, 10}), bar(1, {0, 10}, {10, 0})));
}

TEST(1, "parallel separated bars at rest do not touch") {
    CHECK(!collides(bar(0, {0, 0}, {10, 0}), bar(1, {0, 1}, {10, 1})));
}

TEST(1, "an endpoint resting exactly on the other bar counts as touching") {
    CHECK(collides(bar(0, {0, 0}, {10, 0}), bar(1, {5, 0}, {5, 7})));
    CHECK(collides(bar(0, {0, 0}, {10, 0}), bar(1, {10, 0}, {20, 5})));
}

TEST(1, "collinear bars touch only if their spans overlap") {
    CHECK(collides(bar(0, {0, 0}, {10, 10}), bar(1, {5, 5}, {20, 20})));
    CHECK(collides(bar(0, {0, 0}, {10, 0}), bar(1, {10, 0}, {15, 0})));
    CHECK(!collides(bar(0, {0, 0}, {10, 0}), bar(1, {11, 0}, {15, 0})));
}

TEST(1, "a moving bar that reaches a static bar during the tick touches it") {
    const Segment still = bar(0, {0, 5}, {10, 5});
    CHECK(collides(bar(1, {3, 0}, {7, 0}, {0, 5}), still));
    CHECK(collides(still, bar(1, {3, 0}, {7, 0}, {0, 9})));
}

TEST(1, "stopping one unit short is not a touch") {
    const Segment still = bar(0, {0, 5}, {10, 5});
    CHECK(!collides(bar(1, {3, 0}, {7, 0}, {0, 4}), still));
}

TEST(1, "a static bar already overlapping counts even when moving apart") {
    CHECK(collides(bar(0, {0, 0}, {10, 0}, {0, -9}), bar(1, {5, 0}, {5, 3}, {0, 9})));
}

TEST(1, "a bar swallowed entirely inside the swept area touches") {
    const Segment tiny = bar(0, {50, 40}, {52, 41});
    CHECK(collides(bar(1, {0, 0}, {100, 0}, {0, 100}), tiny));
    CHECK(collides(tiny, bar(1, {0, 0}, {100, 0}, {0, 100})));
}

TEST(1, "moving in lockstep keeps a gap") {
    CHECK(!collides(bar(0, {0, 0}, {10, 0}, {3, 7}), bar(1, {0, 1}, {10, 1}, {3, 7})));
}

TEST(1, "two moving bars meet head-on even when neither reaches the other's start") {
    CHECK(collides(bar(0, {0, 0}, {0, 10}, {5, 0}), bar(1, {8, 0}, {8, 10}, {-5, 0})));
}

TEST(1, "a zero-length bar behaves like a moving point") {
    const Segment wall = bar(0, {5, -5}, {5, 5});
    CHECK(collides(bar(1, {0, 0}, {0, 0}, {5, 0}), wall));
    CHECK(!collides(bar(1, {0, 0}, {0, 0}, {4, 0}), wall));
    CHECK(collides(bar(1, {3, 3}, {3, 3}), bar(2, {3, 3}, {3, 3})));
    CHECK(!collides(bar(1, {3, 3}, {3, 3}), bar(2, {3, 4}, {3, 4})));
}

TEST(1, "movement parallel to a bar sweeps a degenerate area") {
    CHECK(collides(bar(0, {0, 0}, {4, 0}, {10, 0}), bar(1, {12, 0}, {13, 0})));
    CHECK(!collides(bar(0, {0, 0}, {4, 0}, {10, 0}), bar(1, {15, 0}, {16, 0})));
    CHECK(!collides(bar(0, {0, 0}, {4, 0}, {10, 0}), bar(1, {12, 1}, {13, 1})));
}

TEST(1, "collides is symmetric on random bars") {
    Rng rng;
    for (int trial = 0; trial < 20'000; ++trial) {
        const Segment a = random_small_bar(rng, 0, 24, 9);
        const Segment b = random_small_bar(rng, 1, 24, 9);
        INFO("trial ", trial, " a=", a, " b=", b);
        REQUIRE_EQ(collides(a, b), collides(b, a));
    }
}

TEST(1, "collides matches an exact linear-program oracle on small grids") {
    Rng rng;
    int touching = 0;
    const int trials = 25'000;
    for (int trial = 0; trial < trials; ++trial) {
        // Tiny coordinates make collinear, endpoint-touching and degenerate cases common.
        const std::int64_t range = trial % 2 == 0 ? 6 : 30;
        const std::int64_t speed = trial % 3 == 0 ? 1 : 12;
        const Segment a = random_small_bar(rng, 0, range, speed);
        const Segment b = random_small_bar(rng, 1, range, speed);
        INFO("trial ", trial, " a=", a, " b=", b);
        const bool expected = oracle_touch(a, b);
        touching += expected ? 1 : 0;
        REQUIRE_EQ(collides(a, b), expected);
    }
    CHECK_GT(touching, trials / 10);
    CHECK_LT(touching, trials - trials / 10);
}

TEST(1, "results do not depend on absolute position even at the coordinate limits") {
    Rng rng;
    const std::int64_t limit = std::int64_t{1} << 26;
    for (int trial = 0; trial < 5'000; ++trial) {
        Segment a = random_small_bar(rng, 0, 30, 12);
        Segment b = random_small_bar(rng, 1, 30, 12);
        const bool expected = oracle_touch(a, b);
        const Vec shift{limit - 40, -(limit - 40)};
        a.p = a.p + shift;
        a.q = a.q + shift;
        b.p = b.p + shift;
        b.q = b.q + shift;
        INFO("trial ", trial, " a=", a, " b=", b);
        REQUIRE_EQ(collides(a, b), expected);
    }
}

TEST(1, "bars as long as the coordinate range are handled without overflow") {
    const std::int64_t limit = std::int64_t{1} << 26;
    const std::int64_t speed = std::int64_t{1} << 20;
    const Segment a = bar(0, {-limit, -limit}, {limit, limit}, {speed, -speed});
    const Segment crossing = bar(1, {-limit, limit}, {limit, -limit}, {-speed, speed});
    CHECK(collides(a, crossing));
    const Segment clear = bar(1, {limit - 4, -limit}, {limit, -limit + 4});
    CHECK(!collides(a, clear));
    // a's line y = x reaches y = x - 2 * speed at the end of the tick and not a unit beyond.
    CHECK(collides(a, bar(1, {-limit + 2 * speed, -limit}, {limit, limit - 2 * speed})));
    CHECK(!collides(a, bar(1, {-limit + 2 * speed + 8, -limit}, {limit, limit - 2 * speed - 8})));
}

TEST(1, "brute force on empty and single-element input reports nothing") {
    CHECK(detect_brute(std::span<const Segment>{}).empty());
    const std::vector<Segment> one{bar(0, {0, 0}, {5, 5})};
    CHECK(detect_brute(std::span<const Segment>(one)).empty());
}

TEST(1, "brute force reports each pair once, normalized and sorted by id") {
    const std::vector<Segment> segments{
        bar(7, {0, 0}, {10, 10}),
        bar(2, {0, 10}, {10, 0}),
        bar(5, {0, 5}, {10, 5}),
        bar(9, {100, 100}, {110, 100}),
    };
    const std::vector<Hit> expected{{2, 5}, {2, 7}, {5, 7}};
    CHECK_EQ(detect_brute(std::span<const Segment>(segments)), expected);
}

TEST(1, "brute force tests every unordered pair exactly once") {
    scene::Config config;
    config.count = 300;
    const auto segments = scene::uniform(config);
    std::uint64_t calls = 0;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> seen;
    detect_brute(std::span<const Segment>(segments), [&](const Segment& a, const Segment& b) {
        ++calls;
        seen.push_back({std::min(a.id, b.id), std::max(a.id, b.id)});
        return false;
    });
    CHECK_EQ(calls, 300 * 299 / 2);
    std::sort(seen.begin(), seen.end());
    CHECK(std::adjacent_find(seen.begin(), seen.end()) == seen.end());
}

TEST(1, "with nothing to hit every bar moves by its velocity") {
    World world({1000, 1000}, {bar(0, {10, 10}, {20, 10}, {5, 7}), bar(1, {500, 500}, {500, 520}, {-3, 0})});
    world.step(brute);
    CHECK_EQ(world.segments()[0], bar(0, {15, 17}, {25, 17}, {5, 7}));
    CHECK_EQ(world.segments()[1], bar(1, {497, 500}, {497, 520}, {-3, 0}));
    CHECK_EQ(world.segment_hits(), 0);
    CHECK_EQ(world.wall_hits(), 0);
}

TEST(1, "two bars that meet exchange velocities before moving") {
    World world({1000, 1000},
                {bar(0, {100, 0}, {100, 50}, {6, 0}), bar(1, {108, 0}, {108, 50}, {-6, 0})});
    world.step(brute);
    CHECK_EQ(world.segments()[0], bar(0, {94, 0}, {94, 50}, {-6, 0}));
    CHECK_EQ(world.segments()[1], bar(1, {114, 0}, {114, 50}, {6, 0}));
    CHECK_EQ(world.segment_hits(), 1);
}

TEST(1, "simultaneous hits are resolved in sorted id order") {
    // A plus sign: all three bars cross at the same point, so hits are (0,1), (0,2), (1,2).
    World world({1000, 1000}, {bar(0, {90, 100}, {110, 100}, {1, 0}),
                               bar(1, {100, 90}, {100, 110}, {0, 1}),
                               bar(2, {90, 90}, {110, 110}, {2, 3})});
    world.step(brute);
    // swap(0,1): v0=(0,1) v1=(1,0)  swap(0,2): v0=(2,3) v2=(0,1)  swap(1,2): v1=(0,1) v2=(1,0)
    CHECK_EQ(world.segments()[0].v, Vec{2, 3});
    CHECK_EQ(world.segments()[1].v, Vec{0, 1});
    CHECK_EQ(world.segments()[2].v, Vec{1, 0});
    CHECK_EQ(world.segment_hits(), 3);
}

TEST(1, "a chain of hits passes velocities along in sorted order") {
    // Bars 0 and 2 both cross bar 1 but not each other, so hits are exactly (0,1) then (1,2).
    World world({1000, 1000}, {bar(0, {10, 45}, {10, 55}, {1, 0}),
                               bar(1, {0, 50}, {100, 50}, {0, 1}),
                               bar(2, {90, 45}, {90, 55}, {2, 2})});
    world.step(brute);
    CHECK_EQ(world.segments()[0].v, Vec{0, 1});
    CHECK_EQ(world.segments()[1].v, Vec{2, 2});
    CHECK_EQ(world.segments()[2].v, Vec{1, 0});
    CHECK_EQ(world.segment_hits(), 2);
}

TEST(1, "a wall reverses that axis and blocks the move on that axis only") {
    World world({100, 100}, {bar(0, {90, 50}, {98, 50}, {5, 3})});
    world.step(brute);
    CHECK_EQ(world.segments()[0], bar(0, {90, 53}, {98, 53}, {-5, 3}));
    CHECK_EQ(world.wall_hits(), 1);
}

TEST(1, "reaching exactly the wall is allowed") {
    World world({100, 100}, {bar(0, {90, 50}, {98, 50}, {2, 0})});
    world.step(brute);
    CHECK_EQ(world.segments()[0], bar(0, {92, 50}, {100, 50}, {2, 0}));
    CHECK_EQ(world.wall_hits(), 0);
}

TEST(1, "a corner bounces both axes and counts two wall hits") {
    World world({100, 100}, {bar(0, {2, 2}, {2, 6}, {-3, -3})});
    world.step(brute);
    CHECK_EQ(world.segments()[0], bar(0, {2, 2}, {2, 6}, {3, 3}));
    CHECK_EQ(world.wall_hits(), 2);
}

TEST(1, "hits only permute velocities") {
    scene::Config config;
    config.count = 200;
    config.box = {1 << 15, 1 << 15};
    config.max_length = 1 << 11;
    config.max_speed = 1 << 6;
    World world(config.box, scene::uniform(config));
    std::uint64_t hits = 0;
    for (int tick = 0; tick < 30; ++tick) {
        auto velocities = [&] {
            std::vector<std::pair<std::int64_t, std::int64_t>> out;
            for (const Segment& s : world.segments()) {
                out.push_back({s.v.x, s.v.y});
            }
            std::sort(out.begin(), out.end());
            return out;
        };
        const auto before = velocities();
        const auto walls_before = world.wall_hits();
        world.step(brute);
        if (world.wall_hits() == walls_before) {
            CHECK_EQ(velocities(), before);
        }
        hits = world.segment_hits();
    }
    CHECK_GT(hits, 0);
}

// ---------------------------------------------------------------------------------------------
// Stage 2 — quadtree
// ---------------------------------------------------------------------------------------------

TEST(2, "quadtree handles empty, one and two bars") {
    CHECK(detect_quadtree(std::span<const Segment>{}).empty());
    const std::vector<Segment> one{bar(0, {0, 0}, {5, 5})};
    CHECK(detect_quadtree(std::span<const Segment>(one)).empty());
    const std::vector<Segment> two{bar(0, {0, 0}, {5, 5}), bar(1, {0, 5}, {5, 0})};
    CHECK_EQ(detect_quadtree(std::span<const Segment>(two)), std::vector<Hit>{{0, 1}});
}

TEST(2, "quadtree finds the same hits as brute force on every scene family") {
    for (std::uint64_t seed = 1; seed <= 6; ++seed) {
        for (const auto& [name, segments] : scene_families(seed, 700)) {
            INFO("scene ", name, " seed ", seed);
            CHECK_EQ(detect_quadtree(std::span<const Segment>(segments)), brute(segments));
        }
    }
}

TEST(2, "scene families exercise the detector") {
    // Guards the fixtures: a family with no hits, or only hits, would test nothing.
    for (const auto& [name, segments] : scene_families(1, 700)) {
        INFO("scene ", name);
        const auto hits = brute(segments);
        CHECK_GT(hits.size(), 0);
    }
}

TEST(2, "two bars touching exactly on a split line are found") {
    // With the root square [0, 16)^2 the split lines are x = 8 and y = 8.
    const std::vector<Segment> segments{bar(0, {4, 4}, {8, 4}),   // ends on x = 8
                                        bar(1, {8, 4}, {12, 4}),  // starts on x = 8
                                        bar(2, {15, 15}, {15, 15}),
                                        bar(3, {0, 0}, {0, 0})};
    CHECK_EQ(detect_quadtree(std::span<const Segment>(segments), ExactPredicate{},
                             QuadtreeOptions{1, 8}),
             std::vector<Hit>{{0, 1}});
}

TEST(2, "a long bar is compared with short bars far below it in the tree") {
    std::vector<Segment> segments{bar(0, {0, 0}, {1 << 20, 1 << 20})};
    for (std::uint32_t i = 1; i <= 64; ++i) {
        const std::int64_t c = (std::int64_t{1} << 20) / 65 * i;
        segments.push_back(bar(i, {c, c}, {c + 1, c - 1}));
    }
    std::vector<Hit> expected;
    for (std::uint32_t i = 1; i <= 64; ++i) {
        expected.push_back({0, i});
    }
    CHECK_EQ(detect_quadtree(std::span<const Segment>(segments)), expected);
}

TEST(2, "a fast mover is found even if it starts in a far-away cell") {
    std::vector<Segment> segments;
    for (std::uint32_t i = 0; i < 200; ++i) {
        const auto c = static_cast<std::int64_t>(i) * 4096;
        segments.push_back(bar(i, {c, 1000}, {c + 8, 1000}));
    }
    segments.push_back(bar(200, {0, 1000}, {0, 1000}, {4096 * 199 + 4, 0}));
    const auto hits = detect_quadtree(std::span<const Segment>(segments));
    CHECK_EQ(hits, brute(segments));
    CHECK_EQ(hits.size(), 200);
}

TEST(2, "exact duplicates terminate and report every pair") {
    std::vector<Segment> segments;
    for (std::uint32_t i = 0; i < 120; ++i) {
        segments.push_back(bar(i, {77, 91}, {88, 104}, {1, -1}));
    }
    const auto hits = detect_quadtree(std::span<const Segment>(segments));
    CHECK_EQ(hits.size(), 120 * 119 / 2);
    CHECK_EQ(hits, brute(segments));
}

TEST(2, "identical motionless points terminate and report every pair") {
    std::vector<Segment> segments;
    for (std::uint32_t i = 0; i < 80; ++i) {
        segments.push_back(bar(i, {640, 512}, {640, 512}));
    }
    const std::vector<Hit> expected = brute(segments);
    CHECK_EQ(expected.size(), 80 * 79 / 2);
    CHECK_EQ(detect_quadtree(std::span<const Segment>(segments)), expected);
    CHECK_EQ(detect_quadtree(std::span<const Segment>(segments), ExactPredicate{},
                             QuadtreeOptions{1, 1000}),
             expected);
}

TEST(2, "every option setting still gives exact results") {
    scene::Config config;
    config.seed = 5;
    config.count = 500;
    config.max_length = 1 << 13;
    const auto segments = scene::clustered(config, 4, 1 << 13);
    const auto expected = brute(segments);
    for (const QuadtreeOptions options : {QuadtreeOptions{1, 64}, QuadtreeOptions{1000, 16},
                                          QuadtreeOptions{4, 0}, QuadtreeOptions{4, 1},
                                          QuadtreeOptions{64, 3}, QuadtreeOptions{1, 6}}) {
        INFO("leaf_capacity ", options.leaf_capacity, " max_depth ", options.max_depth);
        CHECK_EQ(detect_quadtree(std::span<const Segment>(segments), ExactPredicate{}, options),
                 expected);
    }
}

TEST(2, "results follow ids, not array positions") {
    Rng rng;
    for (const auto& [name, segments] : scene_families(9, 400)) {
        INFO("scene ", name);
        const auto shuffled = relabel_and_shuffle(segments, rng);
        CHECK_EQ(detect_quadtree(std::span<const Segment>(shuffled)), brute(shuffled));
    }
}

TEST(2, "each pair reaches the predicate at most once") {
    for (std::uint64_t seed = 1; seed <= 2; ++seed) {
        for (const auto& [name, segments] : scene_families(seed, 500)) {
            INFO("scene ", name, " seed ", seed);
            std::vector<Hit> seen;
            detect_quadtree(std::span<const Segment>(segments), [&](const Segment& a, const Segment& b) {
                seen.push_back({std::min(a.id, b.id), std::max(a.id, b.id)});
                return collides(a, b);
            });
            std::sort(seen.begin(), seen.end());
            CHECK(std::adjacent_find(seen.begin(), seen.end()) == seen.end());
        }
    }
}

namespace {
std::uint64_t pair_tests(const std::vector<Segment>& segments) {
    std::uint64_t calls = 0;
    detect_quadtree(std::span<const Segment>(segments), [&](const Segment& a, const Segment& b) {
        ++calls;
        return collides(a, b);
    });
    return calls;
}
}  // namespace

TEST(2, "on a sparse scene the quadtree skips almost every pair") {
    scene::Config config;
    config.count = 4000;
    config.max_length = 1 << 8;
    config.max_speed = 1 << 6;
    const auto segments = scene::uniform(config);
    const std::uint64_t all_pairs = 4000ULL * 3999 / 2;
    CHECK_LT(pair_tests(segments) * 100, all_pairs);
}

TEST(2, "pair tests grow nearly linearly with the scene size at constant density") {
    // Four times the bars in twice the width of box keeps the local crowding the same.
    scene::Config config;
    config.max_length = 1 << 13;
    config.max_speed = 1 << 6;
    config.count = 2000;
    config.box = {1 << 19, 1 << 19};
    const auto small_scene = scene::uniform(config);
    config.count = 32000;
    config.box = {1 << 21, 1 << 21};
    const auto large_scene = scene::uniform(config);
    const double exponent = practice::growth_exponent(static_cast<double>(pair_tests(small_scene)),
                                                      static_cast<double>(pair_tests(large_scene)),
                                                      2000, 32000);
    CHECK_LT(exponent, 1.35);
}

TEST(2, "a quadtree world evolves exactly like a brute-force world") {
    for (std::uint64_t seed = 1; seed <= 3; ++seed) {
        scene::Config config;
        config.seed = seed;
        config.box = {1 << 14, 1 << 14};
        config.count = 150;
        config.max_length = 1 << 10;
        config.max_speed = 1 << 6;
        const auto initial = seed == 2 ? scene::clustered(config, 2, 1 << 11) : scene::uniform(config);
        World with_brute(config.box, initial);
        World with_tree(config.box, initial);
        run_world(with_brute, 60, brute);
        run_world(with_tree, 60, [](std::span<const Segment> s) { return detect_quadtree(s); });
        INFO("seed ", seed);
        CHECK_EQ(state_hash(with_tree.segments()), state_hash(with_brute.segments()));
        CHECK_EQ(with_tree.segment_hits(), with_brute.segment_hits());
        CHECK_EQ(with_tree.wall_hits(), with_brute.wall_hits());
        CHECK_GT(with_brute.segment_hits(), 0);
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 3 — fork-join parallelism
// ---------------------------------------------------------------------------------------------

TEST(3, "parallel detection finds the same hits for any worker count") {
    for (const unsigned workers : {1U, 2U, 4U, 8U}) {
        practice::set_workers(workers);
        for (const auto& [name, segments] : scene_families(3, 900)) {
            INFO("workers ", workers, " scene ", name);
            CHECK_EQ(detect_parallel(std::span<const Segment>(segments)), brute(segments));
        }
    }
}

TEST(3, "parallel detection handles empty and tiny inputs") {
    practice::set_workers(4);
    CHECK(detect_parallel(std::span<const Segment>{}).empty());
    const std::vector<Segment> one{bar(0, {0, 0}, {5, 5})};
    CHECK(detect_parallel(std::span<const Segment>(one)).empty());
    const std::vector<Segment> two{bar(0, {0, 0}, {5, 5}), bar(1, {0, 5}, {5, 0})};
    CHECK_EQ(detect_parallel(std::span<const Segment>(two)), std::vector<Hit>{{0, 1}});
}

TEST(3, "a grain of one still gives the exact sorted answer") {
    practice::set_workers(8);
    for (const auto& [name, segments] : scene_families(4, 800)) {
        INFO("scene ", name);
        CHECK_EQ(detect_parallel(std::span<const Segment>(segments), ExactPredicate{},
                                 QuadtreeOptions{}, 1),
                 brute(segments));
    }
}

TEST(3, "repeated parallel runs are identical") {
    practice::set_workers(8);
    scene::Config config;
    config.count = 3000;
    config.max_length = 1 << 11;
    const auto segments = scene::clustered(config, 5, 1 << 15);
    const auto first = detect_parallel(std::span<const Segment>(segments));
    for (int run = 0; run < 8; ++run) {
        INFO("run ", run);
        CHECK_EQ(detect_parallel(std::span<const Segment>(segments)), first);
    }
}

TEST(3, "parallelism does not change how many pairs are tested") {
    practice::set_workers(8);
    for (const auto& [name, segments] : scene_families(2, 900)) {
        INFO("scene ", name);
        std::vector<Hit> seen;
        std::mutex guard;
        detect_parallel(std::span<const Segment>(segments), [&](const Segment& a, const Segment& b) {
            const std::lock_guard lock(guard);
            seen.push_back({std::min(a.id, b.id), std::max(a.id, b.id)});
            return collides(a, b);
        });
        std::sort(seen.begin(), seen.end());
        CHECK(std::adjacent_find(seen.begin(), seen.end()) == seen.end());
        CHECK_EQ(seen.size(), pair_tests(segments));
    }
}

TEST(3, "an exception from the predicate reaches the caller") {
    practice::set_workers(8);
    scene::Config config;
    config.count = 2000;
    const auto segments = scene::clustered(config, 3, 1 << 13);
    std::atomic<int> calls{0};
    CHECK_THROWS_AS(detect_parallel(std::span<const Segment>(segments),
                                    [&](const Segment& a, const Segment& b) {
                                        if (calls.fetch_add(1) == 500) {
                                            throw std::runtime_error("predicate failed");
                                        }
                                        return collides(a, b);
                                    }),
                    std::runtime_error);
}

TEST(3, "a fork-join world evolves exactly like a brute-force world") {
    practice::set_workers(8);
    for (std::uint64_t seed = 1; seed <= 2; ++seed) {
        scene::Config config;
        config.seed = seed;
        config.box = {1 << 14, 1 << 14};
        config.count = 200;
        config.max_length = 1 << 10;
        config.max_speed = 1 << 6;
        const auto initial = scene::uniform(config);
        World with_brute(config.box, initial);
        World with_forks(config.box, initial);
        run_world(with_brute, 40, brute);
        run_world(with_forks, 40, [](std::span<const Segment> s) { return detect_parallel(s); });
        INFO("seed ", seed);
        CHECK_EQ(state_hash(with_forks.segments()), state_hash(with_brute.segments()));
        CHECK_EQ(with_forks.segment_hits(), with_brute.segment_hits());
    }
}

namespace {
scale::Report parallelism_of(const std::vector<Segment>& segments) {
    return scale::measure([&] {
        detect_parallel(std::span<const Segment>(segments), [](const Segment& a, const Segment& b) {
            scale::charge();
            return collides(a, b);
        });
    });
}
}  // namespace

TEST(3, "the detector exposes parallelism on spread-out scenes") {
    practice::set_workers(1);
    scene::Config uniform_config;
    uniform_config.count = 20'000;
    uniform_config.max_length = 1 << 13;
    uniform_config.max_speed = 1 << 6;
    const auto uniform_report = parallelism_of(scene::uniform(uniform_config));
    INFO("uniform work ", uniform_report.work, " span ", uniform_report.span);
    CHECK_GT(uniform_report.parallelism(), 16.0);

    scene::Config clustered_config;
    clustered_config.count = 20'000;
    clustered_config.max_length = 1 << 10;
    clustered_config.max_speed = 1 << 5;
    const auto clustered_report =
        parallelism_of(scene::clustered(clustered_config, 16, 1 << 13));
    INFO("clustered work ", clustered_report.work, " span ", clustered_report.span);
    CHECK_GT(clustered_report.parallelism(), 8.0);
}

// ---------------------------------------------------------------------------------------------
// Stage 4 — measured speedup against the brute-force baseline, in one binary
// ---------------------------------------------------------------------------------------------

namespace {

// Baseline and contender are timed in alternation; taking the minimum of each over several
// rounds cancels other processes' noise far better than a single measurement of either.
template <typename Baseline, typename Contender>
double speedup(int rounds, Baseline&& baseline, Contender&& contender) {
    double baseline_best = std::numeric_limits<double>::infinity();
    double contender_best = std::numeric_limits<double>::infinity();
    for (int round = 0; round < rounds; ++round) {
        baseline_best = std::min(baseline_best, best_of(1, baseline));
        contender_best = std::min(contender_best, best_of(1, contender));
    }
    return baseline_best / contender_best;
}

}  // namespace

PERF(4, "quadtree is at least 20x faster than brute force on a large uniform scene", 20'000) {
    scene::Config config;
    config.count = 16'000;
    config.max_length = 1 << 10;
    config.max_speed = 1 << 7;
    const auto segments = scene::uniform(config);
    std::size_t sink = 0;
    const double ratio = speedup(
        2, [&] { sink += detect_brute(std::span<const Segment>(segments)).size(); },
        [&] { sink += detect_quadtree(std::span<const Segment>(segments)).size(); });
    INFO("speedup ", ratio, " sink ", sink);
    CHECK_GE(ratio, 20.0);
}

PERF(4, "quadtree is at least 5x faster than brute force on a clustered scene", 20'000) {
    scene::Config config;
    config.count = 12'000;
    config.max_length = 1 << 9;
    config.max_speed = 1 << 6;
    const auto segments = scene::clustered(config, 12, 1 << 14);
    std::size_t sink = 0;
    const double ratio = speedup(
        2, [&] { sink += detect_brute(std::span<const Segment>(segments)).size(); },
        [&] { sink += detect_quadtree(std::span<const Segment>(segments)).size(); });
    INFO("speedup ", ratio, " sink ", sink);
    CHECK_GE(ratio, 5.0);
}

PERF(4, "a scene the tree cannot help is still within 3x of brute force", 20'000) {
    // Every bar crosses the centre of the box: every comparison is unavoidable.
    scene::Config config;
    config.count = 6000;
    const auto segments = [&] {
        scene::SplitMix rng(7);
        std::vector<Segment> out;
        for (std::uint32_t i = 0; i < config.count; ++i) {
            const std::int64_t a = rng.range(0, config.box.height);
            const std::int64_t b = rng.range(0, config.box.height);
            out.push_back(bar(i, {0, a}, {config.box.width, b}, {0, 0}));
        }
        return out;
    }();
    std::size_t sink = 0;
    const double ratio = speedup(
        2, [&] { sink += detect_brute(std::span<const Segment>(segments)).size(); },
        [&] { sink += detect_quadtree(std::span<const Segment>(segments)).size(); });
    INFO("speedup ", ratio, " sink ", sink);
    CHECK_GE(ratio, 1.0 / 3.0);
}

PERF(4, "more workers make detection faster than one", 30'000) {
    if (std::thread::hardware_concurrency() < 8) {
        return;
    }
    scene::Config config;
    config.count = 200'000;
    config.box = {1 << 24, 1 << 24};
    config.max_length = 1 << 11;
    config.max_speed = 1 << 8;
    const auto segments = scene::uniform(config);
    std::size_t sink = 0;
    double serial = std::numeric_limits<double>::infinity();
    double parallel = std::numeric_limits<double>::infinity();
    for (int round = 0; round < 5; ++round) {
        practice::set_workers(1);
        serial = std::min(serial, best_of(1, [&] {
                              sink += detect_parallel(std::span<const Segment>(segments)).size();
                          }));
        practice::set_workers(8);
        parallel = std::min(parallel, best_of(1, [&] {
                                sink += detect_parallel(std::span<const Segment>(segments)).size();
                            }));
    }
    practice::set_workers(1);
    INFO("1-worker ", serial, " ms, 8-worker ", parallel, " ms, sink ", sink);
    CHECK_GE(serial / parallel, 1.5);
}

PERF(4, "a full simulation with the fork-join detector beats the baseline by 8x", 30'000) {
    practice::set_workers(std::min(8U, std::max(1U, std::thread::hardware_concurrency())));
    scene::Config config;
    config.count = 8'000;
    config.max_length = 1 << 11;
    config.max_speed = 1 << 8;
    const auto initial = scene::uniform(config);
    std::uint64_t sink = 0;
    const double ratio = speedup(
        2,
        [&] {
            World world(config.box, initial);
            run_world(world, 8, brute);
            sink += world.segment_hits();
        },
        [&] {
            World world(config.box, initial);
            run_world(world, 8, [](std::span<const Segment> s) { return detect_parallel(s); });
            sink += world.segment_hits();
        });
    INFO("speedup ", ratio, " sink ", sink);
    CHECK_GE(ratio, 8.0);
}
