#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

class VanEmdeBoas {
public:
    explicit VanEmdeBoas(int universe_bits) : root_(universe_bits) {
        if (universe_bits < 1 || universe_bits > 64) {
            throw std::invalid_argument("universe_bits must be in [1, 64]");
        }
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }

    std::optional<std::uint64_t> min() const {
        return root_.occupied ? std::optional(root_.min) : std::nullopt;
    }
    std::optional<std::uint64_t> max() const {
        return root_.occupied ? std::optional(root_.max) : std::nullopt;
    }

    bool contains(std::uint64_t x) const {
        check_range(x);
        return contains(root_, x);
    }

    bool insert(std::uint64_t x) {
        check_range(x);
        const bool inserted = insert(root_, x);
        size_ += inserted ? 1 : 0;
        return inserted;
    }

    bool erase(std::uint64_t x) {
        check_range(x);
        const bool erased = erase(root_, x);
        size_ -= erased ? 1 : 0;
        return erased;
    }

    std::optional<std::uint64_t> successor(std::uint64_t x) const {
        check_range(x);
        return successor(root_, x);
    }

    std::optional<std::uint64_t> predecessor(std::uint64_t x) const {
        check_range(x);
        return predecessor(root_, x);
    }

    std::size_t node_count() const { return count_nodes(root_); }
    std::uint64_t visits() const { return visits_; }

private:
    struct Node {
        explicit Node(int b) : bits(b) {}

        int bits;
        bool occupied = false;
        std::uint64_t min = 0;
        std::uint64_t max = 0;
        std::unique_ptr<Node> summary;
        std::unordered_map<std::uint64_t, std::unique_ptr<Node>> clusters;

        int low_bits() const { return bits / 2; }
        int high_bits() const { return bits - bits / 2; }
        std::uint64_t high(std::uint64_t x) const { return x >> low_bits(); }
        std::uint64_t low(std::uint64_t x) const {
            return x & ((std::uint64_t{1} << low_bits()) - 1);
        }
        std::uint64_t index(std::uint64_t h, std::uint64_t l) const { return (h << low_bits()) | l; }
    };

    void check_range(std::uint64_t x) const {
        // A shift by the full width is undefined, so 64 bits needs no bound at all.
        if (root_.bits < 64 && (x >> root_.bits) != 0) {
            throw std::out_of_range("key is outside the universe");
        }
    }

    bool contains(const Node& v, std::uint64_t x) const {
        ++visits_;
        if (!v.occupied) {
            return false;
        }
        if (x == v.min || x == v.max) {
            return true;
        }
        if (v.bits == 1) {
            return false;
        }
        const auto cluster = v.clusters.find(v.high(x));
        return cluster != v.clusters.end() && contains(*cluster->second, v.low(x));
    }

    bool insert(Node& v, std::uint64_t x) {
        ++visits_;
        if (!v.occupied) {
            v.occupied = true;
            v.min = x;
            v.max = x;
            return true;
        }
        if (x == v.min || x == v.max) {
            return false;
        }
        if (x < v.min) {
            std::swap(x, v.min);
        }
        bool inserted = true;
        if (v.bits > 1) {
            auto& cluster = v.clusters[v.high(x)];
            if (cluster == nullptr) {
                cluster = std::make_unique<Node>(v.low_bits());
                if (v.summary == nullptr) {
                    v.summary = std::make_unique<Node>(v.high_bits());
                }
                insert(*v.summary, v.high(x));
            }
            inserted = insert(*cluster, v.low(x));
        }
        if (inserted && x > v.max) {
            v.max = x;
        }
        return inserted;
    }

    bool erase(Node& v, std::uint64_t x) {
        ++visits_;
        if (!v.occupied) {
            return false;
        }
        if (v.min == v.max) {
            if (x != v.min) {
                return false;
            }
            v.occupied = false;
            return true;
        }
        if (v.bits == 1) {
            v.min = v.max = x == 0 ? 1 : 0;
            return true;
        }
        if (x == v.min) {
            // The minimum lives outside the clusters, so promote the next one up.
            const std::uint64_t first = v.summary->min;
            x = v.index(first, v.clusters.at(first)->min);
            v.min = x;
        }
        const auto cluster = v.clusters.find(v.high(x));
        if (cluster == v.clusters.end() || !erase(*cluster->second, v.low(x))) {
            return false;
        }
        if (!cluster->second->occupied) {
            v.clusters.erase(cluster);
            erase(*v.summary, v.high(x));
            if (!v.summary->occupied) {
                v.summary.reset();
            }
        }
        if (x == v.max) {
            if (v.summary == nullptr) {
                v.max = v.min;
            } else {
                const std::uint64_t last = v.summary->max;
                v.max = v.index(last, v.clusters.at(last)->max);
            }
        }
        return true;
    }

    std::optional<std::uint64_t> successor(const Node& v, std::uint64_t x) const {
        ++visits_;
        if (!v.occupied || x >= v.max) {
            return std::nullopt;
        }
        if (x < v.min) {
            return v.min;
        }
        if (v.bits == 1) {
            return v.max;
        }
        const std::uint64_t h = v.high(x);
        const auto cluster = v.clusters.find(h);
        if (cluster != v.clusters.end() && v.low(x) < cluster->second->max) {
            return v.index(h, *successor(*cluster->second, v.low(x)));
        }
        const auto next = successor(*v.summary, h);
        return v.index(*next, v.clusters.at(*next)->min);
    }

    std::optional<std::uint64_t> predecessor(const Node& v, std::uint64_t x) const {
        ++visits_;
        if (!v.occupied || x <= v.min) {
            return std::nullopt;
        }
        if (x > v.max) {
            return v.max;
        }
        if (v.bits == 1) {
            return v.min;
        }
        const std::uint64_t h = v.high(x);
        const auto cluster = v.clusters.find(h);
        if (cluster != v.clusters.end() && v.low(x) > cluster->second->min) {
            return v.index(h, *predecessor(*cluster->second, v.low(x)));
        }
        if (v.summary != nullptr) {
            if (const auto previous = predecessor(*v.summary, h)) {
                return v.index(*previous, v.clusters.at(*previous)->max);
            }
        }
        return v.min;
    }

    static std::size_t count_nodes(const Node& v) {
        std::size_t count = 1;
        if (v.summary != nullptr) {
            count += count_nodes(*v.summary);
        }
        for (const auto& [h, cluster] : v.clusters) {
            count += count_nodes(*cluster);
        }
        return count;
    }

    Node root_;
    std::size_t size_ = 0;
    mutable std::uint64_t visits_ = 0;
};

class RouteTable {
public:
    bool add(std::uint32_t first, std::uint32_t last, int port) {
        if (first > last) {
            throw std::invalid_argument("add: first must not exceed last");
        }
        if (starts_.contains(first)) {
            return false;
        }
        if (const auto next = starts_.successor(first); next && *next <= last) {
            return false;
        }
        if (const auto previous = starts_.predecessor(first); previous) {
            if (routes_.at(static_cast<std::uint32_t>(*previous)).last >= first) {
                return false;
            }
        }
        starts_.insert(first);
        routes_.insert_or_assign(first, Route{last, port});
        return true;
    }

    bool remove(std::uint32_t first) {
        if (!starts_.erase(first)) {
            return false;
        }
        routes_.erase(first);
        return true;
    }

    std::optional<int> lookup(std::uint32_t ip) const {
        std::optional<std::uint64_t> start;
        if (starts_.contains(ip)) {
            start = ip;
        } else {
            start = starts_.predecessor(ip);
        }
        if (!start) {
            return std::nullopt;
        }
        const Route& route = routes_.at(static_cast<std::uint32_t>(*start));
        return route.last >= ip ? std::optional(route.port) : std::nullopt;
    }

    std::size_t size() const { return routes_.size(); }

private:
    struct Route {
        std::uint32_t last;
        int port;
    };

    VanEmdeBoas starts_{32};
    std::unordered_map<std::uint32_t, Route> routes_;
};
