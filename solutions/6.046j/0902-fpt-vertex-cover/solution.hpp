#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <span>
#include <utility>
#include <vector>

using EdgeList = std::span<const std::pair<int, int>>;

namespace fpt_detail {

// Bounded search on an edge list with a removable-vertex mask.
inline bool branch(int n, const std::vector<std::pair<int, int>>& edges, int k,
                   std::vector<char>& removed, std::vector<int>& cover) {
    for (const auto& [u, v] : edges) {
        if (removed[u] || removed[v]) {
            continue;
        }
        if (k == 0) {
            return false;
        }
        for (int choice : {u, v}) {
            removed[choice] = 1;
            cover.push_back(choice);
            if (branch(n, edges, k - 1, removed, cover)) {
                return true;
            }
            cover.pop_back();
            removed[choice] = 0;
        }
        return false;
    }
    return true;  // every edge is covered
}

}  // namespace fpt_detail

inline std::optional<std::vector<int>> vc_search(int n, EdgeList edges, int k) {
    std::vector<std::pair<int, int>> list(edges.begin(), edges.end());
    std::vector<char> removed(n, 0);
    std::vector<int> cover;
    if (k < 0 || !fpt_detail::branch(n, list, k, removed, cover)) {
        return std::nullopt;
    }
    return cover;
}

struct Kernel {
    bool no_instance = false;
    int n = 0;
    std::vector<std::pair<int, int>> edges;
    int k = 0;
    std::vector<int> forced;
    std::vector<int> to_original;
};

inline Kernel kernelize(int n, EdgeList edges, int k) {
    Kernel kernel;
    std::vector<std::pair<int, int>> unique;
    unique.reserve(edges.size());
    for (auto [u, v] : edges) {
        unique.emplace_back(std::min(u, v), std::max(u, v));
    }
    std::sort(unique.begin(), unique.end());
    unique.erase(std::unique(unique.begin(), unique.end()), unique.end());
    std::vector<std::vector<int>> adj(n);
    for (const auto& [u, v] : unique) {
        adj[u].push_back(v);
        adj[v].push_back(u);
    }
    std::vector<char> gone(n, 0);
    int budget = k;
    bool changed = true;
    while (changed && budget >= 0) {
        changed = false;
        for (int v = 0; v < n; ++v) {
            if (gone[v]) {
                continue;
            }
            int degree = 0;
            for (int w : adj[v]) {
                degree += gone[w] ? 0 : 1;
            }
            if (degree > budget) {
                gone[v] = 1;
                kernel.forced.push_back(v);
                --budget;
                changed = true;
                break;
            }
        }
    }
    if (budget < 0) {
        kernel.no_instance = true;
        return kernel;
    }
    std::vector<std::pair<int, int>> remaining;
    std::vector<char> touched(n, 0);
    for (const auto& [u, v] : unique) {
        if (!gone[u] && !gone[v]) {
            remaining.emplace_back(u, v);
            touched[u] = touched[v] = 1;
        }
    }
    if (static_cast<long long>(remaining.size()) >
        static_cast<long long>(budget) * budget) {
        kernel.no_instance = true;
        return kernel;
    }
    std::vector<int> new_id(n, -1);
    for (int v = 0; v < n; ++v) {
        if (touched[v]) {
            new_id[v] = kernel.n++;
            kernel.to_original.push_back(v);
        }
    }
    for (const auto& [u, v] : remaining) {
        kernel.edges.emplace_back(new_id[u], new_id[v]);
    }
    kernel.k = budget;
    return kernel;
}

inline std::optional<std::vector<int>> vc_fpt(int n, EdgeList edges, int k) {
    const Kernel kernel = kernelize(n, edges, k);
    if (kernel.no_instance) {
        return std::nullopt;
    }
    auto inner = vc_search(kernel.n, kernel.edges, kernel.k);
    if (!inner) {
        return std::nullopt;
    }
    std::vector<int> cover = kernel.forced;
    for (int v : *inner) {
        cover.push_back(kernel.to_original[v]);
    }
    return cover;
}

inline std::vector<int> min_vertex_cover(int n, EdgeList edges) {
    // A maximal matching is a lower bound: every matched edge needs its own cover vertex.
    std::vector<char> matched(n, 0);
    int lower_bound = 0;
    for (const auto& [u, v] : edges) {
        if (!matched[u] && !matched[v]) {
            matched[u] = matched[v] = 1;
            ++lower_bound;
        }
    }
    for (int k = lower_bound;; ++k) {
        if (auto cover = vc_fpt(n, edges, k)) {
            return *cover;
        }
    }
}
