#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

#include "../0402-avl-tree/solution.hpp"

template <typename K, typename V, typename Aug = NoAugment, typename Compare = std::less<K>>
class AvlMap {
public:
    struct Item {
        K key;
        V value;
    };
    using Tree = AvlTree<Item, Aug>;
    using Node = typename Tree::Node;

    explicit AvlMap(Compare compare = Compare{}) : compare_(std::move(compare)) {}

    Node* find(const K& key) const {
        Node* node = tree_.root();
        while (node != nullptr) {
            if (compare_(key, node->item.key)) {
                node = node->left;
            } else if (compare_(node->item.key, key)) {
                node = node->right;
            } else {
                return node;
            }
        }
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    bool insert(K key, V value = V{}) {
        Node* node = tree_.root();
        Node* parent = nullptr;
        bool go_left = false;
        while (node != nullptr) {
            parent = node;
            if (compare_(key, node->item.key)) {
                go_left = true;
                node = node->left;
            } else if (compare_(node->item.key, key)) {
                go_left = false;
                node = node->right;
            } else {
                set_value(node, std::move(value));
                return false;
            }
        }
        Item item{std::move(key), std::move(value)};
        if (parent == nullptr || !go_left) {
            tree_.insert_after(parent, std::move(item));
        } else {
            tree_.insert_before(parent, std::move(item));
        }
        return true;
    }

    bool erase(const K& key) {
        Node* node = find(key);
        if (node == nullptr) {
            return false;
        }
        tree_.erase(node);
        return true;
    }

    void set_value(Node* node, V value) {
        node->item.value = std::move(value);
        tree_.refresh(node);
    }

    std::size_t size() const { return tree_.size(); }
    bool empty() const { return tree_.empty(); }
    const Tree& tree() const { return tree_; }

    Node* min() const { return Tree::subtree_first(tree_.root()); }
    Node* max() const { return Tree::subtree_last(tree_.root()); }

    Node* find_next(const K& key) const {
        Node* node = tree_.root();
        Node* best = nullptr;
        while (node != nullptr) {
            if (compare_(key, node->item.key)) {
                best = node;
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return best;
    }

    Node* find_prev(const K& key) const {
        Node* node = tree_.root();
        Node* best = nullptr;
        while (node != nullptr) {
            if (compare_(node->item.key, key)) {
                best = node;
                node = node->right;
            } else {
                node = node->left;
            }
        }
        return best;
    }

    std::size_t rank(const K& key) const {
        std::size_t smaller = 0;
        Node* node = tree_.root();
        while (node != nullptr) {
            if (compare_(key, node->item.key)) {
                node = node->left;
            } else if (compare_(node->item.key, key)) {
                smaller += subtree_size(node->left) + 1;
                node = node->right;
            } else {
                return smaller + subtree_size(node->left);
            }
        }
        return smaller;
    }

    Node* select(std::size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("select index out of range");
        }
        Node* node = tree_.root();
        while (true) {
            const std::size_t left = subtree_size(node->left);
            if (index < left) {
                node = node->left;
            } else if (index == left) {
                return node;
            } else {
                index -= left + 1;
                node = node->right;
            }
        }
    }

private:
    static std::size_t subtree_size(const Node* node) { return node != nullptr ? node->size : 0; }

    Tree tree_;
    Compare compare_;
};

template <typename K, typename Compare = std::less<K>>
using AvlSet = AvlMap<K, std::monostate, NoAugment, Compare>;

class Leaderboard {
public:
    void upsert(const std::string& player, std::int64_t score) {
        const auto found = scores_.find(player);
        if (found != scores_.end()) {
            board_.erase(Entry{found->second, player});
            found->second = score;
        } else {
            scores_.emplace(player, score);
        }
        board_.insert(Entry{score, player});
    }

    bool remove(const std::string& player) {
        const auto found = scores_.find(player);
        if (found == scores_.end()) {
            return false;
        }
        board_.erase(Entry{found->second, player});
        scores_.erase(found);
        return true;
    }

    std::optional<std::size_t> rank_of(const std::string& player) const {
        const auto found = scores_.find(player);
        if (found == scores_.end()) {
            return std::nullopt;
        }
        return board_.rank(Entry{found->second, player});
    }

    std::optional<std::string> player_at(std::size_t rank) const {
        if (rank >= board_.size()) {
            return std::nullopt;
        }
        return board_.select(rank)->item.key.player;
    }

    std::vector<std::pair<std::string, std::int64_t>> top(std::size_t count) const {
        std::vector<std::pair<std::string, std::int64_t>> out;
        auto* node = board_.min();
        while (node != nullptr && out.size() < count) {
            out.emplace_back(node->item.key.player, node->item.key.score);
            node = Board::Tree::successor(node);
        }
        return out;
    }

    std::size_t size() const { return board_.size(); }

private:
    struct Entry {
        std::int64_t score;
        std::string player;
    };
    struct HigherScoreFirst {
        bool operator()(const Entry& a, const Entry& b) const {
            return a.score != b.score ? a.score > b.score : a.player < b.player;
        }
    };
    using Board = AvlMap<Entry, std::monostate, NoAugment, HigherScoreFirst>;

    std::unordered_map<std::string, std::int64_t> scores_;
    Board board_;
};
