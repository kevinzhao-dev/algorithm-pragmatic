#pragma once

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

// An augmentation folds a monoid over the traversal order of every subtree.
struct NoAugment {
    struct Value {};
    static Value identity() { return {}; }
    template <typename T>
    static Value of(const T&) {
        return {};
    }
    static Value combine(const Value&, const Value&) { return {}; }
};

template <typename T, typename Aug = NoAugment>
class AvlTree {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)), aug(Aug::of(item)) {}
        T item;
        Node* parent = nullptr;
        Node* left = nullptr;
        Node* right = nullptr;
        int height = 0;
        std::size_t size = 1;
        typename Aug::Value aug;
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

    AvlTree() = default;
    ~AvlTree() { clear(); }

    AvlTree(const AvlTree&) = delete;
    AvlTree& operator=(const AvlTree&) = delete;

    AvlTree(AvlTree&& other) noexcept
        : root_(std::exchange(other.root_, nullptr)), size_(std::exchange(other.size_, 0)) {}
    AvlTree& operator=(AvlTree&& other) noexcept {
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

    iterator begin() const { return iterator(subtree_first(root_)); }
    iterator end() const { return iterator(nullptr); }

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
        Node* parent = anchor;
        if (anchor->left != nullptr) {
            parent = subtree_last(anchor->left);
            parent->right = node;
        } else {
            anchor->left = node;
        }
        attach(node, parent);
        return node;
    }

    Node* insert_after(Node* anchor, T value) {
        Node* node = new Node(std::move(value));
        Node* parent = anchor;
        if (anchor == nullptr) {
            parent = subtree_first(root_);
            if (parent != nullptr) {
                parent->left = node;
            } else {
                root_ = node;
            }
        } else if (anchor->right != nullptr) {
            parent = subtree_first(anchor->right);
            parent->left = node;
        } else {
            anchor->right = node;
        }
        attach(node, parent);
        return node;
    }

    T erase(Node* node) {
        Node* lowest_changed = nullptr;
        if (node->left != nullptr && node->right != nullptr) {
            Node* next = subtree_first(node->right);
            lowest_changed = next->parent == node ? next : next->parent;
            replace(next, next->right);
            next->left = node->left;
            next->left->parent = next;
            next->right = node->right;
            if (next->right != nullptr) {
                next->right->parent = next;
            }
            replace(node, next);
        } else {
            lowest_changed = node->parent;
            replace(node, node->left != nullptr ? node->left : node->right);
        }
        T value = std::move(node->item);
        delete node;
        --size_;
        maintain(lowest_changed);
        return value;
    }

    // Both return the node that took the rotated node's place. Only the two rotated
    // nodes are updated; ancestors are the caller's job, which is what maintain() does.
    Node* rotate_left(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left != nullptr) {
            y->left->parent = x;
        }
        replace(x, y);
        y->left = x;
        x->parent = y;
        pull(x);
        pull(y);
        return y;
    }

    Node* rotate_right(Node* x) {
        Node* y = x->left;
        x->left = y->right;
        if (y->right != nullptr) {
            y->right->parent = x;
        }
        replace(x, y);
        y->right = x;
        x->parent = y;
        pull(x);
        pull(y);
        return y;
    }

    // Call after changing node->item in place.
    void refresh(Node* node) {
        for (; node != nullptr; node = node->parent) {
            pull(node);
        }
    }

    static AvlTree build(std::vector<T> items) {
        AvlTree tree;
        tree.root_ = tree.build_range(items, 0, items.size());
        tree.size_ = items.size();
        return tree;
    }

private:
    static int height_of(const Node* node) { return node != nullptr ? node->height : -1; }
    static std::size_t size_of(const Node* node) { return node != nullptr ? node->size : 0; }
    static typename Aug::Value aug_of(const Node* node) {
        return node != nullptr ? node->aug : Aug::identity();
    }
    static int skew(const Node* node) { return height_of(node->right) - height_of(node->left); }

    static void pull(Node* node) {
        node->height = 1 + std::max(height_of(node->left), height_of(node->right));
        node->size = 1 + size_of(node->left) + size_of(node->right);
        node->aug = Aug::combine(Aug::combine(aug_of(node->left), Aug::of(node->item)), aug_of(node->right));
    }

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

    void attach(Node* node, Node* parent) {
        node->parent = parent;
        ++size_;
        maintain(parent);
    }

    Node* rebalance(Node* node) {
        if (skew(node) == 2) {
            if (skew(node->right) < 0) {
                rotate_right(node->right);
            }
            return rotate_left(node);
        }
        if (skew(node) == -2) {
            if (skew(node->left) > 0) {
                rotate_left(node->left);
            }
            return rotate_right(node);
        }
        return node;
    }

    void maintain(Node* node) {
        while (node != nullptr) {
            pull(node);
            node = rebalance(node)->parent;
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
        pull(node);
        return node;
    }

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
