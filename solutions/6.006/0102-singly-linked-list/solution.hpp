#pragma once

#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

template <typename T>
class LinkedListSeq {
public:
    struct Node {
        explicit Node(T value) : item(std::move(value)) {}
        T item;
        Node* next = nullptr;
    };

    template <bool IsConst>
    class BasicIterator {
    public:
        using value_type = T;
        using difference_type = std::ptrdiff_t;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using iterator_category = std::forward_iterator_tag;

        BasicIterator() = default;
        explicit BasicIterator(Node* node) : node_(node) {}

        reference operator*() const { return node_->item; }
        pointer operator->() const { return &node_->item; }
        BasicIterator& operator++() {
            node_ = node_->next;
            return *this;
        }
        BasicIterator operator++(int) {
            BasicIterator old = *this;
            ++*this;
            return old;
        }
        friend bool operator==(const BasicIterator& a, const BasicIterator& b) {
            return a.node_ == b.node_;
        }

    private:
        Node* node_ = nullptr;
    };

    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;

    LinkedListSeq() = default;

    explicit LinkedListSeq(std::span<const T> items) {
        for (auto it = items.rbegin(); it != items.rend(); ++it) {
            insert_first(*it);
        }
    }

    ~LinkedListSeq() { clear(); }

    LinkedListSeq(const LinkedListSeq&) = delete;
    LinkedListSeq& operator=(const LinkedListSeq&) = delete;

    LinkedListSeq(LinkedListSeq&& other) noexcept
        : head_(std::exchange(other.head_, nullptr)), size_(std::exchange(other.size_, 0)) {}

    LinkedListSeq& operator=(LinkedListSeq&& other) noexcept {
        if (this != &other) {
            clear();
            head_ = std::exchange(other.head_, nullptr);
            size_ = std::exchange(other.size_, 0);
        }
        return *this;
    }

    std::size_t size() const { return size_; }
    bool empty() const { return size_ == 0; }
    Node* head() const { return head_; }

    Node* insert_first(T value) {
        Node* node = new Node(std::move(value));
        node->next = head_;
        head_ = node;
        ++size_;
        return node;
    }

    T delete_first() {
        if (head_ == nullptr) {
            throw std::out_of_range("delete_first on empty list");
        }
        Node* node = head_;
        head_ = node->next;
        --size_;
        T value = std::move(node->item);
        delete node;
        return value;
    }

    const T& get_at(std::size_t i) const { return node_at(i)->item; }

    void set_at(std::size_t i, T value) { node_at(i)->item = std::move(value); }

    Node* insert_at(std::size_t i, T value) {
        if (i > size_) {
            throw std::out_of_range("insert_at index out of range");
        }
        if (i == 0) {
            return insert_first(std::move(value));
        }
        Node* previous = walk(i - 1);
        Node* node = new Node(std::move(value));
        node->next = previous->next;
        previous->next = node;
        ++size_;
        return node;
    }

    T delete_at(std::size_t i) {
        if (i >= size_) {
            throw std::out_of_range("delete_at index out of range");
        }
        if (i == 0) {
            return delete_first();
        }
        Node* previous = walk(i - 1);
        Node* node = previous->next;
        previous->next = node->next;
        --size_;
        T value = std::move(node->item);
        delete node;
        return value;
    }

    Node* insert_last(T value) { return insert_at(size_, std::move(value)); }

    T delete_last() {
        if (size_ == 0) {
            throw std::out_of_range("delete_last on empty list");
        }
        return delete_at(size_ - 1);
    }

    iterator begin() { return iterator(head_); }
    iterator end() { return iterator(nullptr); }
    const_iterator begin() const { return const_iterator(head_); }
    const_iterator end() const { return const_iterator(nullptr); }

    void reverse() {
        Node* previous = nullptr;
        Node* node = head_;
        while (node != nullptr) {
            Node* next = node->next;
            node->next = previous;
            previous = node;
            node = next;
        }
        head_ = previous;
    }

private:
    Node* walk(std::size_t i) const {
        Node* node = head_;
        for (std::size_t step = 0; step < i; ++step) {
            node = node->next;
        }
        return node;
    }

    Node* node_at(std::size_t i) const {
        if (i >= size_) {
            throw std::out_of_range("index out of range");
        }
        return walk(i);
    }

    void clear() {
        while (head_ != nullptr) {
            delete std::exchange(head_, head_->next);
        }
        size_ = 0;
    }

    Node* head_ = nullptr;
    std::size_t size_ = 0;
};

template <typename Node>
struct CycleInfo {
    std::size_t length;
    const Node* entry;
};

template <typename Node>
std::optional<CycleInfo<Node>> find_cycle(const Node* head) {
    const Node* slow = head;
    const Node* fast = head;
    while (fast != nullptr && fast->next != nullptr) {
        slow = slow->next;
        fast = fast->next->next;
        if (slow == fast) {
            std::size_t length = 1;
            for (const Node* node = slow->next; node != slow; node = node->next) {
                ++length;
            }
            // Two walkers `length` apart meet exactly at the first node of the cycle.
            const Node* behind = head;
            const Node* ahead = head;
            for (std::size_t i = 0; i < length; ++i) {
                ahead = ahead->next;
            }
            while (behind != ahead) {
                behind = behind->next;
                ahead = ahead->next;
            }
            return CycleInfo<Node>{length, behind};
        }
    }
    return std::nullopt;
}
