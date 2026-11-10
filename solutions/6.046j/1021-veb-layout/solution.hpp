#pragma once

#include <cstddef>
#include <optional>
#include <queue>
#include <span>
#include <vector>

struct VebNode {
    long long key = 0;
    int left = -1;
    int right = -1;
};

namespace veb_detail {

struct Shape {
    std::vector<int> left, right;

    explicit Shape(int n) : left(n, -1), right(n, -1) {}

    int build(int lo, int hi) {
        if (lo >= hi) {
            return -1;
        }
        const int mid = lo + (hi - lo) / 2;
        left[mid] = build(lo, mid);
        right[mid] = build(mid + 1, hi);
        return mid;
    }

    int height(int v) const {
        return v < 0 ? 0 : 1 + std::max(height(left[v]), height(right[v]));
    }

    void at_depth(int v, int depth, std::vector<int>& out) const {
        if (v < 0) {
            return;
        }
        if (depth == 0) {
            out.push_back(v);
            return;
        }
        at_depth(left[v], depth - 1, out);
        at_depth(right[v], depth - 1, out);
    }

    void layout(int v, int h, std::vector<int>& order) const {
        if (v < 0) {
            return;
        }
        if (h == 1) {
            order.push_back(v);
            return;
        }
        const int top = h / 2;
        layout(v, top, order);
        std::vector<int> frontier;
        at_depth(v, top - 1, frontier);
        for (int x : frontier) {
            layout(left[x], h - top, order);
            layout(right[x], h - top, order);
        }
    }
};

inline std::vector<VebNode> materialize(std::span<const long long> keys, const Shape& shape,
                                        const std::vector<int>& order) {
    std::vector<int> position(keys.size(), -1);
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
        position[order[i]] = i;
    }
    std::vector<VebNode> out(keys.size());
    for (int i = 0; i < static_cast<int>(order.size()); ++i) {
        const int v = order[i];
        out[i].key = keys[v];
        out[i].left = shape.left[v] == -1 ? -1 : position[shape.left[v]];
        out[i].right = shape.right[v] == -1 ? -1 : position[shape.right[v]];
    }
    return out;
}

}  // namespace veb_detail

inline std::vector<VebNode> build_veb_layout(std::span<const long long> sorted_keys) {
    const int n = static_cast<int>(sorted_keys.size());
    if (n == 0) {
        return {};
    }
    veb_detail::Shape shape(n);
    const int root = shape.build(0, n);
    std::vector<int> order;
    order.reserve(sorted_keys.size());
    shape.layout(root, shape.height(root), order);
    return veb_detail::materialize(sorted_keys, shape, order);
}

inline int search_blocks(std::span<const VebNode> nodes, long long key, std::size_t block_bytes) {
    if (nodes.empty()) {
        return 0;
    }
    std::vector<std::size_t> blocks;
    int v = 0;
    while (v != -1) {
        const auto first = static_cast<std::size_t>(v) * sizeof(VebNode) / block_bytes;
        const auto last = (static_cast<std::size_t>(v) * sizeof(VebNode) + sizeof(VebNode) - 1) /
                          block_bytes;
        for (std::size_t b = first; b <= last; ++b) {
            blocks.push_back(b);
        }
        if (nodes[v].key == key) {
            break;
        }
        v = key < nodes[v].key ? nodes[v].left : nodes[v].right;
    }
    std::sort(blocks.begin(), blocks.end());
    blocks.erase(std::unique(blocks.begin(), blocks.end()), blocks.end());
    return static_cast<int>(blocks.size());
}

inline std::vector<VebNode> build_bfs_layout(std::span<const long long> sorted_keys) {
    const int n = static_cast<int>(sorted_keys.size());
    if (n == 0) {
        return {};
    }
    veb_detail::Shape shape(n);
    const int root = shape.build(0, n);
    std::vector<int> order;
    order.reserve(sorted_keys.size());
    std::queue<int> q;
    q.push(root);
    while (!q.empty()) {
        const int v = q.front();
        q.pop();
        order.push_back(v);
        if (shape.left[v] != -1) {
            q.push(shape.left[v]);
        }
        if (shape.right[v] != -1) {
            q.push(shape.right[v]);
        }
    }
    return veb_detail::materialize(sorted_keys, shape, order);
}

class VebIndex {
public:
    explicit VebIndex(std::vector<long long> sorted_keys) : nodes_(build_veb_layout(sorted_keys)) {}

    bool contains(long long key) const {
        int v = nodes_.empty() ? -1 : 0;
        while (v != -1) {
            if (nodes_[v].key == key) {
                return true;
            }
            v = key < nodes_[v].key ? nodes_[v].left : nodes_[v].right;
        }
        return false;
    }

    std::optional<long long> predecessor(long long key) const {
        std::optional<long long> best;
        int v = nodes_.empty() ? -1 : 0;
        while (v != -1) {
            if (nodes_[v].key <= key) {
                best = nodes_[v].key;
                v = nodes_[v].right;
            } else {
                v = nodes_[v].left;
            }
        }
        return best;
    }

private:
    std::vector<VebNode> nodes_;
};
