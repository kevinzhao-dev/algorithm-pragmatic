#pragma once

#include <algorithm>
#include <cstddef>
#include <memory>
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
        : t_(other.t_),
          size_(std::exchange(other.size_, 0)),
          height_(std::exchange(other.height_, -1)),
          root_(std::move(other.root_)) {}

    BTree& operator=(BTree&& other) noexcept {
        if (this != &other) {
            t_ = other.t_;
            size_ = std::exchange(other.size_, 0);
            height_ = std::exchange(other.height_, -1);
            root_ = std::move(other.root_);
        }
        return *this;
    }

    std::size_t min_degree() const { return t_; }
    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    int height() const { return height_; }
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
            height_ = 0;
        } else if (is_full(*root_)) {
            grow_root();
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
            --height_;
        }
        return true;
    }

    static BTree combine(BTree&& left, Key key, BTree&& right) {
        if (left.t_ != right.t_) {
            throw std::invalid_argument("combine: minimum degrees differ");
        }
        BTree a(std::move(left));
        BTree b(std::move(right));
        if (a.root_ == nullptr || b.root_ == nullptr) {
            BTree& kept = a.root_ == nullptr ? b : a;
            kept.insert(std::move(key));
            return std::move(kept);
        }
        if (a.height_ >= b.height_) {
            a.graft(std::move(key), std::move(b), true);
            return std::move(a);
        }
        b.graft(std::move(key), std::move(a), false);
        return std::move(b);
    }

    static BTree concat(BTree&& left, BTree&& right) {
        BTree a(std::move(left));
        BTree b(std::move(right));
        if (b.root_ == nullptr) {
            return std::move(a);
        }
        Key separator = min_key(*b.root_);
        b.erase(separator);
        return combine(std::move(a), std::move(separator), std::move(b));
    }

private:
    bool is_full(const Node& node) const { return node.keys.size() == 2 * t_ - 1; }

    void grow_root() {
        auto new_root = std::make_unique<Node>();
        new_root->children.push_back(std::move(root_));
        root_ = std::move(new_root);
        split_child(*root_, 0);
        ++height_;
    }

    // Fuses key and the root of `other` into the spine node at other's height, then splits any
    // overfull node on the way back up. Only the spine is touched, so this is O(t * height gap).
    void graft(Key key, BTree other, bool other_on_right) {
        std::vector<Node*> spine{root_.get()};
        for (int level = height_; level > other.height_; --level) {
            Node& node = *spine.back();
            spine.push_back((other_on_right ? node.children.back() : node.children.front()).get());
        }
        Node& target = *spine.back();
        Node& graft_root = *other.root_;
        if (other_on_right) {
            target.keys.push_back(std::move(key));
            target.keys.insert(target.keys.end(), std::make_move_iterator(graft_root.keys.begin()),
                               std::make_move_iterator(graft_root.keys.end()));
            target.children.insert(target.children.end(),
                                   std::make_move_iterator(graft_root.children.begin()),
                                   std::make_move_iterator(graft_root.children.end()));
        } else {
            target.keys.insert(target.keys.begin(), std::move(key));
            target.keys.insert(target.keys.begin(), std::make_move_iterator(graft_root.keys.begin()),
                               std::make_move_iterator(graft_root.keys.end()));
            target.children.insert(target.children.begin(),
                                   std::make_move_iterator(graft_root.children.begin()),
                                   std::make_move_iterator(graft_root.children.end()));
        }
        size_ += other.size_ + 1;

        for (std::size_t depth = spine.size() - 1; depth > 0; --depth) {
            if (spine[depth]->keys.size() <= 2 * t_ - 1) {
                return;
            }
            Node& parent = *spine[depth - 1];
            split_child(parent, other_on_right ? parent.children.size() - 1 : 0);
        }
        if (root_->keys.size() > 2 * t_ - 1) {
            grow_root();
        }
    }

    static std::size_t lower_index(const Node& node, const Key& key) {
        return static_cast<std::size_t>(std::lower_bound(node.keys.begin(), node.keys.end(), key) -
                                        node.keys.begin());
    }

    // Splits an overfull or full child around its median key.
    void split_child(Node& parent, std::size_t index) {
        Node& full = *parent.children[index];
        const std::size_t middle = full.keys.size() / 2;
        auto right = std::make_unique<Node>();
        right->keys.assign(std::make_move_iterator(full.keys.begin() + static_cast<std::ptrdiff_t>(middle + 1)),
                           std::make_move_iterator(full.keys.end()));
        Key median = std::move(full.keys[middle]);
        full.keys.erase(full.keys.begin() + static_cast<std::ptrdiff_t>(middle), full.keys.end());
        if (!full.is_leaf()) {
            right->children.assign(
                std::make_move_iterator(full.children.begin() + static_cast<std::ptrdiff_t>(middle + 1)),
                std::make_move_iterator(full.children.end()));
            full.children.erase(full.children.begin() + static_cast<std::ptrdiff_t>(middle + 1),
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

    std::size_t t_;
    std::size_t size_ = 0;
    int height_ = -1;
    std::unique_ptr<Node> root_;
};
