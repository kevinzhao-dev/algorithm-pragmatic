#pragma once

#include <cstddef>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

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

    ~DoublyLinkedList() { destroy_all(); }

    DoublyLinkedList(const DoublyLinkedList&) = delete;
    DoublyLinkedList& operator=(const DoublyLinkedList&) = delete;

    DoublyLinkedList(DoublyLinkedList&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)), tail_(std::exchange(other.tail_, nullptr)) {}

    DoublyLinkedList& operator=(DoublyLinkedList&& other) noexcept {
        if (this != &other) {
            destroy_all();
            head_ = std::exchange(other.head_, nullptr);
            tail_ = std::exchange(other.tail_, nullptr);
        }
        return *this;
    }

    bool empty() const { return head_ == nullptr; }
    Node* head() const { return head_; }
    Node* tail() const { return tail_; }

    Node* insert_first(T value) {
        Node* node = new Node(std::move(value));
        node->next = head_;
        (head_ != nullptr ? head_->prev : tail_) = node;
        head_ = node;
        return node;
    }

    Node* insert_last(T value) {
        Node* node = new Node(std::move(value));
        node->prev = tail_;
        (tail_ != nullptr ? tail_->next : head_) = node;
        tail_ = node;
        return node;
    }

    T delete_first() {
        if (empty()) {
            throw std::out_of_range("delete_first on empty list");
        }
        return take(head_, head_);
    }

    T delete_last() {
        if (empty()) {
            throw std::out_of_range("delete_last on empty list");
        }
        return take(tail_, tail_);
    }

    DoublyLinkedList remove(Node* first, Node* last) {
        unlink(first, last);
        DoublyLinkedList result;
        result.head_ = first;
        result.tail_ = last;
        return result;
    }

    void splice(Node* after, DoublyLinkedList& other) {
        if (other.empty()) {
            return;
        }
        Node* first = std::exchange(other.head_, nullptr);
        Node* last = std::exchange(other.tail_, nullptr);
        Node* before = after->next;

        after->next = first;
        first->prev = after;
        last->next = before;
        (before != nullptr ? before->prev : tail_) = last;
    }

private:
    // Detaches [first, last] and clears its outer links.
    void unlink(Node* first, Node* last) {
        Node* before = first->prev;
        Node* after = last->next;
        (before != nullptr ? before->next : head_) = after;
        (after != nullptr ? after->prev : tail_) = before;
        first->prev = nullptr;
        last->next = nullptr;
    }

    T take(Node* first, Node* last) {
        unlink(first, last);
        T value = std::move(first->item);
        delete first;
        return value;
    }

    void destroy_all() {
        while (head_ != nullptr) {
            delete std::exchange(head_, head_->next);
        }
        tail_ = nullptr;
    }

    Node* head_ = nullptr;
    Node* tail_ = nullptr;
};

template <typename K, typename V>
class LruCache {
public:
    explicit LruCache(std::size_t capacity) : capacity_(capacity) {}

    std::optional<V> get(const K& key) {
        const auto found = index_.find(key);
        if (found == index_.end()) {
            return std::nullopt;
        }
        touch(found->second);
        return found->second->item.second;
    }

    std::optional<K> put(K key, V value) {
        if (const auto found = index_.find(key); found != index_.end()) {
            found->second->item.second = std::move(value);
            touch(found->second);
            return std::nullopt;
        }

        std::optional<K> evicted;
        if (index_.size() == capacity_) {
            evicted = recency_.delete_first().first;
            index_.erase(*evicted);
        }
        Node* node = recency_.insert_last({key, std::move(value)});
        index_.emplace(std::move(key), node);
        return evicted;
    }

    std::size_t size() const { return index_.size(); }

private:
    using List = DoublyLinkedList<std::pair<K, V>>;
    using Node = typename List::Node;

    // Least recently used lives at the head, most recently used at the tail.
    void touch(Node* node) {
        if (node == recency_.tail()) {
            return;
        }
        List moved = recency_.remove(node, node);
        recency_.splice(recency_.tail(), moved);
    }

    std::size_t capacity_;
    List recency_;
    std::unordered_map<K, Node*> index_;
};
