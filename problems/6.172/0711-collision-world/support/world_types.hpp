// world_types.hpp — value types shared by your code, the scene generators and the tests.
#pragma once

#include <compare>
#include <cstdint>
#include <ostream>

struct Vec {
    std::int64_t x = 0;
    std::int64_t y = 0;

    friend constexpr Vec operator+(Vec a, Vec b) { return {a.x + b.x, a.y + b.y}; }
    friend constexpr Vec operator-(Vec a, Vec b) { return {a.x - b.x, a.y - b.y}; }
    friend constexpr bool operator==(Vec, Vec) = default;
    friend std::ostream& operator<<(std::ostream& os, Vec a) {
        return os << '(' << a.x << ',' << a.y << ')';
    }
};

// A rigid bar with endpoints p and q (p == q is allowed) moving by v per tick.
struct Segment {
    std::uint32_t id = 0;
    Vec p;
    Vec q;
    Vec v;

    friend constexpr bool operator==(const Segment&, const Segment&) = default;
    friend std::ostream& operator<<(std::ostream& os, const Segment& s) {
        return os << "#" << s.id << ' ' << s.p << "-" << s.q << " v" << s.v;
    }
};

// The closed world rectangle [0, width] x [0, height].
struct Box {
    std::int64_t width = 0;
    std::int64_t height = 0;
};

// An unordered pair of segment ids, normalized so that a < b.
struct Hit {
    std::uint32_t a = 0;
    std::uint32_t b = 0;

    friend constexpr auto operator<=>(const Hit&, const Hit&) = default;
    friend std::ostream& operator<<(std::ostream& os, const Hit& h) {
        return os << '{' << h.a << ',' << h.b << '}';
    }
};
