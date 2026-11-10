#pragma once

#include <algorithm>
#include <cmath>
#include <functional>
#include <limits>
#include <numeric>
#include <queue>
#include <span>
#include <utility>
#include <vector>

inline std::vector<int> approx_vertex_cover(int n, std::span<const std::pair<int, int>> edges) {
    std::vector<char> picked(n, 0);
    for (const auto& [u, v] : edges) {
        if (!picked[u] && !picked[v]) {
            picked[u] = picked[v] = 1;
        }
    }
    std::vector<int> cover;
    for (int v = 0; v < n; ++v) {
        if (picked[v]) {
            cover.push_back(v);
        }
    }
    return cover;
}

inline std::vector<int> greedy_set_cover(int universe, const std::vector<std::vector<int>>& sets) {
    const int m = static_cast<int>(sets.size());
    std::vector<int> gain(m);
    std::priority_queue<std::pair<int, int>> heap;  // (recorded gain, set index)
    for (int i = 0; i < m; ++i) {
        gain[i] = static_cast<int>(sets[i].size());
        heap.emplace(gain[i], i);
    }
    std::vector<char> covered(universe, 0);
    int remaining = universe;
    std::vector<int> chosen;
    while (remaining > 0 && !heap.empty()) {
        const auto [recorded, i] = heap.top();
        heap.pop();
        if (recorded != gain[i]) {
            continue;  // stale entry
        }
        // Recompute the true gain lazily.
        int fresh = 0;
        for (int element : sets[i]) {
            fresh += covered[element] ? 0 : 1;
        }
        if (fresh != gain[i]) {
            gain[i] = fresh;
            heap.emplace(fresh, i);
            continue;
        }
        if (fresh == 0) {
            break;
        }
        chosen.push_back(i);
        for (int element : sets[i]) {
            if (!covered[element]) {
                covered[element] = 1;
                --remaining;
            }
        }
    }
    std::sort(chosen.begin(), chosen.end());
    return chosen;
}

struct TwoWaySplit {
    std::vector<int> side_a;
    long long cost = 0;
};

inline TwoWaySplit approx_partition(std::span<const long long> weights, double eps) {
    const int n = static_cast<int>(weights.size());
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return weights[a] > weights[b]; });

    int m = static_cast<int>(std::ceil(1.0 / eps)) - 1;
    m = std::clamp(m, 0, std::min(n, 20));
    // Phase one: best split of the m largest items, by subset enumeration.
    long long best = std::numeric_limits<long long>::max();
    int best_mask = 0;
    long long prefix_sum = 0;
    for (int i = 0; i < m; ++i) {
        prefix_sum += weights[order[i]];
    }
    for (int mask = 0; mask < (1 << m); ++mask) {
        long long a = 0;
        for (int i = 0; i < m; ++i) {
            a += (mask >> i & 1) ? weights[order[i]] : 0;
        }
        const long long cost = std::max(a, prefix_sum - a);
        if (cost < best) {
            best = cost;
            best_mask = mask;
        }
    }
    long long load_a = 0, load_b = 0;
    std::vector<char> in_a(n, 0);
    for (int i = 0; i < m; ++i) {
        if (best_mask >> i & 1) {
            in_a[order[i]] = 1;
            load_a += weights[order[i]];
        } else {
            load_b += weights[order[i]];
        }
    }
    // Phase two: remaining items in decreasing order go to the lighter side.
    for (int i = m; i < n; ++i) {
        if (load_a <= load_b) {
            in_a[order[i]] = 1;
            load_a += weights[order[i]];
        } else {
            load_b += weights[order[i]];
        }
    }
    TwoWaySplit split;
    split.cost = std::max(load_a, load_b);
    for (int i = 0; i < n; ++i) {
        if (in_a[i]) {
            split.side_a.push_back(i);
        }
    }
    return split;
}

inline std::vector<int> approx_tsp(const std::vector<std::vector<long long>>& dist) {
    const int n = static_cast<int>(dist.size());
    if (n == 0) {
        return {};
    }
    // Prim's algorithm on the dense metric, O(n^2).
    std::vector<long long> key(n, std::numeric_limits<long long>::max());
    std::vector<int> parent(n, -1);
    std::vector<char> done(n, 0);
    key[0] = 0;
    for (int round = 0; round < n; ++round) {
        int u = -1;
        for (int v = 0; v < n; ++v) {
            if (!done[v] && (u == -1 || key[v] < key[u])) {
                u = v;
            }
        }
        done[u] = 1;
        for (int v = 0; v < n; ++v) {
            if (!done[v] && dist[u][v] < key[v]) {
                key[v] = dist[u][v];
                parent[v] = u;
            }
        }
    }
    std::vector<std::vector<int>> children(n);
    for (int v = 1; v < n; ++v) {
        children[parent[v]].push_back(v);
    }
    std::vector<int> tour;
    std::vector<int> stack{0};
    while (!stack.empty()) {
        const int v = stack.back();
        stack.pop_back();
        tour.push_back(v);
        for (auto it = children[v].rbegin(); it != children[v].rend(); ++it) {
            stack.push_back(*it);
        }
    }
    return tour;
}
