#pragma once

#include <cstddef>
#include <memory>
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

    // Stage 1
    int height() const { PRACTICE_TODO(); }

    bool contains(const Key& key) const { PRACTICE_TODO(); }
    bool insert(Key key) { PRACTICE_TODO(); }

    bool erase(const Key& key) { PRACTICE_TODO(); }

    // Stages 2 to 4
    static BTree combine(BTree&& left, Key key, BTree&& right) { PRACTICE_TODO(); }

    // Stage 5
    static BTree concat(BTree&& left, BTree&& right) { PRACTICE_TODO(); }
};
