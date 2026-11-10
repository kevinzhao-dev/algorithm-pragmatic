#pragma once

#include <algorithm>
#include <functional>
#include <numeric>
#include <queue>
#include <span>
#include <tuple>
#include <utility>
#include <vector>

struct WeightedEdge {
    int u = 0;
    int v = 0;
    long long w = 0;
};

struct SpanningForest {
    long long total = 0;
    std::vector<int> edges;
};

namespace mst_detail {

class UnionFind {
public:
    explicit UnionFind(int n) : parent_(n), rank_(n, 0), components_(n) {
        std::iota(parent_.begin(), parent_.end(), 0);
    }

    int find(int x) {
        int root = x;
        while (parent_[root] != root) {
            root = parent_[root];
        }
        while (parent_[x] != root) {
            x = std::exchange(parent_[x], root);
        }
        return root;
    }

    bool unite(int x, int y) {
        int a = find(x);
        int b = find(y);
        if (a == b) {
            return false;
        }
        if (rank_[a] < rank_[b]) {
            std::swap(a, b);
        }
        parent_[b] = a;
        if (rank_[a] == rank_[b]) {
            ++rank_[a];
        }
        --components_;
        return true;
    }

    int components() const { return components_; }

private:
    std::vector<int> parent_;
    std::vector<int> rank_;
    int components_;
};

inline std::vector<int> edges_by_weight(std::span<const WeightedEdge> edges) {
    std::vector<int> order(edges.size());
    std::iota(order.begin(), order.end(), 0);
    std::stable_sort(order.begin(), order.end(),
                     [&](int a, int b) { return edges[a].w < edges[b].w; });
    return order;
}

}  // namespace mst_detail

inline SpanningForest kruskal(int n, std::span<const WeightedEdge> edges) {
    SpanningForest out;
    mst_detail::UnionFind sets(n);
    for (int id : mst_detail::edges_by_weight(edges)) {
        if (sets.unite(edges[id].u, edges[id].v)) {
            out.total += edges[id].w;
            out.edges.push_back(id);
        }
    }
    return out;
}

inline SpanningForest prim(int n, std::span<const WeightedEdge> edges) {
    struct Arc {
        int to, id;
        long long w;
    };
    std::vector<std::vector<Arc>> adj(n);
    for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
        const auto& e = edges[id];
        adj[e.u].push_back({e.v, id, e.w});
        adj[e.v].push_back({e.u, id, e.w});
    }
    SpanningForest out;
    std::vector<char> done(n, 0);
    using Entry = std::tuple<long long, int, int>;  // (weight, vertex, edge id)
    for (int start = 0; start < n; ++start) {
        if (done[start]) {
            continue;
        }
        std::priority_queue<Entry, std::vector<Entry>, std::greater<>> heap;
        heap.emplace(0, start, -1);
        while (!heap.empty()) {
            const auto [w, v, id] = heap.top();
            heap.pop();
            if (done[v]) {
                continue;
            }
            done[v] = 1;
            if (id != -1) {
                out.total += w;
                out.edges.push_back(id);
            }
            for (const Arc& arc : adj[v]) {
                if (!done[arc.to]) {
                    heap.emplace(arc.w, arc.to, arc.id);
                }
            }
        }
    }
    return out;
}

inline std::vector<int> cluster(int n, std::span<const WeightedEdge> edges, int k) {
    mst_detail::UnionFind sets(n);
    for (int id : mst_detail::edges_by_weight(edges)) {
        if (sets.components() == k) {
            break;
        }
        sets.unite(edges[id].u, edges[id].v);
    }
    std::vector<int> label(n, -1);
    std::vector<int> of_root(n, -1);
    int next = 0;
    for (int v = 0; v < n; ++v) {
        const int root = sets.find(v);
        if (of_root[root] == -1) {
            of_root[root] = next++;
        }
        label[v] = of_root[root];
    }
    return label;
}
