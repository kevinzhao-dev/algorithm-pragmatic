#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <numeric>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

struct Prize {
    std::int64_t value;
    char type;
};

struct TreePrize {
    std::int64_t value;
    int left;
    int right;
};

namespace prizes_detail {

inline void require_non_negative(std::int64_t value) {
    if (value < 0) {
        throw std::invalid_argument("prize values must be non-negative");
    }
}

}  // namespace prizes_detail

inline std::vector<int> pick_top(std::span<const std::int64_t> values, std::size_t m) {
    for (std::int64_t value : values) {
        prizes_detail::require_non_negative(value);
    }
    const std::size_t n = values.size();
    m = std::min(m, n);
    std::vector<int> order(n);
    std::iota(order.begin(), order.end(), 0);
    if (m < n) {
        std::ranges::nth_element(order, order.begin() + static_cast<std::ptrdiff_t>(m), std::ranges::greater{},
                                 [&](int i) { return values[static_cast<std::size_t>(i)]; });
    }
    std::vector<char> keep(n, 0);
    for (std::size_t i = 0; i < m; ++i) {
        keep[static_cast<std::size_t>(order[i])] = 1;
    }
    std::vector<int> chosen;
    chosen.reserve(m);
    for (std::size_t i = 0; i < n; ++i) {
        if (keep[i]) {
            chosen.push_back(static_cast<int>(i));
        }
    }
    return chosen;
}

inline std::vector<int> pick_a_then_b(std::span<const Prize> prizes, std::size_t m) {
    for (const Prize& prize : prizes) {
        prizes_detail::require_non_negative(prize.value);
        if (prize.type != 'A' && prize.type != 'B') {
            throw std::invalid_argument("prize type must be A or B");
        }
    }
    const std::size_t n = prizes.size();
    m = std::min(m, n);
    if (m == 0) {
        return {};
    }

    // only_a[k]: best using at most k type-A prizes; mixed[k]: best allowing A's then B's.
    std::vector<std::int64_t> only_a(m + 1, 0);
    std::vector<std::int64_t> mixed(m + 1, 0);
    std::vector<std::uint8_t> took_a(n * (m + 1), 0);
    std::vector<std::uint8_t> took_mixed(n * (m + 1), 0);

    for (std::size_t i = 0; i < n; ++i) {
        const std::int64_t value = prizes[i].value;
        for (std::size_t k = m; k >= 1; --k) {
            if (prizes[i].type == 'A') {
                if (value + only_a[k - 1] > mixed[k]) {
                    mixed[k] = value + only_a[k - 1];
                    took_mixed[i * (m + 1) + k] = 1;
                }
                if (value + only_a[k - 1] > only_a[k]) {
                    only_a[k] = value + only_a[k - 1];
                    took_a[i * (m + 1) + k] = 1;
                }
            } else if (value + mixed[k - 1] > mixed[k]) {
                mixed[k] = value + mixed[k - 1];
                took_mixed[i * (m + 1) + k] = 1;
            }
        }
    }

    std::vector<int> chosen;
    std::size_t k = m;
    bool in_mixed = true;
    for (std::size_t i = n; i-- > 0 && k > 0;) {
        const auto& table = in_mixed ? took_mixed : took_a;
        if (table[i * (m + 1) + k]) {
            chosen.push_back(static_cast<int>(i));
            --k;
            if (prizes[i].type == 'A') {
                in_mixed = false;
            }
        }
    }
    std::ranges::reverse(chosen);
    return chosen;
}

inline std::vector<int> pick_non_decreasing(std::span<const std::int64_t> values, std::size_t m) {
    for (std::int64_t value : values) {
        prizes_detail::require_non_negative(value);
    }
    const std::size_t n = values.size();
    m = std::min(m, n);
    if (m == 0) {
        return {};
    }

    std::vector<std::int64_t> sorted(values.begin(), values.end());
    std::ranges::sort(sorted);
    const auto distinct = std::ranges::unique(sorted);
    sorted.erase(distinct.begin(), distinct.end());
    std::vector<std::size_t> rank(n);
    for (std::size_t i = 0; i < n; ++i) {
        rank[i] = static_cast<std::size_t>(std::ranges::lower_bound(sorted, values[i]) - sorted.begin());
    }

    struct Best {
        std::int64_t total = -1;
        int index = -1;
    };
    std::vector<Best> fenwick(sorted.size() + 1);
    const auto reset = [&] { std::ranges::fill(fenwick, Best{}); };
    const auto update = [&](std::size_t r, Best entry) {
        for (std::size_t i = r + 1; i < fenwick.size(); i += i & (~i + 1)) {
            if (entry.total > fenwick[i].total) {
                fenwick[i] = entry;
            }
        }
    };
    const auto query = [&](std::size_t r) {
        Best best;
        for (std::size_t i = r + 1; i > 0; i -= i & (~i + 1)) {
            if (fenwick[i].total > best.total) {
                best = fenwick[i];
            }
        }
        return best;
    };

    // previous[i]: best sum of a chain of the current length ending at i, or -1 if none exists.
    std::vector<std::int64_t> previous(values.begin(), values.end());
    std::vector<std::int64_t> current(n);
    std::vector<std::vector<int>> parent(m, std::vector<int>(n, -1));

    std::int64_t best_total = -1;
    std::size_t best_length = 0;
    int best_end = -1;
    for (std::size_t i = 0; i < n; ++i) {
        if (previous[i] > best_total) {
            best_total = previous[i];
            best_length = 1;
            best_end = static_cast<int>(i);
        }
    }
    for (std::size_t length = 2; length <= m; ++length) {
        reset();
        bool any = false;
        for (std::size_t i = 0; i < n; ++i) {
            const Best found = query(rank[i]);
            if (found.total < 0) {
                current[i] = -1;
            } else {
                current[i] = values[i] + found.total;
                parent[length - 1][i] = found.index;
                any = true;
                if (current[i] > best_total) {
                    best_total = current[i];
                    best_length = length;
                    best_end = static_cast<int>(i);
                }
            }
            if (previous[i] >= 0) {
                update(rank[i], Best{previous[i], static_cast<int>(i)});
            }
        }
        if (!any) {
            break;
        }
        previous.swap(current);
    }

    std::vector<int> chosen;
    for (int index = best_end; best_length > 0; --best_length) {
        chosen.push_back(index);
        index = parent[best_length - 1][static_cast<std::size_t>(index)];
    }
    std::ranges::reverse(chosen);
    return chosen;
}

inline std::vector<int> pick_rooted_subtree(std::span<const TreePrize> tree, int root, std::size_t m) {
    if (tree.empty()) {
        return {};
    }
    if (root < 0 || static_cast<std::size_t>(root) >= tree.size()) {
        throw std::invalid_argument("root is not a node of the tree");
    }
    for (const TreePrize& prize : tree) {
        prizes_detail::require_non_negative(prize.value);
    }
    if (m == 0) {
        return {};
    }

    // Explicit stack: a recursive walk would overflow on a long path.
    std::vector<int> preorder;
    preorder.reserve(tree.size());
    preorder.push_back(root);
    for (std::size_t head = 0; head < preorder.size(); ++head) {
        const TreePrize& node = tree[static_cast<std::size_t>(preorder[head])];
        for (int child : {node.left, node.right}) {
            if (child != -1) {
                preorder.push_back(child);
            }
        }
    }

    const std::size_t n = tree.size();
    std::vector<std::vector<std::int64_t>> best(n);
    std::vector<std::vector<std::uint32_t>> left_share(n);
    const std::vector<std::int64_t> nothing{0};

    for (std::size_t position = preorder.size(); position-- > 0;) {
        const auto u = static_cast<std::size_t>(preorder[position]);
        const TreePrize& node = tree[u];
        const auto& left = node.left == -1 ? nothing : best[static_cast<std::size_t>(node.left)];
        const auto& right = node.right == -1 ? nothing : best[static_cast<std::size_t>(node.right)];

        // Each pair of a left and right count is one pair of nodes, so the total is O(n m).
        const std::size_t size = std::min(left.size() + right.size() - 1, m);
        std::vector<std::int64_t> merged(size, -1);
        std::vector<std::uint32_t> share(size, 0);
        for (std::size_t j = 0; j < left.size(); ++j) {
            for (std::size_t r = 0; r < right.size() && j + r < size; ++r) {
                const std::int64_t total = left[j] + right[r];
                if (total > merged[j + r]) {
                    merged[j + r] = total;
                    share[j + r] = static_cast<std::uint32_t>(j);
                }
            }
        }

        best[u].resize(size + 1);
        best[u][0] = 0;
        for (std::size_t k = 1; k <= size; ++k) {
            best[u][k] = node.value + merged[k - 1];
        }
        left_share[u] = std::move(share);
        if (node.left != -1) {
            best[static_cast<std::size_t>(node.left)] = {};
        }
        if (node.right != -1) {
            best[static_cast<std::size_t>(node.right)] = {};
        }
    }

    std::vector<int> chosen;
    std::vector<std::pair<int, std::size_t>> pending{{root, best[static_cast<std::size_t>(root)].size() - 1}};
    while (!pending.empty()) {
        const auto [u, budget] = pending.back();
        pending.pop_back();
        if (u == -1 || budget == 0) {
            continue;
        }
        chosen.push_back(u);
        const std::size_t to_left = left_share[static_cast<std::size_t>(u)][budget - 1];
        const TreePrize& node = tree[static_cast<std::size_t>(u)];
        pending.emplace_back(node.left, to_left);
        pending.emplace_back(node.right, budget - 1 - to_left);
    }
    std::ranges::sort(chosen);
    return chosen;
}
