#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include "../0402-avl-tree/solution.hpp"
#include "practice.hpp"

template <typename K, typename V, typename Aug = NoAugment, typename Compare = std::less<K>>
class AvlMap {
public:
    struct Item {
        K key;
        V value;
    };
    using Tree = AvlTree<Item, Aug>;
    using Node = typename Tree::Node;

    // Stage 1
    explicit AvlMap(Compare compare = Compare{}) { PRACTICE_TODO(); }

    Node* find(const K& key) const { PRACTICE_TODO(); }
    bool contains(const K& key) const { PRACTICE_TODO(); }
    bool insert(K key, V value = V{}) { PRACTICE_TODO(); }
    bool erase(const K& key) { PRACTICE_TODO(); }
    void set_value(Node* node, V value) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    const Tree& tree() const { PRACTICE_TODO(); }
    Node* min() const { PRACTICE_TODO(); }
    Node* max() const { PRACTICE_TODO(); }

    // Stage 2
    Node* find_next(const K& key) const { PRACTICE_TODO(); }
    Node* find_prev(const K& key) const { PRACTICE_TODO(); }

    // Stage 3
    std::size_t rank(const K& key) const { PRACTICE_TODO(); }
    Node* select(std::size_t index) const { PRACTICE_TODO(); }
};

template <typename K, typename Compare = std::less<K>>
using AvlSet = AvlMap<K, std::monostate, NoAugment, Compare>;

// Stage 4
class Leaderboard {
public:
    void upsert(const std::string& player, std::int64_t score) { PRACTICE_TODO(); }
    bool remove(const std::string& player) { PRACTICE_TODO(); }
    std::optional<std::size_t> rank_of(const std::string& player) const { PRACTICE_TODO(); }
    std::optional<std::string> player_at(std::size_t rank) const { PRACTICE_TODO(); }
    std::vector<std::pair<std::string, std::int64_t>> top(std::size_t count) const {
        PRACTICE_TODO();
    }
    std::size_t size() const { PRACTICE_TODO(); }
};
