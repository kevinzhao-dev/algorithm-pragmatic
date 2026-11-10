#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "practice.hpp"

template <typename T>
class DoublyLinkedList {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)) {}
        T item;
        Node* prev = nullptr;
        Node* next = nullptr;
    };

    DoublyLinkedList() = default;

    // Stage 1: free every node, iteratively.
    ~DoublyLinkedList() {}

    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    // Stage 3. Once implemented, these should be noexcept.
    DoublyLinkedList(DoublyLinkedList&& other) { PRACTICE_TODO(); }
    DoublyLinkedList& operator=(DoublyLinkedList&& other) { PRACTICE_TODO(); }

    bool empty() const { return head_ == nullptr; }
    Node* head() const { return head_; }
    Node* tail() const { return tail_; }

    // Stage 1
    Node* insert_first(T value) { PRACTICE_TODO(); }
    Node* insert_last(T value) { PRACTICE_TODO(); }
    T delete_first() { PRACTICE_TODO(); }
    T delete_last() { PRACTICE_TODO(); }

    // Stage 2
    DoublyLinkedList remove(Node* first, Node* last) { PRACTICE_TODO(); }

    // Stage 3
    void splice(Node* after, DoublyLinkedList& other) { PRACTICE_TODO(); }

private:
    Node* head_ = nullptr;
    Node* tail_ = nullptr;
};

// Stage 4
template <typename K, typename V>
class LruCache {
public:
    explicit LruCache(std::size_t capacity) { PRACTICE_TODO(); }

    std::optional<V> get(const K& key) { PRACTICE_TODO(); }
    std::optional<K> put(K key, V value) { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
