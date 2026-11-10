#pragma once

#include <algorithm>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <utility>
#include <vector>

struct Network {
    std::vector<std::int64_t> value;
    std::vector<std::pair<int, int>> edges;

    std::size_t size() const { return value.size(); }
};

namespace placement_detail {

struct RootedTree {
    std::vector<int> order;   // every parent appears before its children
    std::vector<int> parent;  // -1 for the root
};

// Iterative BFS: a recursive walk would overflow the stack on a long path.
inline RootedTree root_at_zero(const Network& tree) {
    const std::size_t n = tree.size();
    std::vector<int> offset(n + 1, 0);
    for (const auto& [a, b] : tree.edges) {
        ++offset[static_cast<std::size_t>(a) + 1];
        ++offset[static_cast<std::size_t>(b) + 1];
    }
    std::partial_sum(offset.begin(), offset.end(), offset.begin());
    std::vector<int> neighbors(offset[n]);
    std::vector<int> cursor(offset.begin(), offset.end() - 1);
    for (const auto& [a, b] : tree.edges) {
        neighbors[static_cast<std::size_t>(cursor[static_cast<std::size_t>(a)]++)] = b;
        neighbors[static_cast<std::size_t>(cursor[static_cast<std::size_t>(b)]++)] = a;
    }

    RootedTree rooted;
    rooted.parent.assign(n, -1);
    rooted.order.reserve(n);
    if (n != 0) {
        rooted.order.push_back(0);
    }
    for (std::size_t head = 0; head < rooted.order.size(); ++head) {
        const int u = rooted.order[head];
        for (int i = offset[static_cast<std::size_t>(u)]; i < offset[static_cast<std::size_t>(u) + 1]; ++i) {
            const int v = neighbors[static_cast<std::size_t>(i)];
            if (v != rooted.parent[static_cast<std::size_t>(u)]) {
                rooted.parent[static_cast<std::size_t>(v)] = u;
                rooted.order.push_back(v);
            }
        }
    }
    return rooted;
}

}  // namespace placement_detail

inline std::vector<int> greedy_by_value(const Network& network) {
    const std::size_t n = network.size();
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    std::ranges::stable_sort(order, std::ranges::greater{},
                             [&](int v) { return network.value[static_cast<std::size_t>(v)]; });

    std::vector<std::vector<int>> adjacent(n);
    for (const auto& [a, b] : network.edges) {
        adjacent[static_cast<std::size_t>(a)].push_back(b);
        adjacent[static_cast<std::size_t>(b)].push_back(a);
    }
    std::vector<char> blocked(n, 0);
    std::vector<int> chosen;
    for (int v : order) {
        if (!blocked[static_cast<std::size_t>(v)]) {
            chosen.push_back(v);
            for (int u : adjacent[static_cast<std::size_t>(v)]) {
                blocked[static_cast<std::size_t>(u)] = 1;
            }
        }
    }
    std::ranges::sort(chosen);
    return chosen;
}

inline Network greedy_counterexample() {
    return Network{{5, 6, 5}, {{0, 1}, {1, 2}}};
}

inline std::vector<int> max_value_placement(const Network& tree) {
    const std::size_t n = tree.size();
    if (n == 0) {
        return {};
    }
    const auto [order, parent] = placement_detail::root_at_zero(tree);
    std::vector<std::int64_t> with(n, 0);
    std::vector<std::int64_t> without(n, 0);
    for (std::size_t i = n; i-- > 0;) {
        const auto v = static_cast<std::size_t>(order[i]);
        with[v] += tree.value[v];
        if (parent[v] != -1) {
            const auto p = static_cast<std::size_t>(parent[v]);
            with[p] += without[v];
            without[p] += std::max(with[v], without[v]);
        }
    }

    std::vector<char> taken(n, 0);
    std::vector<int> chosen;
    for (int v : order) {
        const auto u = static_cast<std::size_t>(v);
        const bool parent_taken = parent[u] != -1 && taken[static_cast<std::size_t>(parent[u])];
        if (!parent_taken && with[u] >= without[u]) {
            taken[u] = 1;
            chosen.push_back(v);
        }
    }
    std::ranges::sort(chosen);
    return chosen;
}

inline std::vector<int> max_count_placement(const Network& tree) {
    const std::size_t n = tree.size();
    const auto [order, parent] = placement_detail::root_at_zero(tree);
    std::vector<char> blocked(n, 0);
    std::vector<int> chosen;
    for (std::size_t i = n; i-- > 0;) {
        const auto v = static_cast<std::size_t>(order[i]);
        if (!blocked[v]) {
            chosen.push_back(static_cast<int>(v));
            if (parent[v] != -1) {
                blocked[static_cast<std::size_t>(parent[v])] = 1;
            }
        }
    }
    std::ranges::sort(chosen);
    return chosen;
}

namespace placement_detail {

class ExactSolver {
public:
    explicit ExactSolver(const Network& network)
        : value_(network.value), adjacent_(network.size(), 0) {
        for (const auto& [a, b] : network.edges) {
            adjacent_[static_cast<std::size_t>(a)] |= std::uint64_t{1} << b;
            adjacent_[static_cast<std::size_t>(b)] |= std::uint64_t{1} << a;
        }
    }

    std::pair<std::int64_t, std::uint64_t> solve(std::uint64_t active) const {
        if (active == 0) {
            return {0, 0};
        }
        int pivot = -1;
        int pivot_degree = -1;
        for (std::uint64_t rest = active; rest != 0; rest &= rest - 1) {
            const int v = std::countr_zero(rest);
            const int degree = std::popcount(adjacent_[static_cast<std::size_t>(v)] & active);
            if (degree > pivot_degree) {
                pivot = v;
                pivot_degree = degree;
            }
        }
        if (pivot_degree <= 1) {
            return solve_matching(active);
        }

        const std::uint64_t pivot_bit = std::uint64_t{1} << pivot;
        const auto without = solve(active & ~pivot_bit);
        auto with = solve(active & ~pivot_bit & ~adjacent_[static_cast<std::size_t>(pivot)]);
        with.first += value_[static_cast<std::size_t>(pivot)];
        with.second |= pivot_bit;
        return with.first > without.first ? with : without;
    }

private:
    // Every remaining vertex is isolated or belongs to a single edge.
    std::pair<std::int64_t, std::uint64_t> solve_matching(std::uint64_t active) const {
        std::int64_t total = 0;
        std::uint64_t chosen = 0;
        for (std::uint64_t rest = active; rest != 0;) {
            const int v = std::countr_zero(rest);
            const auto index = static_cast<std::size_t>(v);
            rest &= rest - 1;
            const std::uint64_t neighbor = adjacent_[index] & active;
            int winner = v;
            if (neighbor != 0) {
                const int u = std::countr_zero(neighbor);
                if (value_[static_cast<std::size_t>(u)] > value_[index]) {
                    winner = u;
                }
                rest &= ~neighbor;
            }
            total += value_[static_cast<std::size_t>(winner)];
            chosen |= std::uint64_t{1} << winner;
        }
        return {total, chosen};
    }

    const std::vector<std::int64_t>& value_;
    std::vector<std::uint64_t> adjacent_;
};

}  // namespace placement_detail

inline std::vector<int> max_value_placement_general(const Network& network) {
    const std::size_t n = network.size();
    const placement_detail::ExactSolver solver(network);
    const std::uint64_t everyone = n == 64 ? ~std::uint64_t{0} : (std::uint64_t{1} << n) - 1;
    const std::uint64_t mask = solver.solve(everyone).second;
    std::vector<int> chosen;
    for (std::size_t v = 0; v < n; ++v) {
        if ((mask >> v) & 1) {
            chosen.push_back(static_cast<int>(v));
        }
    }
    return chosen;
}
