#pragma once

#include <algorithm>
#include <array>
#include <numeric>
#include <optional>
#include <utility>
#include <vector>

using Tournament = std::vector<std::vector<char>>;

namespace tournament_detail {

inline std::vector<int> scores(const Tournament& t) {
    const int n = static_cast<int>(t.size());
    std::vector<int> out(n, 0);
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            out[u] += t[u][v];
        }
    }
    return out;
}

}  // namespace tournament_detail

inline bool is_acyclic(const Tournament& t) {
    const int n = static_cast<int>(t.size());
    auto s = tournament_detail::scores(t);
    std::sort(s.begin(), s.end());
    for (int i = 0; i < n; ++i) {
        if (s[i] != i) {
            return false;
        }
    }
    return true;
}

inline std::optional<std::array<int, 3>> find_triangle(const Tournament& t) {
    const int n = static_cast<int>(t.size());
    const auto s = tournament_detail::scores(t);
    // An edge u -> v with score(u) <= score(v) exists iff the tournament is not transitive.
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            if (u == v || !t[u][v] || s[u] > s[v]) {
                continue;
            }
            for (int w = 0; w < n; ++w) {
                if (w != u && w != v && t[v][w] && t[w][u]) {
                    return std::array<int, 3>{u, v, w};
                }
            }
        }
    }
    return std::nullopt;
}

struct TournamentKernel {
    bool no_instance = false;
    Tournament reduced;
    int k = 0;
    std::vector<int> to_original;
    std::vector<std::pair<int, int>> reversed;
};

inline TournamentKernel kernelize(const Tournament& input, int k) {
    TournamentKernel out;
    Tournament t = input;
    const int n = static_cast<int>(t.size());
    std::vector<char> alive(n, 1);
    bool changed = true;
    while (changed) {
        changed = false;
        if (k < 0) {
            out.no_instance = true;
            return out;
        }
        // Count triangles through every edge.
        std::vector<std::vector<int>> through(n, std::vector<int>(n, 0));
        std::vector<int> vertex_triangles(n, 0);
        for (int a = 0; a < n; ++a) {
            for (int b = 0; b < n; ++b) {
                if (a == b || !alive[a] || !alive[b] || !t[a][b]) {
                    continue;
                }
                for (int c = 0; c < n; ++c) {
                    if (c != a && c != b && alive[c] && t[b][c] && t[c][a]) {
                        ++through[a][b];
                        ++vertex_triangles[a];
                    }
                }
            }
        }
        // Rule 1: an edge in at least k+1 triangles belongs to every solution of size <= k.
        for (int a = 0; a < n && !changed; ++a) {
            for (int b = 0; b < n && !changed; ++b) {
                if (alive[a] && alive[b] && t[a][b] && through[a][b] >= k + 1) {
                    t[a][b] = 0;
                    t[b][a] = 1;
                    out.reversed.emplace_back(a, b);
                    --k;
                    changed = true;
                }
            }
        }
        if (changed) {
            continue;
        }
        // Rule 2: a vertex in no triangle can be deleted.
        for (int v = 0; v < n; ++v) {
            if (alive[v] && vertex_triangles[v] == 0) {
                alive[v] = 0;
                changed = true;
            }
        }
    }
    if (k < 0) {
        out.no_instance = true;
        return out;
    }
    for (int v = 0; v < n; ++v) {
        if (alive[v]) {
            out.to_original.push_back(v);
        }
    }
    const int m = static_cast<int>(out.to_original.size());
    if (m > k * (k + 2)) {
        out.no_instance = true;
        return out;
    }
    out.reduced.assign(m, std::vector<char>(m, 0));
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < m; ++j) {
            out.reduced[i][j] = t[out.to_original[i]][out.to_original[j]];
        }
    }
    out.k = k;
    return out;
}

namespace tournament_detail {

inline bool search(Tournament& t, int k, std::vector<std::pair<int, int>>& picked) {
    const auto triangle = find_triangle(t);
    if (!triangle) {
        return true;
    }
    if (k == 0) {
        return false;
    }
    const auto [a, b, c] = *triangle;
    for (const auto& [u, v] : std::array<std::pair<int, int>, 3>{{{a, b}, {b, c}, {c, a}}}) {
        t[u][v] = 0;
        t[v][u] = 1;
        picked.emplace_back(u, v);
        if (search(t, k - 1, picked)) {
            return true;
        }
        picked.pop_back();
        t[u][v] = 1;
        t[v][u] = 0;
    }
    return false;
}

}  // namespace tournament_detail

inline std::optional<std::vector<std::pair<int, int>>> feedback_arcs(const Tournament& t, int k) {
    TournamentKernel kernel = kernelize(t, k);
    if (kernel.no_instance) {
        return std::nullopt;
    }
    std::vector<std::pair<int, int>> picked;
    if (!tournament_detail::search(kernel.reduced, kernel.k, picked)) {
        return std::nullopt;
    }
    auto result = kernel.reversed;
    for (const auto& [u, v] : picked) {
        result.emplace_back(kernel.to_original[u], kernel.to_original[v]);
    }
    return result;
}

inline std::optional<std::vector<int>> repair_ranking(const Tournament& t, int max_upsets) {
    for (int k = 0; k <= max_upsets; ++k) {
        const auto arcs = feedback_arcs(t, k);
        if (!arcs) {
            continue;
        }
        Tournament fixed = t;
        for (const auto& [u, v] : *arcs) {
            fixed[u][v] = 0;
            fixed[v][u] = 1;
        }
        const int n = static_cast<int>(t.size());
        const auto s = tournament_detail::scores(fixed);
        std::vector<int> order(n);
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return s[a] > s[b]; });
        return order;
    }
    return std::nullopt;
}
