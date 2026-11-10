#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "practice.hpp"

template <typename Key>
class IndexedSkipList {
public:
    class Node {
    public:
        const Key& key() const { PRACTICE_TODO(); }
    };

    // Stage 1
    explicit IndexedSkipList(std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    // Free every node, iteratively.
    ~IndexedSkipList() {}

    IndexedSkipList(const IndexedSkipList&) = delete;
    IndexedSkipList& operator=(const IndexedSkipList&) = delete;

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    Node* first() const { PRACTICE_TODO(); }
    Node* next(const Node* node) const { PRACTICE_TODO(); }
    Node* prev(const Node* node) const { PRACTICE_TODO(); }
    Node* find(const Key& key) const { PRACTICE_TODO(); }
    Node* insert(Key key) { PRACTICE_TODO(); }
    bool erase(const Key& key) { PRACTICE_TODO(); }
    Node* finger_search(Node* from, const Key& key) const { PRACTICE_TODO(); }

    // Stage 2
    std::size_t rank(const Node* node) const { PRACTICE_TODO(); }
    Node* at(std::size_t index) const { PRACTICE_TODO(); }

    // Stage 3
    Node* rank_search(Node* from, std::ptrdiff_t delta) const { PRACTICE_TODO(); }

    // Cost model used by the tests: links followed plus level changes.
    std::uint64_t steps() const { PRACTICE_TODO(); }
};

// Stage 4
class SlidingMedian {
public:
    explicit SlidingMedian(std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    std::optional<std::int64_t> median() const { PRACTICE_TODO(); }
    void push(std::int64_t value) { PRACTICE_TODO(); }
    void pop() { PRACTICE_TODO(); }
    std::uint64_t steps() const { PRACTICE_TODO(); }
};
