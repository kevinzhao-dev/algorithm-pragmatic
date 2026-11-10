#pragma once

#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>

#include "practice.hpp"

// An augmentation folds a monoid over the traversal order of every subtree. Stage 4.
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
        int height = 0;          // leaf: 0, empty subtree counts as -1
        std::size_t size = 1;    // number of nodes in this subtree
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
        reference operator*() const { PRACTICE_TODO(); }
        pointer operator->() const { PRACTICE_TODO(); }
        iterator& operator++() { PRACTICE_TODO(); }
        iterator operator++(int) { PRACTICE_TODO(); }
        friend bool operator==(const iterator&, const iterator&) { PRACTICE_TODO(); }
    };

    AvlTree() = default;
    ~AvlTree() {}  // Stage 1: release every node.

    AvlTree(const AvlTree&) = delete;
    AvlTree& operator=(const AvlTree&) = delete;

    // Stage 5. Once implemented, these should be noexcept.
    AvlTree(AvlTree&& other) { PRACTICE_TODO(); }
    AvlTree& operator=(AvlTree&& other) { PRACTICE_TODO(); }

    // Stage 1
    Node* root() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    iterator begin() const { PRACTICE_TODO(); }
    iterator end() const { PRACTICE_TODO(); }

    static Node* subtree_first(Node* node) { PRACTICE_TODO(); }
    static Node* subtree_last(Node* node) { PRACTICE_TODO(); }
    static Node* successor(Node* node) { PRACTICE_TODO(); }
    static Node* predecessor(Node* node) { PRACTICE_TODO(); }

    Node* insert_before(Node* anchor, T value) { PRACTICE_TODO(); }
    Node* insert_after(Node* anchor, T value) { PRACTICE_TODO(); }
    T erase(Node* node) { PRACTICE_TODO(); }

    // Stage 2
    Node* rotate_left(Node* node) { PRACTICE_TODO(); }
    Node* rotate_right(Node* node) { PRACTICE_TODO(); }

    // Stage 4
    void refresh(Node* node) { PRACTICE_TODO(); }

    // Stage 5
    static AvlTree build(std::vector<T> items) { PRACTICE_TODO(); }
};
