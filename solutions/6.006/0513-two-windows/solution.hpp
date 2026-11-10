#pragma once

#include <algorithm>
#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using Conflict = std::pair<std::string, std::string>;

struct Partition {
    std::vector<std::string> first;
    std::vector<std::string> second;
};

struct OddCycle {
    std::vector<std::string> members;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

struct InternedConflicts {
    std::vector<std::string_view> names;
    std::vector<std::pair<int, int>> edges;
};

// The string_views point into `conflicts`, which outlives every use below.
inline InternedConflicts intern(std::span<const Conflict> conflicts) {
    InternedConflicts out;
    std::unordered_map<std::string_view, int> ids;
    ids.reserve(2 * conflicts.size());
    out.edges.reserve(conflicts.size());
    const auto id_of = [&](std::string_view name) {
        const auto [position, inserted] = ids.try_emplace(name, static_cast<int>(out.names.size()));
        if (inserted) {
            out.names.push_back(name);
        }
        return position->second;
    };
    for (const auto& [a, b] : conflicts) {
        const int x = id_of(a);
        const int y = id_of(b);
        out.edges.emplace_back(x, y);
    }
    return out;
}

struct Coloring {
    std::vector<int> color;
    std::vector<int> parent;
    std::vector<int> depth;
    std::optional<std::pair<int, int>> violated_edge;
};

inline Coloring two_color(int vertex_count, std::span<const std::pair<int, int>> edges) {
    std::vector<int> offsets(idx(vertex_count) + 1, 0);
    for (const auto& [a, b] : edges) {
        ++offsets[idx(a) + 1];
        ++offsets[idx(b) + 1];
    }
    for (std::size_t i = 1; i < offsets.size(); ++i) {
        offsets[i] += offsets[i - 1];
    }
    std::vector<int> fill = offsets;
    std::vector<int> adjacent(2 * edges.size());
    for (const auto& [a, b] : edges) {
        adjacent[idx(fill[idx(a)]++)] = b;
        adjacent[idx(fill[idx(b)]++)] = a;
    }

    Coloring out;
    out.color.assign(idx(vertex_count), -1);
    out.parent.assign(idx(vertex_count), -1);
    out.depth.assign(idx(vertex_count), 0);
    std::vector<int> queue;
    queue.reserve(idx(vertex_count));
    for (int root = 0; root < vertex_count; ++root) {
        if (out.color[idx(root)] != -1) {
            continue;
        }
        out.color[idx(root)] = 0;
        queue.clear();
        queue.push_back(root);
        for (std::size_t head = 0; head < queue.size(); ++head) {
            const int u = queue[head];
            for (int e = offsets[idx(u)]; e < offsets[idx(u) + 1]; ++e) {
                const int v = adjacent[idx(e)];
                if (out.color[idx(v)] == -1) {
                    out.color[idx(v)] = 1 - out.color[idx(u)];
                    out.parent[idx(v)] = u;
                    out.depth[idx(v)] = out.depth[idx(u)] + 1;
                    queue.push_back(v);
                }
            }
        }
    }
    for (const auto& [a, b] : edges) {
        if (out.color[idx(a)] == out.color[idx(b)]) {
            out.violated_edge = std::pair{a, b};
            break;
        }
    }
    return out;
}

// Walks both endpoints of a same-colored edge up to their lowest common ancestor; stopping there
// (rather than at the root) is what keeps the returned cycle simple.
inline std::vector<int> odd_cycle_through(const Coloring& coloring, int a, int b) {
    if (a == b) {
        return {a};
    }
    std::vector<int> up_from_a;
    std::vector<int> up_from_b;
    int x = a;
    int y = b;
    while (coloring.depth[idx(x)] > coloring.depth[idx(y)]) {
        up_from_a.push_back(x);
        x = coloring.parent[idx(x)];
    }
    while (coloring.depth[idx(y)] > coloring.depth[idx(x)]) {
        up_from_b.push_back(y);
        y = coloring.parent[idx(y)];
    }
    while (x != y) {
        up_from_a.push_back(x);
        up_from_b.push_back(y);
        x = coloring.parent[idx(x)];
        y = coloring.parent[idx(y)];
    }
    up_from_a.push_back(x);
    std::reverse(up_from_a.begin(), up_from_a.end());
    up_from_a.insert(up_from_a.end(), up_from_b.begin(), up_from_b.end());
    return up_from_a;
}

}  // namespace detail

inline bool can_fit_two_windows(std::span<const Conflict> conflicts) {
    const auto interned = detail::intern(conflicts);
    return !detail::two_color(static_cast<int>(interned.names.size()), interned.edges).violated_edge;
}

inline std::variant<Partition, OddCycle> plan_two_windows(std::span<const Conflict> conflicts) {
    const auto interned = detail::intern(conflicts);
    const auto coloring = detail::two_color(static_cast<int>(interned.names.size()), interned.edges);
    if (coloring.violated_edge) {
        const auto [a, b] = *coloring.violated_edge;
        OddCycle cycle;
        for (const int v : detail::odd_cycle_through(coloring, a, b)) {
            cycle.members.emplace_back(interned.names[detail::idx(v)]);
        }
        return cycle;
    }
    Partition partition;
    for (std::size_t v = 0; v < interned.names.size(); ++v) {
        (coloring.color[v] == 0 ? partition.first : partition.second).emplace_back(interned.names[v]);
    }
    return partition;
}

// Feasibility only degrades as conflicts are added, so the boundary can be binary searched.
inline std::optional<std::size_t> first_breaking_conflict(std::span<const Conflict> conflicts) {
    const auto interned = detail::intern(conflicts);
    const auto names = static_cast<int>(interned.names.size());
    const auto breaks = [&](std::size_t prefix) {
        return detail::two_color(names, std::span(interned.edges).first(prefix))
            .violated_edge.has_value();
    };
    if (!breaks(conflicts.size())) {
        return std::nullopt;
    }
    std::size_t fine = 0;
    std::size_t broken = conflicts.size();
    while (fine + 1 < broken) {
        const std::size_t middle = fine + (broken - fine) / 2;
        (breaks(middle) ? broken : fine) = middle;
    }
    return broken - 1;
}
