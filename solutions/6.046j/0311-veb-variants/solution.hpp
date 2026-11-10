#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

struct SqrtSplit {
    static constexpr int high_bits(int bits) { return bits - bits / 2; }
};

struct CubeRootSplit {
    static constexpr int high_bits(int bits) { return std::max(1, bits / 3); }
};

template <typename Split, bool ExcludeMax>
class Veb {
public:
    explicit Veb(int universe_bits) : root_(universe_bits) {
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

        int high_bits() const { return Split::high_bits(bits); }
        int low_bits() const { return bits - high_bits(); }
        std::uint64_t high(std::uint64_t x) const { return x >> low_bits(); }
        std::uint64_t low(std::uint64_t x) const {
            return x & ((std::uint64_t{1} << low_bits()) - 1);
        }
        std::uint64_t index(std::uint64_t h, std::uint64_t l) const { return (h << low_bits()) | l; }
    };

    void check_range(std::uint64_t x) const {
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

    bool insert_into_cluster(Node& v, std::uint64_t x) {
        auto& cluster = v.clusters[v.high(x)];
        if (cluster == nullptr) {
            cluster = std::make_unique<Node>(v.low_bits());
            if (v.summary == nullptr) {
                v.summary = std::make_unique<Node>(v.high_bits());
            }
            insert(*v.summary, v.high(x));
        }
        return insert(*cluster, v.low(x));
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
        if constexpr (ExcludeMax) {
            if (v.min == v.max) {
                (x < v.min ? v.min : v.max) = x;
                return true;
            }
            if (x < v.min) {
                std::swap(x, v.min);
            } else if (x > v.max) {
                std::swap(x, v.max);
            }
            return insert_into_cluster(v, x);
        } else {
            if (x < v.min) {
                std::swap(x, v.min);
            }
            bool inserted = true;
            if (v.bits > 1) {
                inserted = insert_into_cluster(v, x);
            }
            if (inserted && x > v.max) {
                v.max = x;
            }
            return inserted;
        }
    }

    bool erase_from_cluster(Node& v, std::uint64_t x) {
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
        return true;
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
        if constexpr (ExcludeMax) {
            if (v.summary == nullptr) {
                if (x != v.min && x != v.max) {
                    return false;
                }
                v.min = v.max = x == v.min ? v.max : v.min;
                return true;
            }
            if (x == v.min) {
                const std::uint64_t first = v.summary->min;
                x = v.index(first, v.clusters.at(first)->min);
                v.min = x;
            } else if (x == v.max) {
                const std::uint64_t last = v.summary->max;
                x = v.index(last, v.clusters.at(last)->max);
                v.max = x;
            }
            return erase_from_cluster(v, x);
        } else {
            if (v.bits == 1) {
                v.min = v.max = x == 0 ? 1 : 0;
                return true;
            }
            if (x == v.min) {
                const std::uint64_t first = v.summary->min;
                x = v.index(first, v.clusters.at(first)->min);
                v.min = x;
            }
            if (!erase_from_cluster(v, x)) {
                return false;
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
    }

    std::optional<std::uint64_t> successor(const Node& v, std::uint64_t x) const {
        ++visits_;
        if (!v.occupied || x >= v.max) {
            return std::nullopt;
        }
        if (x < v.min) {
            return v.min;
        }
        if constexpr (ExcludeMax) {
            if (v.summary == nullptr) {
                return v.max;
            }
        } else if (v.bits == 1) {
            return v.max;
        }
        const std::uint64_t h = v.high(x);
        const auto cluster = v.clusters.find(h);
        if (cluster != v.clusters.end() && v.low(x) < cluster->second->max) {
            return v.index(h, *successor(*cluster->second, v.low(x)));
        }
        if (const auto next = successor(*v.summary, h)) {
            return v.index(*next, v.clusters.at(*next)->min);
        }
        return v.max;
    }

    std::optional<std::uint64_t> predecessor(const Node& v, std::uint64_t x) const {
        ++visits_;
        if (!v.occupied || x <= v.min) {
            return std::nullopt;
        }
        if (x > v.max) {
            return v.max;
        }
        if (v.summary == nullptr) {
            return v.min;
        }
        const std::uint64_t h = v.high(x);
        const auto cluster = v.clusters.find(h);
        if (cluster != v.clusters.end() && v.low(x) > cluster->second->min) {
            return v.index(h, *predecessor(*cluster->second, v.low(x)));
        }
        if (const auto previous = predecessor(*v.summary, h)) {
            return v.index(*previous, v.clusters.at(*previous)->max);
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
