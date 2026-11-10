#pragma once

#include <algorithm>
#include <limits>
#include <numeric>
#include <span>
#include <string>
#include <string_view>
#include <vector>

inline std::string longest_palindromic_subsequence(std::string_view s) {
    const int n = static_cast<int>(s.size());
    if (n == 0) {
        return {};
    }
    // best[i][j] = length of the longest palindromic subsequence of s[i..j]
    std::vector<std::vector<int>> best(n, std::vector<int>(n, 0));
    for (int i = n - 1; i >= 0; --i) {
        best[i][i] = 1;
        for (int j = i + 1; j < n; ++j) {
            if (s[i] == s[j]) {
                best[i][j] = (i + 1 <= j - 1 ? best[i + 1][j - 1] : 0) + 2;
            } else {
                best[i][j] = std::max(best[i + 1][j], best[i][j - 1]);
            }
        }
    }
    std::string left, right;
    int i = 0;
    int j = n - 1;
    while (i <= j) {
        if (i == j) {
            left.push_back(s[i]);
            break;
        }
        if (s[i] == s[j]) {
            left.push_back(s[i]);
            right.push_back(s[j]);
            ++i;
            --j;
        } else if (best[i + 1][j] >= best[i][j - 1]) {
            ++i;
        } else {
            --j;
        }
    }
    std::reverse(right.begin(), right.end());
    return left + right;
}

struct BstPlan {
    long long cost = 0;
    int root = -1;
    std::vector<int> left, right;
};

inline BstPlan optimal_bst(std::span<const long long> weights) {
    const int n = static_cast<int>(weights.size());
    BstPlan plan;
    plan.left.assign(n, -1);
    plan.right.assign(n, -1);
    if (n == 0) {
        return plan;
    }
    std::vector<long long> prefix(n + 1, 0);
    for (int i = 0; i < n; ++i) {
        prefix[i + 1] = prefix[i] + weights[i];
    }
    // cost[i][j], root_of[i][j]: optimal tree on keys i..j-1 (half-open, empty when i == j)
    std::vector<std::vector<long long>> cost(n + 1, std::vector<long long>(n + 1, 0));
    std::vector<std::vector<int>> root_of(n + 1, std::vector<int>(n + 1, -1));
    for (int len = 1; len <= n; ++len) {
        for (int i = 0; i + len <= n; ++i) {
            const int j = i + len;
            long long best = std::numeric_limits<long long>::max();
            for (int r = i; r < j; ++r) {
                const long long candidate = cost[i][r] + cost[r + 1][j];
                if (candidate < best) {
                    best = candidate;
                    root_of[i][j] = r;
                }
            }
            cost[i][j] = best + (prefix[j] - prefix[i]);
        }
    }
    plan.cost = cost[0][n];
    // Rebuild children with an explicit stack: (range, parent, is_left).
    struct Frame {
        int i, j, parent;
        bool is_left;
    };
    std::vector<Frame> stack{{0, n, -1, false}};
    while (!stack.empty()) {
        const Frame frame = stack.back();
        stack.pop_back();
        if (frame.i == frame.j) {
            continue;
        }
        const int r = root_of[frame.i][frame.j];
        if (frame.parent == -1) {
            plan.root = r;
        } else if (frame.is_left) {
            plan.left[frame.parent] = r;
        } else {
            plan.right[frame.parent] = r;
        }
        stack.push_back({frame.i, r, r, true});
        stack.push_back({r + 1, frame.j, r, false});
    }
    return plan;
}

inline long long first_player_value(std::span<const long long> values) {
    const int n = static_cast<int>(values.size());
    if (n == 0) {
        return 0;
    }
    std::vector<long long> prefix(n + 1, 0);
    for (int i = 0; i < n; ++i) {
        prefix[i + 1] = prefix[i] + values[i];
    }
    // value[i][j]: the most the player to move can collect from coins i..j
    std::vector<std::vector<long long>> value(n, std::vector<long long>(n, 0));
    for (int i = n - 1; i >= 0; --i) {
        value[i][i] = values[i];
        for (int j = i + 1; j < n; ++j) {
            const long long total = prefix[j + 1] - prefix[i];
            value[i][j] = total - std::min(value[i + 1][j], value[i][j - 1]);
        }
    }
    return value[0][n - 1];
}

struct Block {
    long long length = 0;
    long long width = 0;
    long long height = 0;
};

inline long long tallest_tower(std::span<const Block> blocks) {
    std::vector<Block> order(blocks.begin(), blocks.end());
    std::sort(order.begin(), order.end(), [](const Block& a, const Block& b) {
        if (a.length != b.length) {
            return a.length > b.length;
        }
        return a.width > b.width;
    });
    std::vector<long long> tallest(order.size(), 0);
    long long best = 0;
    for (std::size_t i = 0; i < order.size(); ++i) {
        tallest[i] = order[i].height;
        for (std::size_t j = 0; j < i; ++j) {
            if (order[i].length <= order[j].length && order[i].width <= order[j].width) {
                tallest[i] = std::max(tallest[i], tallest[j] + order[i].height);
            }
        }
        best = std::max(best, tallest[i]);
    }
    return best;
}

inline long long count_true(std::string_view expr) {
    constexpr long long kMod = 1'000'000'007;
    const int operands = static_cast<int>(expr.size() + 1) / 2;
    std::vector<std::vector<long long>> yes(operands, std::vector<long long>(operands, 0));
    std::vector<std::vector<long long>> no(operands, std::vector<long long>(operands, 0));
    for (int i = operands - 1; i >= 0; --i) {
        yes[i][i] = expr[2 * i] == 'T' ? 1 : 0;
        no[i][i] = expr[2 * i] == 'F' ? 1 : 0;
        for (int j = i + 1; j < operands; ++j) {
            for (int k = i; k < j; ++k) {
                const char op = expr[2 * k + 1];
                const long long lt = yes[i][k], lf = no[i][k];
                const long long rt = yes[k + 1][j], rf = no[k + 1][j];
                const long long tt = lt * rt % kMod, tf = lt * rf % kMod;
                const long long ft = lf * rt % kMod, ff = lf * rf % kMod;
                if (op == '&') {
                    yes[i][j] += tt;
                    no[i][j] += tf + ft + ff;
                } else if (op == '|') {
                    yes[i][j] += tt + tf + ft;
                    no[i][j] += ff;
                } else {
                    yes[i][j] += tf + ft;
                    no[i][j] += tt + ff;
                }
                yes[i][j] %= kMod;
                no[i][j] %= kMod;
            }
        }
    }
    return yes[0][operands - 1];
}
