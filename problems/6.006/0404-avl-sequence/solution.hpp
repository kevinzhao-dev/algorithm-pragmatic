#pragma once

#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "../0402-avl-tree/solution.hpp"
#include "practice.hpp"

template <typename T, typename Aug = NoAugment>
class AvlSequence {
public:
    using Tree = AvlTree<T, Aug>;
    using Node = typename Tree::Node;
    using Value = typename Aug::Value;

    // Stage 1
    AvlSequence() = default;
    explicit AvlSequence(std::vector<T> items) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    const Tree& tree() const { PRACTICE_TODO(); }
    Node* node_at(std::size_t index) const { PRACTICE_TODO(); }
    const T& at(std::size_t index) const { PRACTICE_TODO(); }
    void set_at(std::size_t index, T value) { PRACTICE_TODO(); }
    std::vector<T> to_vector() const { PRACTICE_TODO(); }

    // Stage 2
    void insert_at(std::size_t index, T value) { PRACTICE_TODO(); }
    T delete_at(std::size_t index) { PRACTICE_TODO(); }

    // Stage 3
    Value fold(std::size_t left, std::size_t right) const { PRACTICE_TODO(); }
};

// Stage 4
class TextBuffer {
public:
    explicit TextBuffer(std::string_view text = "") { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    char at(std::size_t index) const { PRACTICE_TODO(); }
    void insert(std::size_t index, char ch) { PRACTICE_TODO(); }
    void erase(std::size_t index) { PRACTICE_TODO(); }
    std::string substr(std::size_t left, std::size_t right) const { PRACTICE_TODO(); }
    bool equal_ranges(std::size_t a, std::size_t b, std::size_t length) const { PRACTICE_TODO(); }
};
