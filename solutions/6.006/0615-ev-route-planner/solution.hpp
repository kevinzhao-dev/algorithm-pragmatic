#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <queue>
#include <stdexcept>
#include <utility>
#include <vector>

struct Road {
    int a;
    int b;
    std::int64_t minutes;
};

struct RoadMap {
    std::vector<int> elevation;
    std::vector<char> has_charger;
    std::vector<Road> roads;
    int capacity;
    std::int64_t minutes_per_unit;
};

struct Stop {
    int location;
    int charge_units;
};

struct RoutePlan {
    std::int64_t total_minutes;
    std::vector<Stop> stops;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline constexpr std::int64_t kNever = std::numeric_limits<std::int64_t>::max();

struct Neighbor {
    int to;
    std::int64_t minutes;
};

struct SearchResult {
    int end_state = -1;
    std::int64_t minutes = 0;
    std::vector<int> parent;
};

// A search state is (location, charge level). Only the level matters for what the car can do next,
// so Dijkstra over these n * capacity states, with charging as single-unit moves, is enough.
inline SearchResult search(const RoadMap& map, int source, int target, bool want_parents) {
    const int n = static_cast<int>(map.elevation.size());
    if (source < 0 || source >= n || target < 0 || target >= n) {
        throw std::out_of_range("no such intersection");
    }
    const int capacity = map.capacity;
    std::vector<std::vector<Neighbor>> adjacent(idx(n));
    for (const Road& road : map.roads) {
        adjacent[idx(road.a)].push_back({road.b, road.minutes});
        adjacent[idx(road.b)].push_back({road.a, road.minutes});
    }

    const auto state_of = [capacity](int location, int level) {
        return location * capacity + (level - 1);
    };
    const auto states = idx(n) * idx(capacity);
    std::vector<std::int64_t> dist(states, kNever);
    SearchResult out;
    if (want_parents) {
        out.parent.assign(states, -1);
    }
    using Entry = std::pair<std::int64_t, int>;
    std::priority_queue<Entry, std::vector<Entry>, std::greater<Entry>> frontier;
    const int start = state_of(source, capacity);
    dist[idx(start)] = 0;
    frontier.emplace(0, start);

    const auto reach = [&](int state, std::int64_t minutes, int from_state) {
        if (minutes < dist[idx(state)]) {
            dist[idx(state)] = minutes;
            if (want_parents) {
                out.parent[idx(state)] = from_state;
            }
            frontier.emplace(minutes, state);
        }
    };

    while (!frontier.empty()) {
        const auto [minutes, state] = frontier.top();
        frontier.pop();
        if (minutes != dist[idx(state)]) {
            continue;
        }
        const int location = state / capacity;
        const int level = state % capacity + 1;
        if (location == target) {
            out.end_state = state;
            out.minutes = minutes;
            return out;
        }
        for (const Neighbor& next : adjacent[idx(location)]) {
            const int climb = std::max(0, map.elevation[idx(next.to)] - map.elevation[idx(location)]);
            if (level - climb >= 1) {
                reach(state_of(next.to, level - climb), minutes + next.minutes, state);
            }
        }
        if (map.has_charger[idx(location)] != 0 && level < capacity) {
            reach(state_of(location, level + 1), minutes + map.minutes_per_unit, state);
        }
    }
    return out;
}

}  // namespace detail

inline std::optional<std::int64_t> fastest_time(const RoadMap& map, int source, int target) {
    const detail::SearchResult found = detail::search(map, source, target, false);
    if (found.end_state == -1) {
        return std::nullopt;
    }
    return found.minutes;
}

inline std::optional<RoutePlan> fastest_plan(const RoadMap& map, int source, int target) {
    const detail::SearchResult found = detail::search(map, source, target, true);
    if (found.end_state == -1) {
        return std::nullopt;
    }
    std::vector<int> states;
    for (int s = found.end_state; s != -1; s = found.parent[detail::idx(s)]) {
        states.push_back(s);
    }
    std::reverse(states.begin(), states.end());

    RoutePlan plan{found.minutes, {}};
    for (const int state : states) {
        const int location = state / map.capacity;
        if (plan.stops.empty() || plan.stops.back().location != location) {
            plan.stops.push_back({location, 0});
        } else {
            ++plan.stops.back().charge_units;
        }
    }
    return plan;
}
