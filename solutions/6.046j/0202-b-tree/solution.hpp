#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

template <typename Key>
class BTree {
public:
    struct Node {
        std::vector<Key> keys;
        std::vector<std::unique_ptr<Node>> children;

        bool is_leaf() const { return children.empty(); }
    };

    explicit BTree(std::size_t min_degree) : t_(min_degree) {
        if (min_degree < 2) {
            throw std::invalid_argument("BTree: minimum degree must be at least 2");
        }
    }

    BTree(const BTree&) = delete;
    BTree& operator=(const BTree&) = delete;

    BTree(BTree&& other) noexcept
        : t_(other.t_), size_(std::exchange(other.size_, 0)), root_(std::move(other.root_)) {}

    BTree& operator=(BTree&& other) noexcept {
        if (this != &other) {
            t_ = other.t_;
            size_ = std::exchange(other.size_, 0);
            root_ = std::move(other.root_);
        }
        return *this;
    }

    std::size_t min_degree() const { return t_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    const Node* root() const { return root_.get(); }

    bool contains(const Key& key) const {
        const Node* node = root_.get();
        while (node != nullptr) {
            const std::size_t i = lower_index(*node, key);
            if (i < node->keys.size() && !(key < node->keys[i])) {
                return true;
            }
            node = node->is_leaf() ? nullptr : node->children[i].get();
        }
        return false;
    }

    bool insert(Key key) {
        if (contains(key)) {
            return false;
        }
        if (!root_) {
            root_ = std::make_unique<Node>();
        } else if (is_full(*root_)) {
            auto new_root = std::make_unique<Node>();
            new_root->children.push_back(std::move(root_));
            root_ = std::move(new_root);
            split_child(*root_, 0);
        }
        Node* node = root_.get();
        while (!node->is_leaf()) {
            std::size_t i = lower_index(*node, key);
            if (is_full(*node->children[i])) {
                split_child(*node, i);
                if (node->keys[i] < key) {
                    ++i;
                }
            }
            node = node->children[i].get();
        }
        const std::size_t i = lower_index(*node, key);
        node->keys.insert(node->keys.begin() + static_cast<std::ptrdiff_t>(i), std::move(key));
        ++size_;
        return true;
    }

    bool erase(const Key& key) {
        if (!contains(key)) {
            return false;
        }
        remove(*root_, key);
        --size_;
        if (root_->keys.empty()) {
            root_ = root_->is_leaf() ? nullptr : std::move(root_->children[0]);
        }
        return true;
    }

    std::optional<Key> lower_bound(const Key& key) const {
        std::optional<Key> answer;
        const Node* node = root_.get();
        while (node != nullptr) {
            const std::size_t i = lower_index(*node, key);
            if (i < node->keys.size()) {
                answer = node->keys[i];
            }
            node = node->is_leaf() ? nullptr : node->children[i].get();
        }
        return answer;
    }

    template <typename Visit>
    void scan(const Key& low, const Key& high, Visit visit) const {
        if (root_) {
            scan_node(*root_, low, high, visit);
        }
    }

private:
    bool is_full(const Node& node) const { return node.keys.size() == 2 * t_ - 1; }

    static std::size_t lower_index(const Node& node, const Key& key) {
        return static_cast<std::size_t>(std::lower_bound(node.keys.begin(), node.keys.end(), key) -
                                        node.keys.begin());
    }

    void split_child(Node& parent, std::size_t index) {
        Node& full = *parent.children[index];
        auto right = std::make_unique<Node>();
        right->keys.assign(std::make_move_iterator(full.keys.begin() + static_cast<std::ptrdiff_t>(t_)),
                           std::make_move_iterator(full.keys.end()));
        Key median = std::move(full.keys[t_ - 1]);
        full.keys.erase(full.keys.begin() + static_cast<std::ptrdiff_t>(t_ - 1), full.keys.end());
        if (!full.is_leaf()) {
            right->children.assign(
                std::make_move_iterator(full.children.begin() + static_cast<std::ptrdiff_t>(t_)),
                std::make_move_iterator(full.children.end()));
            full.children.erase(full.children.begin() + static_cast<std::ptrdiff_t>(t_),
                                full.children.end());
        }
        parent.keys.insert(parent.keys.begin() + static_cast<std::ptrdiff_t>(index), std::move(median));
        parent.children.insert(parent.children.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                               std::move(right));
    }

    // Merges children[index], keys[index] and children[index + 1] into children[index].
    void merge_children(Node& parent, std::size_t index) {
        Node& left = *parent.children[index];
        Node& right = *parent.children[index + 1];
        left.keys.push_back(std::move(parent.keys[index]));
        left.keys.insert(left.keys.end(), std::make_move_iterator(right.keys.begin()),
                         std::make_move_iterator(right.keys.end()));
        left.children.insert(left.children.end(), std::make_move_iterator(right.children.begin()),
                             std::make_move_iterator(right.children.end()));
        parent.keys.erase(parent.keys.begin() + static_cast<std::ptrdiff_t>(index));
        parent.children.erase(parent.children.begin() + static_cast<std::ptrdiff_t>(index) + 1);
    }

    void rotate_from_left(Node& parent, std::size_t index) {
        Node& child = *parent.children[index];
        Node& sibling = *parent.children[index - 1];
        child.keys.insert(child.keys.begin(), std::move(parent.keys[index - 1]));
        parent.keys[index - 1] = std::move(sibling.keys.back());
        sibling.keys.pop_back();
        if (!sibling.is_leaf()) {
            child.children.insert(child.children.begin(), std::move(sibling.children.back()));
            sibling.children.pop_back();
        }
    }

    void rotate_from_right(Node& parent, std::size_t index) {
        Node& child = *parent.children[index];
        Node& sibling = *parent.children[index + 1];
        child.keys.push_back(std::move(parent.keys[index]));
        parent.keys[index] = std::move(sibling.keys.front());
        sibling.keys.erase(sibling.keys.begin());
        if (!sibling.is_leaf()) {
            child.children.push_back(std::move(sibling.children.front()));
            sibling.children.erase(sibling.children.begin());
        }
    }

    // Removes key from the subtree of node, which holds at least t keys unless it is the root.
    void remove(Node& node, const Key& key) {
        std::size_t i = lower_index(node, key);
        const bool here = i < node.keys.size() && !(key < node.keys[i]);
        if (node.is_leaf()) {
            node.keys.erase(node.keys.begin() + static_cast<std::ptrdiff_t>(i));
            return;
        }
        if (here) {
            Node& left = *node.children[i];
            Node& right = *node.children[i + 1];
            if (left.keys.size() >= t_) {
                Key replacement = max_key(left);
                node.keys[i] = replacement;
                remove(left, replacement);
            } else if (right.keys.size() >= t_) {
                Key replacement = min_key(right);
                node.keys[i] = replacement;
                remove(right, replacement);
            } else {
                merge_children(node, i);
                remove(left, key);
            }
            return;
        }
        if (node.children[i]->keys.size() == t_ - 1) {
            if (i > 0 && node.children[i - 1]->keys.size() >= t_) {
                rotate_from_left(node, i);
            } else if (i + 1 < node.children.size() && node.children[i + 1]->keys.size() >= t_) {
                rotate_from_right(node, i);
            } else if (i + 1 < node.children.size()) {
                merge_children(node, i);
            } else {
                merge_children(node, i - 1);
                --i;
            }
        }
        remove(*node.children[i], key);
    }

    static const Key& max_key(const Node& node) {
        const Node* current = &node;
        while (!current->is_leaf()) {
            current = current->children.back().get();
        }
        return current->keys.back();
    }

    static const Key& min_key(const Node& node) {
        const Node* current = &node;
        while (!current->is_leaf()) {
            current = current->children.front().get();
        }
        return current->keys.front();
    }

    template <typename Visit>
    void scan_node(const Node& node, const Key& low, const Key& high, Visit& visit) const {
        std::size_t i = lower_index(node, low);
        for (; i < node.keys.size(); ++i) {
            if (!node.is_leaf()) {
                scan_node(*node.children[i], low, high, visit);
            }
            if (!(node.keys[i] < high)) {
                return;
            }
            visit(node.keys[i]);
        }
        if (!node.is_leaf()) {
            scan_node(*node.children[i], low, high, visit);
        }
    }

    std::size_t t_;
    std::size_t size_ = 0;
    std::unique_ptr<Node> root_;
};
