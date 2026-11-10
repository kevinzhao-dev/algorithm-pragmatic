#pragma once

#include <algorithm>
#include <limits>
#include <vector>

inline constexpr long long kInf = std::numeric_limits<long long>::max() / 4;
using Matrix = std::vector<std::vector<long long>>;

class DynamicApsp {
public:
    explicit DynamicApsp(Matrix w) : w_(std::move(w)), n_(static_cast<int>(w_.size())) { rebuild(); }

    long long dist(int u, int v) const { return d_[u][v]; }

    std::vector<int> path(int u, int v) const {
        if (d_[u][v] == kInf) {
            return {};
        }
        std::vector<int> reversed{v};
        while (v != u) {
            v = pred_[u][v];
            reversed.push_back(v);
        }
        std::reverse(reversed.begin(), reversed.end());
        return reversed;
    }

    long long update(int i, int j, long long r) {
        if (i == j) {
            return 0;
        }
        const long long old = w_[i][j];
        if (r == old) {
            return 0;
        }
        w_[i][j] = r;
        if (r > old) {
            return increase(i, j, old);
        }
        long long changed = 0;
        for (int x = 0; x < n_; ++x) {
            if (d_[x][i] == kInf) {
                continue;
            }
            for (int y = 0; y < n_; ++y) {
                if (d_[j][y] != kInf && d_[x][i] + r + d_[j][y] < d_[x][y]) {
                    d_[x][y] = d_[x][i] + r + d_[j][y];
                    pred_[x][y] = y == j ? i : pred_[j][y];
                    ++changed;
                }
            }
        }
        return changed;
    }

private:
    long long increase(int i, int j, long long old) {
        // If the old edge was on no shortest path, nothing can get worse.
        if (d_[i][j] < old) {
            return 0;
        }
        const Matrix before = d_;
        rebuild();
        long long changed = 0;
        for (int x = 0; x < n_; ++x) {
            for (int y = 0; y < n_; ++y) {
                changed += before[x][y] != d_[x][y] ? 1 : 0;
            }
        }
        return changed;
    }

    void rebuild() {
        d_ = w_;
        pred_.assign(n_, std::vector<int>(n_, -1));
        for (int i = 0; i < n_; ++i) {
            for (int j = 0; j < n_; ++j) {
                if (i != j && d_[i][j] != kInf) {
                    pred_[i][j] = i;
                }
            }
        }
        for (int k = 0; k < n_; ++k) {
            for (int i = 0; i < n_; ++i) {
                if (d_[i][k] == kInf) {
                    continue;
                }
                for (int j = 0; j < n_; ++j) {
                    if (d_[k][j] != kInf && d_[i][k] + d_[k][j] < d_[i][j]) {
                        d_[i][j] = d_[i][k] + d_[k][j];
                        pred_[i][j] = pred_[k][j];
                    }
                }
            }
        }
    }

    Matrix w_;
    int n_;
    Matrix d_;
    std::vector<std::vector<int>> pred_;
};

struct Scenario {
    Matrix w;
    int i = 0;
    int j = 0;
    long long r = 0;
};

inline Scenario worst_update_scenario(int n) {
    // A path 0 -> 1 -> ... -> n-1; adding the back edge (n-1, 0) makes every pair reachable.
    Matrix w(n, std::vector<long long>(n, kInf));
    for (int i = 0; i < n; ++i) {
        w[i][i] = 0;
        if (i + 1 < n) {
            w[i][i + 1] = 1;
        }
    }
    return {std::move(w), n - 1, 0, 1};
}

inline Matrix bounded_hop_floyd(const Matrix& w, int h) {
    const int n = static_cast<int>(w.size());
    // table[b][i][j]: at most b hops, intermediates restricted to the first k vertices.
    std::vector<Matrix> table(h + 1, Matrix(n, std::vector<long long>(n, kInf)));
    for (int b = 0; b <= h; ++b) {
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                table[b][i][j] = (i == j) ? 0 : (b >= 1 ? w[i][j] : kInf);
            }
        }
    }
    for (int k = 0; k < n; ++k) {
        std::vector<Matrix> next = table;
        for (int i = 0; i < n; ++i) {
            for (int j = 0; j < n; ++j) {
                for (int b = 0; b <= h; ++b) {
                    for (int first = 0; first <= b; ++first) {
                        const long long via_k = table[first][i][k];
                        const long long rest = table[b - first][k][j];
                        if (via_k != kInf && rest != kInf) {
                            next[b][i][j] = std::min(next[b][i][j], via_k + rest);
                        }
                    }
                }
            }
        }
        table = std::move(next);
    }
    return table[h];
}

namespace bounded_hop_detail {

inline Matrix min_plus(const Matrix& a, const Matrix& b) {
    const std::size_t n = a.size();
    Matrix c(n, std::vector<long long>(n, kInf));
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t k = 0; k < n; ++k) {
            if (a[i][k] == kInf) {
                continue;
            }
            for (std::size_t j = 0; j < n; ++j) {
                if (b[k][j] != kInf) {
                    c[i][j] = std::min(c[i][j], a[i][k] + b[k][j]);
                }
            }
        }
    }
    return c;
}

}  // namespace bounded_hop_detail

inline Matrix bounded_hop_doubling(const Matrix& w, int h) {
    const int n = static_cast<int>(w.size());
    Matrix result(n, std::vector<long long>(n, kInf));
    for (int i = 0; i < n; ++i) {
        result[i][i] = 0;
    }
    if (h == 0) {
        return result;
    }
    int top = 0;
    while ((1 << (top + 1)) <= h) {
        ++top;
    }
    result = w;  // the bit `top` of h is always set
    for (int bit = top - 1; bit >= 0; --bit) {
        result = bounded_hop_detail::min_plus(result, result);
        if (h >> bit & 1) {
            result = bounded_hop_detail::min_plus(result, w);
        }
    }
    return result;
}

class DynamicBoundedApsp {
public:
    DynamicBoundedApsp(Matrix w, int h) : w_(std::move(w)), h_(h), n_(static_cast<int>(w_.size())) {
        rebuild();
    }

    long long dist(int u, int v) const { return levels_[h_][u][v]; }
    long long dist_at_most(int u, int v, int k) const { return levels_[k][u][v]; }

    void update(int i, int j, long long r) {
        if (i == j) {
            return;
        }
        const long long old = w_[i][j];
        if (r == old) {
            return;
        }
        w_[i][j] = r;
        if (r > old) {
            rebuild();
            return;
        }
        // Decrease: every improved path uses (i,j) once; split it at that edge.
        for (int k = 1; k <= h_; ++k) {
            for (int a = 0; a < k; ++a) {
                const int b = k - 1 - a;
                for (int x = 0; x < n_; ++x) {
                    if (levels_[a][x][i] == kInf) {
                        continue;
                    }
                    for (int y = 0; y < n_; ++y) {
                        if (levels_[b][j][y] != kInf) {
                            levels_[k][x][y] =
                                std::min(levels_[k][x][y], levels_[a][x][i] + r + levels_[b][j][y]);
                        }
                    }
                }
            }
        }
    }

private:
    void rebuild() {
        levels_.assign(h_ + 1, Matrix(n_, std::vector<long long>(n_, kInf)));
        for (int i = 0; i < n_; ++i) {
            levels_[0][i][i] = 0;
        }
        for (int k = 1; k <= h_; ++k) {
            levels_[k] = bounded_hop_detail::min_plus(levels_[k - 1], w_);
            for (int i = 0; i < n_; ++i) {
                levels_[k][i][i] = 0;
            }
        }
    }

    Matrix w_;
    int h_;
    int n_;
    std::vector<Matrix> levels_;
};
