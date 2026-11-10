#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename Key>
class IndexedMinHeap {
public:
    explicit IndexedMinHeap(int capacity) {
        if (capacity < 0) {
            throw std::invalid_argument("negative capacity");
        }
        position_.assign(static_cast<std::size_t>(capacity), kAbsent);
    }

    bool empty() const { return items_.empty(); }
    std::size_t size() const { return items_.size(); }

    bool contains(int id) const {
        check(id);
        return position_[static_cast<std::size_t>(id)] != kAbsent;
    }

    const Key& key(int id) const {
        if (!contains(id)) {
            throw std::out_of_range("id is not in the heap");
        }
        return items_[static_cast<std::size_t>(position_[static_cast<std::size_t>(id)])].key;
    }

    void push(int id, Key key) {
        if (contains(id)) {
            throw std::invalid_argument("id is already in the heap");
        }
        items_.push_back(Item{std::move(key), id});
        position_[static_cast<std::size_t>(id)] = static_cast<int>(items_.size()) - 1;
        sift_up(static_cast<int>(items_.size()) - 1);
    }

    std::pair<int, Key> pop_min() {
        if (items_.empty()) {
            throw std::out_of_range("heap is empty");
        }
        Item top = std::move(items_.front());
        position_[static_cast<std::size_t>(top.id)] = kAbsent;
        Item last = std::move(items_.back());
        items_.pop_back();
        if (!items_.empty()) {
            items_.front() = std::move(last);
            position_[static_cast<std::size_t>(items_.front().id)] = 0;
            sift_down(0);
        }
        return {top.id, std::move(top.key)};
    }

    bool decrease_key(int id, Key key) {
        if (!contains(id)) {
            throw std::out_of_range("id is not in the heap");
        }
        const int at = position_[static_cast<std::size_t>(id)];
        if (!(key < items_[static_cast<std::size_t>(at)].key)) {
            return false;
        }
        items_[static_cast<std::size_t>(at)].key = std::move(key);
        sift_up(at);
        return true;
    }

    void clear() {
        for (const Item& item : items_) {
            position_[static_cast<std::size_t>(item.id)] = kAbsent;
        }
        items_.clear();
    }

private:
    struct Item {
        Key key;
        int id;
    };

    static constexpr int kAbsent = -1;

    void check(int id) const {
        if (id < 0 || static_cast<std::size_t>(id) >= position_.size()) {
            throw std::out_of_range("no such id");
        }
    }

    void place(int at, Item item) {
        position_[static_cast<std::size_t>(item.id)] = at;
        items_[static_cast<std::size_t>(at)] = std::move(item);
    }

    void sift_up(int at) {
        Item item = std::move(items_[static_cast<std::size_t>(at)]);
        while (at > 0) {
            const int up = (at - 1) / 2;
            if (!(item.key < items_[static_cast<std::size_t>(up)].key)) {
                break;
            }
            place(at, std::move(items_[static_cast<std::size_t>(up)]));
            at = up;
        }
        place(at, std::move(item));
    }

    void sift_down(int at) {
        const int count = static_cast<int>(items_.size());
        Item item = std::move(items_[static_cast<std::size_t>(at)]);
        while (2 * at + 1 < count) {
            int child = 2 * at + 1;
            if (child + 1 < count &&
                items_[static_cast<std::size_t>(child + 1)].key < items_[static_cast<std::size_t>(child)].key) {
                ++child;
            }
            if (!(items_[static_cast<std::size_t>(child)].key < item.key)) {
                break;
            }
            place(at, std::move(items_[static_cast<std::size_t>(child)]));
            at = child;
        }
        place(at, std::move(item));
    }

    std::vector<Item> items_;
    std::vector<int> position_;
};

struct Arc {
    int to;
    std::int64_t weight;
};

using WeightedGraph = std::vector<std::vector<Arc>>;

inline constexpr std::int64_t kUnreachable = std::numeric_limits<std::int64_t>::max();
inline constexpr int kNoParent = -1;

struct ShortestPaths {
    std::vector<std::int64_t> dist;
    std::vector<int> parent;
};

namespace detail {

constexpr std::size_t idx(int v) { return static_cast<std::size_t>(v); }

inline void reject_negative_weights(const WeightedGraph& g) {
    for (const auto& arcs : g) {
        for (const Arc& arc : arcs) {
            if (arc.weight < 0) {
                throw std::invalid_argument("negative edge weight");
            }
        }
    }
}

}  // namespace detail

inline ShortestPaths dijkstra(const WeightedGraph& g, int source) {
    if (source < 0 || detail::idx(source) >= g.size()) {
        throw std::out_of_range("no such vertex");
    }
    detail::reject_negative_weights(g);
    ShortestPaths paths{std::vector<std::int64_t>(g.size(), kUnreachable),
                        std::vector<int>(g.size(), kNoParent)};
    IndexedMinHeap<std::int64_t> frontier(static_cast<int>(g.size()));
    paths.dist[detail::idx(source)] = 0;
    frontier.push(source, 0);
    while (!frontier.empty()) {
        const auto [u, d] = frontier.pop_min();
        for (const Arc& arc : g[detail::idx(u)]) {
            const std::int64_t candidate = d + arc.weight;
            if (candidate < paths.dist[detail::idx(arc.to)]) {
                const bool discovered = paths.dist[detail::idx(arc.to)] != kUnreachable;
                paths.dist[detail::idx(arc.to)] = candidate;
                paths.parent[detail::idx(arc.to)] = u;
                if (discovered) {
                    frontier.decrease_key(arc.to, candidate);
                } else {
                    frontier.push(arc.to, candidate);
                }
            }
        }
    }
    return paths;
}

class RouteEngine {
public:
    explicit RouteEngine(WeightedGraph graph)
        : graph_(std::move(graph)),
          frontier_(static_cast<int>(graph_.size())),
          dist_(graph_.size()),
          parent_(graph_.size()),
          stamp_(graph_.size(), 0) {
        detail::reject_negative_weights(graph_);
    }

    std::optional<std::int64_t> distance(int from, int to) {
        if (!search(from, to)) {
            return std::nullopt;
        }
        return dist_[detail::idx(to)];
    }

    std::optional<std::vector<int>> route(int from, int to) {
        if (!search(from, to)) {
            return std::nullopt;
        }
        std::vector<int> path;
        for (int v = to; v != kNoParent; v = parent_[detail::idx(v)]) {
            path.push_back(v);
        }
        std::reverse(path.begin(), path.end());
        return path;
    }

private:
    void check(int v) const {
        if (v < 0 || detail::idx(v) >= graph_.size()) {
            throw std::out_of_range("no such vertex");
        }
    }

    // dist_ and parent_ are only trusted for vertices stamped with the current query's epoch,
    // which is what lets a new query start without touching the whole graph.
    bool search(int from, int to) {
        check(from);
        check(to);
        if (++epoch_ == 0) {
            std::fill(stamp_.begin(), stamp_.end(), 0);
            epoch_ = 1;
        }
        frontier_.clear();
        visit(from, 0, kNoParent);
        frontier_.push(from, 0);
        while (!frontier_.empty()) {
            const auto [u, d] = frontier_.pop_min();
            if (u == to) {
                return true;
            }
            for (const Arc& arc : graph_[detail::idx(u)]) {
                const std::int64_t candidate = d + arc.weight;
                const bool seen = stamp_[detail::idx(arc.to)] == epoch_;
                if (!seen) {
                    visit(arc.to, candidate, u);
                    frontier_.push(arc.to, candidate);
                } else if (candidate < dist_[detail::idx(arc.to)]) {
                    visit(arc.to, candidate, u);
                    frontier_.decrease_key(arc.to, candidate);
                }
            }
        }
        return false;
    }

    void visit(int v, std::int64_t distance, int parent) {
        stamp_[detail::idx(v)] = epoch_;
        dist_[detail::idx(v)] = distance;
        parent_[detail::idx(v)] = parent;
    }

    WeightedGraph graph_;
    IndexedMinHeap<std::int64_t> frontier_;
    std::vector<std::int64_t> dist_;
    std::vector<int> parent_;
    std::vector<std::uint32_t> stamp_;
    std::uint32_t epoch_ = 0;
};
