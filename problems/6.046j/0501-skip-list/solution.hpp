#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "practice.hpp"

template <typename Key>
class SkipList {
public:
    struct Node {
        Node(Key k, std::size_t height) : key(std::move(k)), next(height, nullptr) {}

        Key key;
        std::vector<Node*> next;
    };

    // Stage 1
    explicit SkipList(std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    // Free every node, iteratively.
    ~SkipList() {}

    SkipList(const SkipList&) = delete;
    SkipList& operator=(const SkipList&) = delete;

    // Once implemented, these should be noexcept.
    SkipList(SkipList&& other) { PRACTICE_TODO(); }
    SkipList& operator=(SkipList&& other) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    int height() const { PRACTICE_TODO(); }
    Node* first_at(int level) const { PRACTICE_TODO(); }
    Node* first() const { PRACTICE_TODO(); }

    const Node* find(const Key& key) const { PRACTICE_TODO(); }
    bool contains(const Key& key) const { PRACTICE_TODO(); }
    bool insert(Key key) { PRACTICE_TODO(); }

    // Stage 2
    bool erase(const Key& key) { PRACTICE_TODO(); }
};

// Stage 3
class JobScheduler {
public:
    explicit JobScheduler(std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    int schedule(std::int64_t run_at) { PRACTICE_TODO(); }
    bool cancel(int job) { PRACTICE_TODO(); }
    std::vector<int> pop_due(std::int64_t now) { PRACTICE_TODO(); }
    std::size_t pending() const { PRACTICE_TODO(); }
};
