#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <variant>
#include <vector>

using AdjacencyList = std::vector<std::vector<int>>;

inline constexpr int kNoParent = -1;

struct DfsForest {
    std::vector<int> parent;
    std::vector<int> finish_order;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

struct Frame {
    int vertex;
    std::size_t next_neighbor;
};

inline void check_vertex(int v, std::size_t n) {
    if (v < 0 || idx(v) >= n) {
        throw std::out_of_range("no such vertex");
    }
}

// Mirrors the recursion of textbook DFS with an explicit stack, so deep graphs cannot blow the
// call stack and neighbours are still tried in adjacency order.
inline void explore_from(const AdjacencyList& g, int root, std::vector<char>& visited,
                         DfsForest& out) {
    std::vector<Frame> stack;
    visited[idx(root)] = 1;
    stack.push_back({root, 0});
    while (!stack.empty()) {
        const int u = stack.back().vertex;
        if (stack.back().next_neighbor == g[idx(u)].size()) {
            out.finish_order.push_back(u);
            stack.pop_back();
            continue;
        }
        const int v = g[idx(u)][stack.back().next_neighbor++];
        if (visited[idx(v)] == 0) {
            visited[idx(v)] = 1;
            out.parent[idx(v)] = u;
            stack.push_back({v, 0});
        }
    }
}

}  // namespace detail

inline DfsForest dfs(const AdjacencyList& g, int source) {
    detail::check_vertex(source, g.size());
    DfsForest out{std::vector<int>(g.size(), kNoParent), {}};
    std::vector<char> visited(g.size(), 0);
    detail::explore_from(g, source, visited, out);
    return out;
}

inline DfsForest full_dfs(const AdjacencyList& g) {
    DfsForest out{std::vector<int>(g.size(), kNoParent), {}};
    out.finish_order.reserve(g.size());
    std::vector<char> visited(g.size(), 0);
    for (int root = 0; root < static_cast<int>(g.size()); ++root) {
        if (visited[detail::idx(root)] == 0) {
            detail::explore_from(g, root, visited, out);
        }
    }
    return out;
}

inline std::vector<int> connected_components(const AdjacencyList& g) {
    std::vector<int> label(g.size(), -1);
    std::vector<int> stack;
    int components = 0;
    for (int root = 0; root < static_cast<int>(g.size()); ++root) {
        if (label[detail::idx(root)] != -1) {
            continue;
        }
        label[detail::idx(root)] = components;
        stack.push_back(root);
        while (!stack.empty()) {
            const int u = stack.back();
            stack.pop_back();
            for (const int v : g[detail::idx(u)]) {
                if (label[detail::idx(v)] == -1) {
                    label[detail::idx(v)] = components;
                    stack.push_back(v);
                }
            }
        }
        ++components;
    }
    return label;
}

struct TopologicalOrder {
    std::vector<int> vertices;
};

struct DirectedCycle {
    std::vector<int> vertices;
};

inline std::variant<TopologicalOrder, DirectedCycle> topological_sort(const AdjacencyList& g) {
    enum Color : char { kUnseen, kOnStack, kFinished };
    std::vector<char> color(g.size(), kUnseen);
    std::vector<int> parent(g.size(), kNoParent);
    std::vector<int> finished;
    finished.reserve(g.size());
    std::vector<detail::Frame> stack;
    for (int root = 0; root < static_cast<int>(g.size()); ++root) {
        if (color[detail::idx(root)] != kUnseen) {
            continue;
        }
        color[detail::idx(root)] = kOnStack;
        stack.push_back({root, 0});
        while (!stack.empty()) {
            const int u = stack.back().vertex;
            if (stack.back().next_neighbor == g[detail::idx(u)].size()) {
                color[detail::idx(u)] = kFinished;
                finished.push_back(u);
                stack.pop_back();
                continue;
            }
            const int v = g[detail::idx(u)][stack.back().next_neighbor++];
            if (color[detail::idx(v)] == kOnStack) {
                std::vector<int> cycle;
                for (int x = u; x != v; x = parent[detail::idx(x)]) {
                    cycle.push_back(x);
                }
                cycle.push_back(v);
                std::reverse(cycle.begin(), cycle.end());
                return DirectedCycle{std::move(cycle)};
            }
            if (color[detail::idx(v)] == kUnseen) {
                color[detail::idx(v)] = kOnStack;
                parent[detail::idx(v)] = u;
                stack.push_back({v, 0});
            }
        }
    }
    std::reverse(finished.begin(), finished.end());
    return TopologicalOrder{std::move(finished)};
}

class CycleError : public std::runtime_error {
public:
    explicit CycleError(std::vector<int> cycle)
        : std::runtime_error("build graph has a cycle"), cycle_(std::move(cycle)) {}
    const std::vector<int>& cycle() const noexcept { return cycle_; }

private:
    std::vector<int> cycle_;
};

class BuildGraph {
public:
    BuildGraph(int targets, std::span<const std::pair<int, int>> edges)
        : downstream_(static_cast<std::size_t>(targets)),
          stamp_(static_cast<std::size_t>(targets), 0) {
        for (const auto& [before, after] : edges) {
            detail::check_vertex(before, downstream_.size());
            detail::check_vertex(after, downstream_.size());
            downstream_[detail::idx(before)].push_back(after);
        }
        auto sorted = topological_sort(downstream_);
        if (auto* cycle = std::get_if<DirectedCycle>(&sorted)) {
            throw CycleError(std::move(cycle->vertices));
        }
    }

    std::vector<int> rebuild_order(std::span<const int> changed) {
        for (const int target : changed) {
            detail::check_vertex(target, downstream_.size());
        }
        // Stamping instead of clearing keeps a query proportional to what it actually touches.
        if (++epoch_ == 0) {
            std::fill(stamp_.begin(), stamp_.end(), 0);
            epoch_ = 1;
        }
        std::vector<int> finished;
        std::vector<detail::Frame> stack;
        for (const int root : changed) {
            if (stamp_[detail::idx(root)] == epoch_) {
                continue;
            }
            stamp_[detail::idx(root)] = epoch_;
            stack.push_back({root, 0});
            while (!stack.empty()) {
                const int u = stack.back().vertex;
                const auto& adjacency = downstream_[detail::idx(u)];
                if (stack.back().next_neighbor == adjacency.size()) {
                    finished.push_back(u);
                    stack.pop_back();
                    continue;
                }
                const int v = adjacency[stack.back().next_neighbor++];
                if (stamp_[detail::idx(v)] != epoch_) {
                    stamp_[detail::idx(v)] = epoch_;
                    stack.push_back({v, 0});
                }
            }
        }
        std::reverse(finished.begin(), finished.end());
        return finished;
    }

private:
    AdjacencyList downstream_;
    std::vector<std::uint32_t> stamp_;
    std::uint32_t epoch_ = 0;
};
