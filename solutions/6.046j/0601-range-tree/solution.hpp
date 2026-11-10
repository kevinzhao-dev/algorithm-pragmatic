#pragma once

#include <algorithm>
#include <cstddef>
#include <numeric>
#include <utility>
#include <vector>

template <typename T = int>
class RangeTree1D {
public:
    explicit RangeTree1D(std::vector<T> keys) : order_(keys.size()) {
        std::iota(order_.begin(), order_.end(), std::size_t{0});
        std::stable_sort(order_.begin(), order_.end(),
                         [&](std::size_t a, std::size_t b) { return keys[a] < keys[b]; });
        sorted_.reserve(keys.size());
        for (std::size_t index : order_) {
            sorted_.push_back(std::move(keys[index]));
        }
    }

    std::size_t count(const T& a, const T& b) const {
        const auto [lo, hi] = bounds(a, b);
        return hi - lo;
    }

    std::vector<std::size_t> report(const T& a, const T& b) const {
        const auto [lo, hi] = bounds(a, b);
        return {order_.begin() + static_cast<std::ptrdiff_t>(lo),
                order_.begin() + static_cast<std::ptrdiff_t>(hi)};
    }

    std::vector<std::pair<std::size_t, std::size_t>> canonical(const T& a, const T& b) const {
        std::vector<std::pair<std::size_t, std::size_t>> out;
        const auto [lo, hi] = bounds(a, b);
        if (lo < hi) {
            decompose(0, sorted_.size(), lo, hi, out);
        }
        return out;
    }

private:
    std::pair<std::size_t, std::size_t> bounds(const T& a, const T& b) const {
        if (b < a) {
            return {0, 0};
        }
        const auto lo = std::lower_bound(sorted_.begin(), sorted_.end(), a);
        const auto hi = std::upper_bound(lo, sorted_.end(), b);
        return {static_cast<std::size_t>(lo - sorted_.begin()),
                static_cast<std::size_t>(hi - sorted_.begin())};
    }

    static void decompose(std::size_t nl, std::size_t nr, std::size_t ql, std::size_t qr,
                          std::vector<std::pair<std::size_t, std::size_t>>& out) {
        if (qr <= nl || nr <= ql) {
            return;
        }
        if (ql <= nl && nr <= qr) {
            out.emplace_back(nl, nr);
            return;
        }
        const std::size_t mid = nl + (nr - nl) / 2;
        decompose(nl, mid, ql, qr, out);
        decompose(mid, nr, ql, qr, out);
    }

    std::vector<T> sorted_;
    std::vector<std::size_t> order_;
};

template <typename T = int>
struct Point {
    T x{};
    T y{};
};

template <typename T, typename V>
struct WeightedPoint {
    T x{};
    T y{};
    V weight{};
};

namespace range_tree_detail {

// Primary balanced tree over points sorted by x (implicit, node 1 covers [0, n)); every node
// keeps its points sorted by y, plus prefix sums when V is non-void.
template <typename T, typename V>
class Core {
public:
    struct Slot {
        T y;
        std::size_t id;
    };

    template <typename Pt, typename WeightOf>
    Core(const std::vector<Pt>& pts, WeightOf weight_of) : n_(pts.size()) {
        std::vector<std::size_t> by_x(n_);
        std::iota(by_x.begin(), by_x.end(), std::size_t{0});
        std::stable_sort(by_x.begin(), by_x.end(),
                         [&](std::size_t a, std::size_t b) { return pts[a].x < pts[b].x; });
        xs_.reserve(n_);
        for (std::size_t index : by_x) {
            xs_.push_back(pts[index].x);
        }
        nodes_.resize(n_ == 0 ? 1 : 4 * n_);
        sums_.resize(nodes_.size());
        if (n_ > 0) {
            build(1, 0, n_, by_x, pts, weight_of);
        }
    }

    std::pair<std::size_t, std::size_t> x_bounds(const T& x1, const T& x2) const {
        if (x2 < x1) {
            return {0, 0};
        }
        const auto lo = std::lower_bound(xs_.begin(), xs_.end(), x1);
        const auto hi = std::upper_bound(lo, xs_.end(), x2);
        return {static_cast<std::size_t>(lo - xs_.begin()),
                static_cast<std::size_t>(hi - xs_.begin())};
    }

    std::size_t size() const { return n_; }

    // Calls visit(node, lo, hi): [lo, hi) are the slots of `node` whose y lies in [y1, y2].
    template <typename Visit>
    void for_each_rect(std::size_t ql, std::size_t qr, const T& y1, const T& y2,
                       Visit&& visit) const {
        if (ql < qr && !(y2 < y1)) {
            descend(1, 0, n_, ql, qr, y1, y2, visit);
        }
    }

    const std::vector<Slot>& slots(std::size_t node) const { return nodes_[node]; }
    const std::vector<V>& prefix(std::size_t node) const { return sums_[node]; }

private:
    template <typename Pt, typename WeightOf>
    void build(std::size_t node, std::size_t l, std::size_t r, const std::vector<std::size_t>& by_x,
               const std::vector<Pt>& pts, WeightOf weight_of) {
        if (r - l == 1) {
            nodes_[node].push_back({pts[by_x[l]].y, by_x[l]});
        } else {
            const std::size_t mid = l + (r - l) / 2;
            build(2 * node, l, mid, by_x, pts, weight_of);
            build(2 * node + 1, mid, r, by_x, pts, weight_of);
            std::merge(nodes_[2 * node].begin(), nodes_[2 * node].end(),
                       nodes_[2 * node + 1].begin(), nodes_[2 * node + 1].end(),
                       std::back_inserter(nodes_[node]),
                       [](const Slot& a, const Slot& b) { return a.y < b.y; });
        }
        auto& sums = sums_[node];
        sums.assign(nodes_[node].size() + 1, V{});
        for (std::size_t i = 0; i < nodes_[node].size(); ++i) {
            sums[i + 1] = sums[i] + weight_of(pts[nodes_[node][i].id]);
        }
    }

    template <typename Visit>
    void descend(std::size_t node, std::size_t l, std::size_t r, std::size_t ql, std::size_t qr,
                 const T& y1, const T& y2, Visit&& visit) const {
        if (qr <= l || r <= ql) {
            return;
        }
        if (ql <= l && r <= qr) {
            const auto& list = nodes_[node];
            const auto lo = std::partition_point(list.begin(), list.end(),
                                                 [&](const Slot& s) { return s.y < y1; });
            const auto hi = std::partition_point(lo, list.end(),
                                                 [&](const Slot& s) { return !(y2 < s.y); });
            visit(node, static_cast<std::size_t>(lo - list.begin()),
                  static_cast<std::size_t>(hi - list.begin()));
            return;
        }
        const std::size_t mid = l + (r - l) / 2;
        descend(2 * node, l, mid, ql, qr, y1, y2, visit);
        descend(2 * node + 1, mid, r, ql, qr, y1, y2, visit);
    }

    std::size_t n_;
    std::vector<T> xs_;
    std::vector<std::vector<Slot>> nodes_;
    std::vector<std::vector<V>> sums_;
};

}  // namespace range_tree_detail

template <typename T = int>
class RangeTree2D {
public:
    explicit RangeTree2D(std::vector<Point<T>> pts)
        : core_(pts, [](const Point<T>&) { return 1; }) {}

    std::size_t count(const T& x1, const T& x2, const T& y1, const T& y2) const {
        const auto [lo, hi] = core_.x_bounds(x1, x2);
        std::size_t total = 0;
        core_.for_each_rect(lo, hi, y1, y2, [&](std::size_t, std::size_t a, std::size_t b) {
            total += b - a;
        });
        return total;
    }

    std::vector<std::size_t> report(const T& x1, const T& x2, const T& y1, const T& y2) const {
        const auto [lo, hi] = core_.x_bounds(x1, x2);
        std::vector<std::size_t> out;
        core_.for_each_rect(lo, hi, y1, y2, [&](std::size_t node, std::size_t a, std::size_t b) {
            const auto& slots = core_.slots(node);
            for (std::size_t i = a; i < b; ++i) {
                out.push_back(slots[i].id);
            }
        });
        return out;
    }

private:
    range_tree_detail::Core<T, int> core_;
};

template <typename T, typename V>
class WeightedRangeTree {
public:
    explicit WeightedRangeTree(std::vector<WeightedPoint<T, V>> pts)
        : core_(pts, [](const WeightedPoint<T, V>& p) { return p.weight; }) {}

    V sum(const T& x1, const T& x2, const T& y1, const T& y2) const {
        const auto [lo, hi] = core_.x_bounds(x1, x2);
        V total{};
        core_.for_each_rect(lo, hi, y1, y2, [&](std::size_t node, std::size_t a, std::size_t b) {
            const auto& prefix = core_.prefix(node);
            total = total + (prefix[b] - prefix[a]);
        });
        return total;
    }

    std::size_t count(const T& x1, const T& x2, const T& y1, const T& y2) const {
        const auto [lo, hi] = core_.x_bounds(x1, x2);
        std::size_t total = 0;
        core_.for_each_rect(lo, hi, y1, y2, [&](std::size_t, std::size_t a, std::size_t b) {
            total += b - a;
        });
        return total;
    }

private:
    range_tree_detail::Core<T, V> core_;
};
