#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <optional>
#include <utility>
#include <vector>

inline constexpr int kZoneA = -1;
inline constexpr int kZoneB = -2;

using TileMap = std::vector<std::vector<int>>;
using Corner = std::pair<int, int>;  // (r, c)

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

class Lattice {
public:
    explicit Lattice(const TileMap& tiles) : tiles_(tiles), n_(static_cast<int>(tiles.size())) {}

    int size() const { return n_; }
    int vertices() const { return (n_ + 1) * (n_ + 1); }
    int id(int r, int c) const { return r * (n_ + 1) + c; }
    Corner corner(int id) const { return {id / (n_ + 1), id % (n_ + 1)}; }

    // Interior segments have a tile on each side; the outer boundary never conflicts.
    bool horizontal_open(int r, int c) const {
        if (r == 0 || r == n_) {
            return true;
        }
        return !same_tenant(tiles_[idx(r - 1)][idx(c)], tiles_[idx(r)][idx(c)]);
    }

    bool vertical_open(int r, int c) const {
        if (c == 0 || c == n_) {
            return true;
        }
        return !same_tenant(tiles_[idx(r)][idx(c - 1)], tiles_[idx(r)][idx(c)]);
    }

    template <typename Visit>
    void for_each_open_neighbor(int r, int c, Visit&& visit) const {
        if (c > 0 && horizontal_open(r, c - 1)) {
            visit(r, c - 1);
        }
        if (c < n_ && horizontal_open(r, c)) {
            visit(r, c + 1);
        }
        if (r > 0 && vertical_open(r - 1, c)) {
            visit(r - 1, c);
        }
        if (r < n_ && vertical_open(r, c)) {
            visit(r + 1, c);
        }
    }

private:
    static bool same_tenant(int a, int b) { return a >= 0 && a == b; }

    const TileMap& tiles_;
    int n_;
};

struct Search {
    std::vector<int> parent;
    int end = -1;  // reached B-corner, -1 when none
    int length = 0;
};

// Seeding the queue with every A-corner at distance zero replaces a separate BFS per corner.
inline Search search(const TileMap& tiles) {
    const Lattice lattice(tiles);
    const int n = lattice.size();
    std::vector<int> dist(idx(lattice.vertices()), -1);
    std::vector<char> is_target(idx(lattice.vertices()), 0);
    Search out{std::vector<int>(idx(lattice.vertices()), -1)};
    std::vector<int> queue;
    queue.reserve(idx(lattice.vertices()));
    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            if (tiles[idx(r)][idx(c)] != kZoneA && tiles[idx(r)][idx(c)] != kZoneB) {
                continue;
            }
            const bool source = tiles[idx(r)][idx(c)] == kZoneA;
            for (const auto& [dr, dc] : std::array<Corner, 4>{{{0, 0}, {0, 1}, {1, 0}, {1, 1}}}) {
                const int corner = lattice.id(r + dr, c + dc);
                if (source && dist[idx(corner)] == -1) {
                    dist[idx(corner)] = 0;
                    queue.push_back(corner);
                }
                if (!source) {
                    is_target[idx(corner)] = 1;
                }
            }
        }
    }
    for (std::size_t head = 0; head < queue.size(); ++head) {
        const int current = queue[head];
        if (is_target[idx(current)] != 0) {
            out.end = current;
            out.length = dist[idx(current)];
            return out;
        }
        const auto [r, c] = lattice.corner(current);
        lattice.for_each_open_neighbor(r, c, [&](int nr, int nc) {
            const int next = lattice.id(nr, nc);
            if (dist[idx(next)] == -1) {
                dist[idx(next)] = dist[idx(current)] + 1;
                out.parent[idx(next)] = current;
                queue.push_back(next);
            }
        });
    }
    return out;
}

}  // namespace detail

inline std::optional<int> shortest_tray_length(const TileMap& tiles) {
    const detail::Search found = detail::search(tiles);
    if (found.end == -1) {
        return std::nullopt;
    }
    return found.length;
}

inline std::optional<std::vector<Corner>> shortest_tray_route(const TileMap& tiles) {
    const detail::Search found = detail::search(tiles);
    if (found.end == -1) {
        return std::nullopt;
    }
    const detail::Lattice lattice(tiles);
    std::vector<Corner> route;
    for (int v = found.end; v != -1; v = found.parent[detail::idx(v)]) {
        route.push_back(lattice.corner(v));
    }
    std::reverse(route.begin(), route.end());
    return route;
}
