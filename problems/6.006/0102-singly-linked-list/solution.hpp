#pragma once

#include <cstddef>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "practice.hpp"

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

        reference operator*() const { PRACTICE_TODO(); }
        pointer operator->() const { PRACTICE_TODO(); }
        BasicIterator& operator++() { PRACTICE_TODO(); }
        BasicIterator operator++(int) { PRACTICE_TODO(); }
        friend bool operator==(const BasicIterator& a, const BasicIterator& b) { PRACTICE_TODO(); }
    };

    using iterator = BasicIterator<false>;
    using const_iterator = BasicIterator<true>;

    // Stage 1
    LinkedListSeq() { PRACTICE_TODO(); }
    explicit LinkedListSeq(std::span<const T> items) { PRACTICE_TODO(); }

    // Free every node, iteratively.
    ~LinkedListSeq() {}

    LinkedListSeq(const LinkedListSeq&) = delete;
    LinkedListSeq& operator=(const LinkedListSeq&) = delete;

    // These should be noexcept once implemented.
    LinkedListSeq(LinkedListSeq&& other) { PRACTICE_TODO(); }
    LinkedListSeq& operator=(LinkedListSeq&& other) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    Node* head() const { PRACTICE_TODO(); }

    Node* insert_first(T value) { PRACTICE_TODO(); }
    T delete_first() { PRACTICE_TODO(); }

    // Stage 2
    const T& get_at(std::size_t i) const { PRACTICE_TODO(); }
    void set_at(std::size_t i, T value) { PRACTICE_TODO(); }
    Node* insert_at(std::size_t i, T value) { PRACTICE_TODO(); }
    T delete_at(std::size_t i) { PRACTICE_TODO(); }
    Node* insert_last(T value) { PRACTICE_TODO(); }
    T delete_last() { PRACTICE_TODO(); }

    iterator begin() { PRACTICE_TODO(); }
    iterator end() { PRACTICE_TODO(); }
    const_iterator begin() const { PRACTICE_TODO(); }
    const_iterator end() const { PRACTICE_TODO(); }

    // Stage 3
    void reverse() { PRACTICE_TODO(); }
};

// Stage 4
template <typename Node>
struct CycleInfo {
    std::size_t length;
    const Node* entry;
};

template <typename Node>
std::optional<CycleInfo<Node>> find_cycle(const Node* head) {
    PRACTICE_TODO();
}
