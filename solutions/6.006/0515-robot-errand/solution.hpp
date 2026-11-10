#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <vector>

inline constexpr int kNoBadge = -1;

struct Door {
    int a;
    int b;
    bool two_way;
    int badge;
};

struct Site {
    int locations;
    int dock;
    std::vector<Door> doors;
    std::vector<int> item_locations;
    std::vector<int> badge_locations;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

struct Move {
    int to;
    int required;
};

struct Search {
    std::optional<int> doors;
    std::vector<int> route;
};

// BFS over (location, inventory), where the inventory is a bitmask of held badges plus one bit
// for "carrying an item". That is 2^(k+1) copies of the map, which is linear for constant k.
inline Search search(const Site& site, bool want_route) {
    const int badges = static_cast<int>(site.badge_locations.size());
    const int item_bit = 1 << badges;
    const int inventory_states = 1 << (badges + 1);

    std::vector<int> contents(idx(site.locations), 0);
    for (int badge = 0; badge < badges; ++badge) {
        contents[idx(site.badge_locations[idx(badge)])] |= 1 << badge;
    }
    for (const int location : site.item_locations) {
        contents[idx(location)] |= item_bit;
    }

    std::vector<std::vector<Move>> moves(idx(site.locations));
    for (const Door& door : site.doors) {
        const int required = door.badge == kNoBadge ? 0 : 1 << door.badge;
        moves[idx(door.a)].push_back({door.b, required});
        if (door.two_way) {
            moves[idx(door.b)].push_back({door.a, required});
        }
    }

    const auto states = static_cast<std::size_t>(site.locations) * static_cast<std::size_t>(inventory_states);
    const auto encode = [&](int location, int inventory) {
        return location * inventory_states + inventory;
    };
    std::vector<int> dist(states, -1);
    std::vector<int> parent(want_route ? states : 0, -1);
    std::vector<int> queue;
    const int start = encode(site.dock, contents[idx(site.dock)]);
    dist[idx(start)] = 0;
    queue.push_back(start);

    Search out;
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int state = queue[head];
        const int location = state / inventory_states;
        const int inventory = state % inventory_states;
        if (location == site.dock && (inventory & item_bit) != 0) {
            out.doors = dist[idx(state)];
            if (want_route) {
                for (int s = state; s != -1; s = parent[idx(s)]) {
                    out.route.push_back(s / inventory_states);
                }
                std::reverse(out.route.begin(), out.route.end());
            }
            return out;
        }
        for (const Move& move : moves[idx(location)]) {
            if ((inventory & move.required) != move.required) {
                continue;
            }
            const int next = encode(move.to, inventory | contents[idx(move.to)]);
            if (dist[idx(next)] == -1) {
                dist[idx(next)] = dist[idx(state)] + 1;
                if (want_route) {
                    parent[idx(next)] = state;
                }
                queue.push_back(next);
            }
        }
    }
    return out;
}

}  // namespace detail

inline std::optional<int> fewest_doors(const Site& site) {
    return detail::search(site, false).doors;
}

inline std::optional<std::vector<int>> errand_route(const Site& site) {
    auto found = detail::search(site, true);
    if (!found.doors) {
        return std::nullopt;
    }
    return std::move(found.route);
}
