#pragma once

#include <algorithm>
#include <functional>
#include <numeric>
#include <optional>
#include <span>
#include <utility>
#include <vector>

struct Edge {
    int u = 0;
    int v = 0;
    long long w = 0;
};

namespace variants_detail {

class Dsu {
public:
    explicit Dsu(int n) : parent_(n) { std::iota(parent_.begin(), parent_.end(), 0); }
    int find(int x) {
        while (parent_[x] != x) {
            parent_[x] = parent_[parent_[x]];
            x = parent_[x];
        }
        return x;
    }
    bool unite(int a, int b) {
        a = find(a);
        b = find(b);
        if (a == b) {
            return false;
        }
        parent_[a] = b;
        return true;
    }

private:
    std::vector<int> parent_;
};

}  // namespace variants_detail

inline bool has_unique_mst(int n, std::span<const Edge> edges) {
    std::vector<int> order(edges.size());
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(),
              [&](int a, int b) { return edges[a].w < edges[b].w; });
    variants_detail::Dsu sets(n);
    for (std::size_t i = 0; i < order.size();) {
        std::size_t j = i;
        while (j < order.size() && edges[order[j]].w == edges[order[i]].w) {
            ++j;
        }
        // Edges of this weight joining distinct components: all of them must be used,
        // otherwise some of them are interchangeable.
        int candidates = 0;
        for (std::size_t k = i; k < j; ++k) {
            const auto& e = edges[order[k]];
            candidates += sets.find(e.u) != sets.find(e.v) ? 1 : 0;
        }
        int merged = 0;
        for (std::size_t k = i; k < j; ++k) {
            const auto& e = edges[order[k]];
            merged += sets.unite(e.u, e.v) ? 1 : 0;
        }
        if (candidates != merged) {
            return false;
        }
        i = j;
    }
    return true;
}

struct BoruvkaResult {
    std::vector<int> edges;
    int phases = 0;
};

inline BoruvkaResult boruvka(int n, std::span<const Edge> edges) {
    BoruvkaResult out;
    variants_detail::Dsu sets(n);
    int components = n;
    while (components > 1) {
        ++out.phases;
        std::vector<int> best(n, -1);
        for (int id = 0; id < static_cast<int>(edges.size()); ++id) {
            const int a = sets.find(edges[id].u);
            const int b = sets.find(edges[id].v);
            if (a == b) {
                continue;
            }
            if (best[a] == -1 || edges[id].w < edges[best[a]].w) {
                best[a] = id;
            }
            if (best[b] == -1 || edges[id].w < edges[best[b]].w) {
                best[b] = id;
            }
        }
        for (int v = 0; v < n; ++v) {
            if (best[v] != -1 && sets.unite(edges[best[v]].u, edges[best[v]].v)) {
                out.edges.push_back(best[v]);
                --components;
            }
        }
    }
    std::sort(out.edges.begin(), out.edges.end());
    return out;
}

inline std::vector<int> divide_and_conquer_mst(int n, std::span<const Edge> edges) {
    std::vector<int> ids(edges.size());
    std::iota(ids.begin(), ids.end(), 0);
    const std::function<std::vector<int>(int, int, std::vector<int>)> solve =
        [&](int lo, int hi, std::vector<int> current) -> std::vector<int> {
        if (hi - lo <= 1) {
            return {};
        }
        const int mid = lo + (hi - lo) / 2;
        std::vector<int> left, right;
        int cross = -1;
        for (int id : current) {
            const bool u_left = edges[id].u < mid;
            const bool v_left = edges[id].v < mid;
            if (u_left && v_left) {
                left.push_back(id);
            } else if (!u_left && !v_left) {
                right.push_back(id);
            } else if (cross == -1 || edges[id].w < edges[cross].w) {
                cross = id;
            }
        }
        auto out = solve(lo, mid, std::move(left));
        auto r = solve(mid, hi, std::move(right));
        out.insert(out.end(), r.begin(), r.end());
        if (cross != -1) {
            out.push_back(cross);
        }
        return out;
    };
    auto result = solve(0, n, std::move(ids));
    std::sort(result.begin(), result.end());
    return result;
}

struct DcTrap {
    int n = 0;
    std::vector<Edge> edges;
};

inline DcTrap divide_and_conquer_trap() {
    // A 4-cycle 0-1-3-2-0 split as {0,1} | {2,3}: the heavy inner edges are forced, but the two
    // light crossing edges are both needed by the true MST.
    return {4, {{0, 1, 90}, {2, 3, 80}, {0, 2, 1}, {1, 3, 2}}};
}

inline std::vector<int> cycle_breaking_mst(int n, std::span<const Edge> edges) {
    const int m = static_cast<int>(edges.size());
    std::vector<char> alive(m, 1);
    std::vector<std::vector<std::pair<int, int>>> adj(n);
    for (int id = 0; id < m; ++id) {
        adj[edges[id].u].emplace_back(edges[id].v, id);
        adj[edges[id].v].emplace_back(edges[id].u, id);
    }
    // Finds a cycle with DFS; returns the ids of its edges, or empty when acyclic.
    const auto find_cycle = [&]() -> std::vector<int> {
        std::vector<int> depth(n, -1);
        std::vector<int> parent_edge(n, -1);
        std::vector<int> parent_vertex(n, -1);
        std::vector<int> stack;
        for (int s = 0; s < n; ++s) {
            if (depth[s] != -1) {
                continue;
            }
            depth[s] = 0;
            stack.push_back(s);
            while (!stack.empty()) {
                const int v = stack.back();
                stack.pop_back();
                for (const auto& [to, id] : adj[v]) {
                    if (!alive[id] || id == parent_edge[v]) {
                        continue;
                    }
                    if (depth[to] == -1) {
                        depth[to] = depth[v] + 1;
                        parent_edge[to] = id;
                        parent_vertex[to] = v;
                        stack.push_back(to);
                    } else if (depth[to] <= depth[v]) {
                        // A non-tree edge: walk both ends up to their meeting point.
                        std::vector<int> cycle{id};
                        int a = v;
                        int b = to;
                        while (a != b) {
                            if (depth[a] >= depth[b]) {
                                cycle.push_back(parent_edge[a]);
                                a = parent_vertex[a];
                            } else {
                                cycle.push_back(parent_edge[b]);
                                b = parent_vertex[b];
                            }
                        }
                        return cycle;
                    }
                }
            }
        }
        return {};
    };
    while (true) {
        const auto cycle = find_cycle();
        if (cycle.empty()) {
            break;
        }
        int heaviest = cycle.front();
        for (int id : cycle) {
            if (edges[id].w > edges[heaviest].w) {
                heaviest = id;
            }
        }
        alive[heaviest] = 0;
    }
    std::vector<int> result;
    for (int id = 0; id < m; ++id) {
        if (alive[id]) {
            result.push_back(id);
        }
    }
    return result;
}

class IncrementalMst {
public:
    explicit IncrementalMst(int n) : adj_(n), total_(0) {}

    std::optional<int> add_edge(int u, int v, long long w) {
        const int id = static_cast<int>(edges_.size());
        edges_.push_back({u, v, w});
        alive_.push_back(0);

        // Find the current tree path u..v with an explicit-stack DFS.
        std::vector<int> parent(adj_.size(), -2);
        std::vector<int> via(adj_.size(), -1);
        std::vector<int> stack{u};
        parent[u] = -1;
        while (!stack.empty() && parent[v] == -2) {
            const int x = stack.back();
            stack.pop_back();
            for (const auto& [to, edge] : adj_[x]) {
                if (parent[to] == -2) {
                    parent[to] = x;
                    via[to] = edge;
                    stack.push_back(to);
                }
            }
        }
        if (parent[v] == -2) {  // u and v are in different components
            link(id);
            return std::nullopt;
        }
        int heaviest = id;
        long long worst = w;
        for (int x = v; x != u; x = parent[x]) {
            if (edges_[via[x]].w > worst) {
                worst = edges_[via[x]].w;
                heaviest = via[x];
            }
        }
        if (heaviest != id) {
            unlink(heaviest);
            link(id);
        }
        return heaviest;
    }

    long long weight() const { return total_; }

    std::vector<int> edges() const {
        std::vector<int> out;
        for (int id = 0; id < static_cast<int>(edges_.size()); ++id) {
            if (alive_[id]) {
                out.push_back(id);
            }
        }
        return out;
    }

private:
    void link(int id) {
        alive_[id] = 1;
        total_ += edges_[id].w;
        adj_[edges_[id].u].emplace_back(edges_[id].v, id);
        adj_[edges_[id].v].emplace_back(edges_[id].u, id);
    }

    void unlink(int id) {
        alive_[id] = 0;
        total_ -= edges_[id].w;
        erase_arc(edges_[id].u, id);
        erase_arc(edges_[id].v, id);
    }

    void erase_arc(int v, int id) {
        auto& arcs = adj_[v];
        for (std::size_t i = 0; i < arcs.size(); ++i) {
            if (arcs[i].second == id) {
                arcs[i] = arcs.back();
                arcs.pop_back();
                return;
            }
        }
    }

    std::vector<std::vector<std::pair<int, int>>> adj_;  // (neighbor, edge id), forest only
    std::vector<Edge> edges_;
    std::vector<char> alive_;
    long long total_;
};
