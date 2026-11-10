#pragma once

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>
#include <span>
#include <utility>
#include <vector>

struct Matching {
    int size = 0;
    std::vector<int> match_left;
    std::vector<int> match_right;
};
struct VertexCover {
    std::vector<int> left;
    std::vector<int> right;
};

namespace matching_detail {

using Adjacency = std::vector<std::vector<int>>;

inline Adjacency adjacency(int left, std::span<const std::pair<int, int>> edges) {
    Adjacency adj(left);
    for (const auto& [l, r] : edges) {
        adj[l].push_back(r);
    }
    return adj;
}

// Marks the vertices reachable from free left vertices along alternating paths.
inline std::pair<std::vector<char>, std::vector<char>> alternating_reach(
    const Adjacency& adj, const Matching& m, int right) {
    const int left = static_cast<int>(adj.size());
    std::vector<char> reach_left(left, 0), reach_right(right, 0);
    std::queue<int> frontier;
    for (int l = 0; l < left; ++l) {
        if (m.match_left[l] == -1) {
            reach_left[l] = 1;
            frontier.push(l);
        }
    }
    while (!frontier.empty()) {
        const int l = frontier.front();
        frontier.pop();
        for (int r : adj[l]) {
            if (!reach_right[r]) {
                reach_right[r] = 1;
                const int next = m.match_right[r];
                if (next != -1 && !reach_left[next]) {
                    reach_left[next] = 1;
                    frontier.push(next);
                }
            }
        }
    }
    return {std::move(reach_left), std::move(reach_right)};
}

}  // namespace matching_detail

inline Matching hopcroft_karp(int left, int right, std::span<const std::pair<int, int>> edges) {
    const auto adj = matching_detail::adjacency(left, edges);
    Matching m;
    m.match_left.assign(left, -1);
    m.match_right.assign(right, -1);
    std::vector<int> dist(left);
    const int kNone = std::numeric_limits<int>::max();
    const auto bfs = [&] {
        std::queue<int> frontier;
        for (int l = 0; l < left; ++l) {
            dist[l] = m.match_left[l] == -1 ? 0 : kNone;
            if (m.match_left[l] == -1) {
                frontier.push(l);
            }
        }
        bool found = false;
        while (!frontier.empty()) {
            const int l = frontier.front();
            frontier.pop();
            for (int r : adj[l]) {
                const int next = m.match_right[r];
                if (next == -1) {
                    found = true;
                } else if (dist[next] == kNone) {
                    dist[next] = dist[l] + 1;
                    frontier.push(next);
                }
            }
        }
        return found;
    };
    const std::function<bool(int)> dfs = [&](int l) {
        for (int r : adj[l]) {
            const int next = m.match_right[r];
            if (next == -1 || (dist[next] == dist[l] + 1 && dfs(next))) {
                m.match_left[l] = r;
                m.match_right[r] = l;
                return true;
            }
        }
        dist[l] = kNone;
        return false;
    };
    while (bfs()) {
        for (int l = 0; l < left; ++l) {
            if (m.match_left[l] == -1 && dfs(l)) {
                ++m.size;
            }
        }
    }
    return m;
}

inline Matching max_matching(int left, int right, std::span<const std::pair<int, int>> edges) {
    return hopcroft_karp(left, right, edges);
}

inline VertexCover min_vertex_cover(int left, int right,
                                    std::span<const std::pair<int, int>> edges) {
    const auto adj = matching_detail::adjacency(left, edges);
    const Matching m = hopcroft_karp(left, right, edges);
    const auto [reach_left, reach_right] = matching_detail::alternating_reach(adj, m, right);
    VertexCover cover;
    for (int l = 0; l < left; ++l) {
        if (!reach_left[l]) {
            cover.left.push_back(l);
        }
    }
    for (int r = 0; r < right; ++r) {
        if (reach_right[r]) {
            cover.right.push_back(r);
        }
    }
    return cover;
}

inline std::vector<int> hall_violator(int left, int right,
                                      std::span<const std::pair<int, int>> edges) {
    const Matching m = hopcroft_karp(left, right, edges);
    if (m.size == left) {
        return {};
    }
    const auto adj = matching_detail::adjacency(left, edges);
    const auto [reach_left, reach_right] = matching_detail::alternating_reach(adj, m, right);
    std::vector<int> violator;
    for (int l = 0; l < left; ++l) {
        if (reach_left[l]) {
            violator.push_back(l);
        }
    }
    return violator;
}
