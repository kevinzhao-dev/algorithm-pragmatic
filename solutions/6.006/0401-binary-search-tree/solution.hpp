#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <queue>
#include <span>
#include <utility>
#include <vector>

template <typename T>
class BinaryTree {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)) {}
        T item;
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
    };

    class iterator {
    public:
        using iterator_concept = std::forward_iterator_tag;
        using iterator_category = std::forward_iterator_tag;
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using pointer = T*;
        using reference = T&;

        iterator() = default;
        explicit iterator(Node* node) : node_(node) {}

        reference operator*() const { return node_->item; }
        pointer operator->() const { return &node_->item; }
        iterator& operator++() {
            node_ = successor(node_);
            return *this;
        }
        iterator operator++(int) {
            iterator before = *this;
            ++*this;
            return before;
        }
        friend bool operator==(const iterator&, const iterator&) = default;

    private:
        Node* node_ = nullptr;
    };

    BinaryTree() = default;
    ~BinaryTree() { clear(); }

    BinaryTree(const BinaryTree&) = delete;
    BinaryTree& operator=(const BinaryTree&) = delete;

    BinaryTree(BinaryTree&& other) noexcept
        : root_(std::exchange(other.root_, nullptr)), size_(std::exchange(other.size_, 0)) {}
    BinaryTree& operator=(BinaryTree&& other) noexcept {
        if (this != &other) {
            clear();
            root_ = std::exchange(other.root_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    Node* root() const { return root_; }
    std::size_t size() const { return size_; }
    bool empty() const { return root_ == nullptr; }

    static Node* subtree_first(Node* node) {
        while (node != nullptr && node->left != nullptr) {
            node = node->left;
        }
        return node;
    }

    static Node* subtree_last(Node* node) {
        while (node != nullptr && node->right != nullptr) {
            node = node->right;
        }
        return node;
    }

    static Node* successor(Node* node) {
        if (node->right != nullptr) {
            return subtree_first(node->right);
        }
        while (node->parent != nullptr && node == node->parent->right) {
            node = node->parent;
        }
        return node->parent;
    }

    static Node* predecessor(Node* node) {
        if (node->left != nullptr) {
            return subtree_last(node->left);
        }
        while (node->parent != nullptr && node == node->parent->left) {
            node = node->parent;
        }
        return node->parent;
    }

    Node* insert_before(Node* anchor, T value) {
        if (anchor == nullptr) {
            return insert_after(subtree_last(root_), std::move(value));
        }
        Node* node = new Node(std::move(value));
        if (anchor->left != nullptr) {
            Node* last = subtree_last(anchor->left);
            last->right = node;
            node->parent = last;
        } else {
            anchor->left = node;
            node->parent = anchor;
        }
        ++size_;
        return node;
    }

    Node* insert_after(Node* anchor, T value) {
        Node* node = new Node(std::move(value));
        if (anchor == nullptr) {
            Node* first = subtree_first(root_);
            if (first == nullptr) {
                root_ = node;
            } else {
                first->left = node;
                node->parent = first;
            }
        } else if (anchor->right != nullptr) {
            Node* first = subtree_first(anchor->right);
            first->left = node;
            node->parent = first;
        } else {
            anchor->right = node;
            node->parent = anchor;
        }
        ++size_;
        return node;
    }

    // Relinks instead of swapping items, so every other Node* keeps holding the same item.
    T erase(Node* node) {
        if (node->left != nullptr && node->right != nullptr) {
            Node* next = subtree_first(node->right);
            replace(next, next->right);
            next->left = node->left;
            next->left->parent = next;
            next->right = node->right;
            if (next->right != nullptr) {
                next->right->parent = next;
            }
            replace(node, next);
        } else {
            replace(node, node->left != nullptr ? node->left : node->right);
        }
        T value = std::move(node->item);
        delete node;
        --size_;
        return value;
    }

    iterator begin() const { return iterator(subtree_first(root_)); }
    iterator end() const { return iterator(nullptr); }

    static BinaryTree build(std::vector<T> items) {
        BinaryTree tree;
        tree.root_ = tree.build_range(items, 0, items.size());
        tree.size_ = items.size();
        return tree;
    }

private:
    // Makes `incoming` (possibly null) take the place of `outgoing` under its parent.
    void replace(Node* outgoing, Node* incoming) {
        Node* parent = outgoing->parent;
        if (parent == nullptr) {
            root_ = incoming;
        } else if (parent->left == outgoing) {
            parent->left = incoming;
        } else {
            parent->right = incoming;
        }
        if (incoming != nullptr) {
            incoming->parent = parent;
        }
    }

    Node* build_range(std::vector<T>& items, std::size_t begin, std::size_t end) {
        if (begin == end) {
            return nullptr;
        }
        const std::size_t middle = begin + (end - begin) / 2;
        Node* node = new Node(std::move(items[middle]));
        node->left = build_range(items, begin, middle);
        node->right = build_range(items, middle + 1, end);
        if (node->left != nullptr) {
            node->left->parent = node;
        }
        if (node->right != nullptr) {
            node->right->parent = node;
        }
        return node;
    }

    // Deletes leaves bottom-up with no recursion and no extra memory.
    void clear() {
        Node* node = root_;
        while (node != nullptr) {
            if (node->left != nullptr) {
                node = node->left;
            } else if (node->right != nullptr) {
                node = node->right;
            } else {
                Node* parent = node->parent;
                if (parent != nullptr) {
                    (parent->left == node ? parent->left : parent->right) = nullptr;
                }
                delete node;
                node = parent;
            }
        }
        root_ = nullptr;
        size_ = 0;
    }

    Node* root_ = nullptr;
    std::size_t size_ = 0;
};

template <typename K, typename Compare = std::less<K>>
class BstSet {
    using Tree = BinaryTree<K>;
    using Node = typename Tree::Node;

public:
    explicit BstSet(Compare compare = Compare{}) : compare_(std::move(compare)) {}

    bool insert(K key) {
        if (tree_.root() == nullptr) {
            tree_.insert_after(nullptr, std::move(key));
            return true;
        }
        Node* node = tree_.root();
        while (true) {
            if (compare_(key, node->item)) {
                if (node->left == nullptr) {
                    tree_.insert_before(node, std::move(key));
                    return true;
                }
                node = node->left;
            } else if (compare_(node->item, key)) {
                if (node->right == nullptr) {
                    tree_.insert_after(node, std::move(key));
                    return true;
                }
                node = node->right;
            } else {
                return false;
            }
        }
    }

    bool contains(const K& key) const { return find_node(key) != nullptr; }

    bool erase(const K& key) {
        Node* node = find_node(key);
        if (node == nullptr) {
            return false;
        }
        tree_.erase(node);
        return true;
    }

    std::size_t size() const { return tree_.size(); }
    bool empty() const { return tree_.empty(); }

    std::optional<K> find_min() const { return value_of(Tree::subtree_first(tree_.root())); }
    std::optional<K> find_max() const { return value_of(Tree::subtree_last(tree_.root())); }

    std::optional<K> find_next(const K& key) const {
        Node* node = tree_.root();
        Node* best = nullptr;
        while (node != nullptr) {
            if (compare_(key, node->item)) {
                best = node;
                node = node->left;
            } else {
                node = node->right;
            }
        }
        return value_of(best);
    }

    std::optional<K> find_prev(const K& key) const {
        Node* node = tree_.root();
        Node* best = nullptr;
        while (node != nullptr) {
            if (compare_(node->item, key)) {
                best = node;
                node = node->right;
            } else {
                node = node->left;
            }
        }
        return value_of(best);
    }

    std::vector<K> to_vector() const { return std::vector<K>(tree_.begin(), tree_.end()); }

    int height() const {
        if (tree_.root() == nullptr) {
            return -1;
        }
        std::queue<std::pair<Node*, int>> frontier;
        frontier.emplace(tree_.root(), 0);
        int deepest = 0;
        while (!frontier.empty()) {
            const auto [node, depth] = frontier.front();
            frontier.pop();
            deepest = std::max(deepest, depth);
            if (node->left != nullptr) {
                frontier.emplace(node->left, depth + 1);
            }
            if (node->right != nullptr) {
                frontier.emplace(node->right, depth + 1);
            }
        }
        return deepest;
    }

private:
    Node* find_node(const K& key) const {
        Node* node = tree_.root();
        while (node != nullptr) {
            if (compare_(key, node->item)) {
                node = node->left;
            } else if (compare_(node->item, key)) {
                node = node->right;
            } else {
                return node;
            }
        }
        return nullptr;
    }

    static std::optional<K> value_of(Node* node) {
        return node == nullptr ? std::nullopt : std::optional<K>(node->item);
    }

    Tree tree_;
    Compare compare_;
};
