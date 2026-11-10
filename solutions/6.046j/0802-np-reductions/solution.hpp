#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <set>
#include <span>
#include <utility>
#include <vector>

struct Graph {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
};

namespace reductions_detail {

inline std::set<std::pair<int, int>> edge_set(const Graph& g) {
    std::set<std::pair<int, int>> out;
    for (auto [u, v] : g.edges) {
        if (u > v) {
            std::swap(u, v);
        }
        out.insert({u, v});
    }
    return out;
}

inline std::vector<int> complement_of(int n, std::span<const int> vertices) {
    std::vector<char> in(n, 0);
    for (int v : vertices) {
        in[v] = 1;
    }
    std::vector<int> out;
    for (int v = 0; v < n; ++v) {
        if (!in[v]) {
            out.push_back(v);
        }
    }
    return out;
}

}  // namespace reductions_detail

inline bool is_clique(const Graph& g, std::span<const int> vertices) {
    const auto edges = reductions_detail::edge_set(g);
    for (std::size_t i = 0; i < vertices.size(); ++i) {
        for (std::size_t j = i + 1; j < vertices.size(); ++j) {
            const auto [a, b] = std::minmax(vertices[i], vertices[j]);
            if (edges.count({a, b}) == 0) {
                return false;
            }
        }
    }
    return true;
}

inline bool is_independent_set(const Graph& g, std::span<const int> vertices) {
    std::vector<char> in(g.n, 0);
    for (int v : vertices) {
        in[v] = 1;
    }
    for (const auto& [u, v] : g.edges) {
        if (in[u] && in[v]) {
            return false;
        }
    }
    return true;
}

inline bool is_vertex_cover(const Graph& g, std::span<const int> vertices) {
    std::vector<char> in(g.n, 0);
    for (int v : vertices) {
        in[v] = 1;
    }
    for (const auto& [u, v] : g.edges) {
        if (!in[u] && !in[v]) {
            return false;
        }
    }
    return true;
}

inline bool is_set_cover(int universe, const std::vector<std::vector<int>>& sets,
                         std::span<const int> chosen) {
    std::vector<char> covered(universe, 0);
    for (int index : chosen) {
        for (int element : sets[index]) {
            covered[element] = 1;
        }
    }
    return std::all_of(covered.begin(), covered.end(), [](char c) { return c != 0; });
}

struct CliqueInstance {
    Graph g;
    int k = 0;
};
struct IsInstance {
    Graph g;
    int k = 0;
};
struct VcInstance {
    Graph g;
    int k = 0;
};
struct SetCoverInstance {
    int universe = 0;
    std::vector<std::vector<int>> sets;
    int k = 0;
};

inline IsInstance clique_to_is(const CliqueInstance& src) {
    IsInstance out;
    out.g.n = src.g.n;
    out.k = src.k;
    const auto edges = reductions_detail::edge_set(src.g);
    for (int u = 0; u < src.g.n; ++u) {
        for (int v = u + 1; v < src.g.n; ++v) {
            if (edges.count({u, v}) == 0) {
                out.g.edges.emplace_back(u, v);
            }
        }
    }
    return out;
}

inline VcInstance is_to_vc(const IsInstance& src) {
    VcInstance out;
    out.g = src.g;
    out.k = src.g.n - src.k;
    return out;
}

inline SetCoverInstance vc_to_set_cover(const VcInstance& src) {
    SetCoverInstance out;
    out.universe = static_cast<int>(src.g.edges.size());
    out.k = src.k;
    out.sets.resize(src.g.n);
    for (int e = 0; e < out.universe; ++e) {
        out.sets[src.g.edges[e].first].push_back(e);
        out.sets[src.g.edges[e].second].push_back(e);
    }
    return out;
}

inline std::vector<int> lift_is_to_clique(const CliqueInstance& src, std::span<const int> witness) {
    return {witness.begin(), witness.end()};
}

inline std::vector<int> lift_vc_to_is(const IsInstance& src, std::span<const int> witness) {
    return reductions_detail::complement_of(src.g.n, witness);
}

inline std::vector<int> lift_set_cover_to_vc(const VcInstance& src, const SetCoverInstance& dst,
                                             std::span<const int> chosen) {
    return {chosen.begin(), chosen.end()};
}

struct SubsetSumInstance {
    std::vector<long long> a;
    long long t = 0;
};
struct PartitionInstance {
    std::vector<long long> a;
};

inline PartitionInstance subset_sum_to_partition(const SubsetSumInstance& src) {
    const long long sigma = std::accumulate(src.a.begin(), src.a.end(), 0LL);
    PartitionInstance out;
    out.a = src.a;
    out.a.push_back(sigma + src.t);       // index n
    out.a.push_back(2 * sigma - src.t);   // index n + 1
    return out;
}

inline std::vector<int> lift_partition_to_subset_sum(const SubsetSumInstance& src,
                                                     const PartitionInstance& dst,
                                                     std::span<const int> side) {
    const int n = static_cast<int>(src.a.size());
    // The side containing the "2σ - t" element holds, among the originals, exactly a subset of
    // sum t; the other side holds the complement.
    std::vector<char> on_side(dst.a.size(), 0);
    for (int index : side) {
        on_side[index] = 1;
    }
    const bool t_side_is_given = on_side[n + 1];
    std::vector<int> out;
    for (int i = 0; i < n; ++i) {
        if ((on_side[i] != 0) == t_side_is_given) {
            out.push_back(i);
        }
    }
    return out;
}

struct Digraph {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
};

inline Digraph ham_cycle_to_path(const Digraph& g) {
    // Vertex 0 is split: 0 keeps the outgoing edges, the new vertex n receives the incoming ones.
    Digraph out;
    out.n = g.n + 1;
    for (const auto& [u, v] : g.edges) {
        out.edges.emplace_back(u, v == 0 ? g.n : v);
    }
    return out;
}

inline std::vector<int> lift_path_to_cycle(const Digraph& src, const Digraph& dst,
                                           std::span<const int> path) {
    std::vector<int> out;
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (path[i] != src.n) {  // drop the copy of vertex 0
            out.push_back(path[i]);
        }
    }
    return out;
}

struct ThreeDm {
    int n = 0;
    std::vector<std::array<int, 3>> triples;
};

inline SubsetSumInstance three_dm_to_subset_sum(const ThreeDm& t) {
    SubsetSumInstance out;
    const long long base = static_cast<long long>(t.triples.size()) + 1;
    const auto digit = [&](int position) {
        long long value = 1;
        for (int i = 0; i < position; ++i) {
            value *= base;
        }
        return value;
    };
    for (const auto& [x, y, z] : t.triples) {
        out.a.push_back(digit(x) + digit(t.n + y) + digit(2 * t.n + z));
    }
    for (int position = 0; position < 3 * t.n; ++position) {
        out.t += digit(position);
    }
    return out;
}

inline std::vector<int> lift_subset_sum_to_3dm(const ThreeDm& src, const SubsetSumInstance& dst,
                                               std::span<const int> chosen) {
    return {chosen.begin(), chosen.end()};
}
