#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "../0403-avl-map/solution.hpp"

// Monoid over the items of a subtree in key order: the total, and the best non-empty prefix.
struct MaxPrefixAug {
    struct Value {
        std::int64_t sum = 0;
        std::int64_t best = std::numeric_limits<std::int64_t>::min();  // "no prefix yet"
        std::int64_t best_key = 0;
    };

    static Value identity() { return {}; }

    template <typename Item>
    static Value of(const Item& item) {
        return {item.value, item.value, item.key};
    }

    static Value combine(const Value& a, const Value& b) {
        Value out{a.sum + b.sum, a.best, a.best_key};
        if (b.best != std::numeric_limits<std::int64_t>::min()) {
            const std::int64_t extended = a.sum + b.best;
            if (out.best == std::numeric_limits<std::int64_t>::min() || extended > out.best) {
                out.best = extended;
                out.best_key = b.best_key;
            }
        }
        return out;
    }
};

class PrefixTree {
public:
    bool insert(std::int64_t key, std::int64_t value) { return map_.insert(key, value); }
    bool erase(std::int64_t key) { return map_.erase(key); }
    std::size_t size() const { return map_.size(); }

    std::optional<std::int64_t> value_of(std::int64_t key) const {
        const auto* node = map_.find(key);
        return node == nullptr ? std::nullopt : std::optional<std::int64_t>(node->item.value);
    }

    void add(std::int64_t key, std::int64_t delta) {
        if (auto* node = map_.find(key)) {
            map_.set_value(node, node->item.value + delta);
        } else {
            map_.insert(key, delta);
        }
    }

    std::optional<std::pair<std::int64_t, std::int64_t>> max_prefix() const {
        const auto* root = map_.tree().root();
        if (root == nullptr) {
            return std::nullopt;
        }
        return std::pair{root->aug.best_key, root->aug.best};
    }

private:
    AvlMap<std::int64_t, std::int64_t, MaxPrefixAug> map_;
};

struct Topping {
    std::int64_t x;
    std::int64_t y;
    std::int64_t t;
};

struct Slice {
    std::int64_t x;
    std::int64_t y;
    std::int64_t t;
    friend bool operator==(const Slice&, const Slice&) = default;
};

// Sweep by x; after absorbing every point with x <= X, the best slice with right edge X is
// exactly the tree's best prefix over y.
inline std::optional<Slice> tastiest_slice(std::span<const Topping> toppings) {
    std::vector<Topping> sorted(toppings.begin(), toppings.end());
    std::sort(sorted.begin(), sorted.end(), [](const Topping& a, const Topping& b) { return a.x < b.x; });

    PrefixTree tree;
    std::optional<Slice> best;
    for (std::size_t i = 0; i < sorted.size();) {
        const std::int64_t x = sorted[i].x;
        for (; i < sorted.size() && sorted[i].x == x; ++i) {
            tree.add(sorted[i].y, sorted[i].t);
        }
        const auto [y, t] = *tree.max_prefix();
        if (t > 0 && (!best.has_value() || t > best->t)) {
            best = Slice{x, y, t};
        }
    }
    return best;
}
