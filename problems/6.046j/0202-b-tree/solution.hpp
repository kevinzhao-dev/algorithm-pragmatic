#pragma once

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

#include "practice.hpp"

template <typename Key>
class BTree {
public:
    struct Node {
        std::vector<Key> keys;
        std::vector<std::unique_ptr<Node>> children;

        bool is_leaf() const { return children.empty(); }
    };

    // Stage 1
    explicit BTree(std::size_t min_degree) { PRACTICE_TODO(); }

    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    // Once implemented, these should be noexcept.
    BTree(BTree&& other) { PRACTICE_TODO(); }
    BTree& operator=(BTree&& other) { PRACTICE_TODO(); }

    std::size_t min_degree() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    const Node* root() const { PRACTICE_TODO(); }

    bool contains(const Key& key) const { PRACTICE_TODO(); }
    bool insert(Key key) { PRACTICE_TODO(); }

    // Stage 2
    bool erase(const Key& key) { PRACTICE_TODO(); }

    // Stage 3
    std::optional<Key> lower_bound(const Key& key) const { PRACTICE_TODO(); }

    template <typename Visit>
    void scan(const Key& low, const Key& high, Visit visit) const {
        PRACTICE_TODO();
    }
};
