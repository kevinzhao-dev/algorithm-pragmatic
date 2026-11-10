#pragma once

#include <cstddef>
#include <functional>
#include <iterator>
#include <optional>
#include <utility>
#include <vector>

#include "practice.hpp"

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

    // Stage 2
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

    BinaryTree() = default;

    // Stage 2: release every node without recursion.
    ~BinaryTree() {}

    BinaryTree(const BinaryTree&) = delete;
    BinaryTree& operator=(const BinaryTree&) = delete;

    // Stage 4. Once implemented, these should be noexcept.
    BinaryTree(BinaryTree&& other) { PRACTICE_TODO(); }
    BinaryTree& operator=(BinaryTree&& other) { PRACTICE_TODO(); }

    // Stage 1
    static Node* subtree_first(Node* node) { PRACTICE_TODO(); }
    static Node* subtree_last(Node* node) { PRACTICE_TODO(); }
    static Node* successor(Node* node) { PRACTICE_TODO(); }
    static Node* predecessor(Node* node) { PRACTICE_TODO(); }

    // Stage 2
    Node* root() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    Node* insert_before(Node* anchor, T value) { PRACTICE_TODO(); }
    Node* insert_after(Node* anchor, T value) { PRACTICE_TODO(); }
    iterator begin() const { PRACTICE_TODO(); }
    iterator end() const { PRACTICE_TODO(); }

    // Stage 3
    T erase(Node* node) { PRACTICE_TODO(); }

    // Stage 4
    static BinaryTree build(std::vector<T> items) { PRACTICE_TODO(); }
};

// Stage 5
template <typename K, typename Compare = std::less<K>>
class BstSet {
public:
    explicit BstSet(Compare compare = Compare{}) { PRACTICE_TODO(); }

    bool insert(K key) { PRACTICE_TODO(); }
    bool contains(const K& key) const { PRACTICE_TODO(); }
    bool erase(const K& key) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::optional<K> find_min() const { PRACTICE_TODO(); }
    std::optional<K> find_max() const { PRACTICE_TODO(); }
    std::optional<K> find_next(const K& key) const { PRACTICE_TODO(); }
    std::optional<K> find_prev(const K& key) const { PRACTICE_TODO(); }
    std::vector<K> to_vector() const { PRACTICE_TODO(); }
    int height() const { PRACTICE_TODO(); }
};
