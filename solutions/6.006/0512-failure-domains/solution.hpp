#pragma once

#include <algorithm>
#include <cstddef>
#include <span>
#include <utility>
#include <vector>

struct Topology {
    int feeds;
    int racks;
    std::vector<std::pair<int, int>> links;
};

struct BackupChoice {
    int feed;
    int racks_covered;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

struct Incident {
    int to;
    int link;
};

inline std::vector<std::vector<Incident>> adjacency_of(const Topology& topology) {
    std::vector<std::vector<Incident>> adjacency(idx(topology.feeds + topology.racks));
    for (int i = 0; i < static_cast<int>(topology.links.size()); ++i) {
        const auto& [a, b] = topology.links[idx(i)];
        adjacency[idx(a)].push_back({b, i});
        adjacency[idx(b)].push_back({a, i});
    }
    return adjacency;
}

}  // namespace detail

inline BackupChoice choose_backup_feed(const Topology& topology) {
    const auto adjacency = detail::adjacency_of(topology);
    std::vector<char> seen(adjacency.size(), 0);
    std::vector<int> stack;
    BackupChoice best{0, -1};
    for (int feed = 0; feed < topology.feeds; ++feed) {
        int racks = 0;
        seen[detail::idx(feed)] = 1;
        stack.push_back(feed);
        while (!stack.empty()) {
            const int u = stack.back();
            stack.pop_back();
            if (u >= topology.feeds) {
                ++racks;
            }
            for (const auto& [to, link] : adjacency[detail::idx(u)]) {
                if (seen[detail::idx(to)] == 0) {
                    seen[detail::idx(to)] = 1;
                    stack.push_back(to);
                }
            }
        }
        if (racks > best.racks_covered) {
            best = {feed, racks};
        }
    }
    return best;
}

inline std::vector<int> racks_without_power(const Topology& topology,
                                            std::span<const std::size_t> cut_links) {
    std::vector<char> cut(topology.links.size(), 0);
    for (const std::size_t link : cut_links) {
        cut[link] = 1;
    }
    const auto adjacency = detail::adjacency_of(topology);
    std::vector<char> powered(adjacency.size(), 0);
    std::vector<int> stack;
    for (int feed = 0; feed < topology.feeds; ++feed) {
        powered[detail::idx(feed)] = 1;
        stack.push_back(feed);
    }
    while (!stack.empty()) {
        const int u = stack.back();
        stack.pop_back();
        for (const auto& [to, link] : adjacency[detail::idx(u)]) {
            if (cut[detail::idx(link)] == 0 && powered[detail::idx(to)] == 0) {
                powered[detail::idx(to)] = 1;
                stack.push_back(to);
            }
        }
    }
    std::vector<int> dark;
    for (int v = topology.feeds; v < topology.feeds + topology.racks; ++v) {
        if (powered[detail::idx(v)] == 0) {
            dark.push_back(v);
        }
    }
    return dark;
}

// Bridge finding: a link is a single point of failure exactly when the subtree hanging below it
// has no other way up. Each DFS is rooted at a feed, so "below" is the side that loses power.
inline std::vector<int> blast_radius(const Topology& topology) {
    const auto adjacency = detail::adjacency_of(topology);
    const std::size_t n = adjacency.size();
    std::vector<int> answer(topology.links.size(), 0);
    std::vector<int> entered(n, -1);
    std::vector<int> low(n, 0);
    std::vector<int> racks_below(n, 0);

    struct Frame {
        int vertex;
        int entering_link;
        std::size_t next;
    };
    std::vector<Frame> stack;
    int clock = 0;
    for (int feed = 0; feed < topology.feeds; ++feed) {
        entered[detail::idx(feed)] = low[detail::idx(feed)] = clock++;
        stack.push_back({feed, -1, 0});
        while (!stack.empty()) {
            const int u = stack.back().vertex;
            if (stack.back().next < adjacency[detail::idx(u)].size()) {
                const auto [to, link] = adjacency[detail::idx(u)][stack.back().next++];
                if (link == stack.back().entering_link) {
                    continue;
                }
                if (entered[detail::idx(to)] != -1) {
                    low[detail::idx(u)] = std::min(low[detail::idx(u)], entered[detail::idx(to)]);
                } else {
                    entered[detail::idx(to)] = low[detail::idx(to)] = clock++;
                    stack.push_back({to, link, 0});
                }
                continue;
            }
            const int entering_link = stack.back().entering_link;
            stack.pop_back();
            racks_below[detail::idx(u)] += u >= topology.feeds ? 1 : 0;
            if (!stack.empty()) {
                const int p = stack.back().vertex;
                low[detail::idx(p)] = std::min(low[detail::idx(p)], low[detail::idx(u)]);
                racks_below[detail::idx(p)] += racks_below[detail::idx(u)];
                if (low[detail::idx(u)] > entered[detail::idx(p)]) {
                    answer[detail::idx(entering_link)] = racks_below[detail::idx(u)];
                }
            }
        }
    }
    return answer;
}
