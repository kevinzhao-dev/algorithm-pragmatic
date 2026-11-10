#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../0402-avl-tree/solution.hpp"

template <typename T, typename Aug = NoAugment>
class AvlSequence {
public:
    using Tree = AvlTree<T, Aug>;
    using Node = typename Tree::Node;
    using Value = typename Aug::Value;

    AvlSequence() = default;
    explicit AvlSequence(std::vector<T> items) : tree_(Tree::build(std::move(items))) {}

    std::size_t size() const { return tree_.size(); }
    bool empty() const { return tree_.empty(); }
    const Tree& tree() const { return tree_; }

    const T& at(std::size_t index) const { return node_at(index)->item; }

    void set_at(std::size_t index, T value) {
        Node* node = node_at(index);
        node->item = std::move(value);
        tree_.refresh(node);
    }

    Node* node_at(std::size_t index) const {
        if (index >= size()) {
            throw std::out_of_range("sequence index out of range");
        }
        Node* node = tree_.root();
        while (true) {
            const std::size_t left = subtree_size(node->left);
            if (index < left) {
                node = node->left;
            } else if (index == left) {
                return node;
            } else {
                index -= left + 1;
                node = node->right;
            }
        }
    }

    void insert_at(std::size_t index, T value) {
        if (index > size()) {
            throw std::out_of_range("insert position out of range");
        }
        if (index == size()) {
            tree_.insert_before(nullptr, std::move(value));
        } else {
            tree_.insert_before(node_at(index), std::move(value));
        }
    }

    T delete_at(std::size_t index) { return tree_.erase(node_at(index)); }

    std::vector<T> to_vector() const { return std::vector<T>(tree_.begin(), tree_.end()); }

    Value fold(std::size_t left, std::size_t right) const {
        if (left > right || right > size()) {
            throw std::out_of_range("fold range out of range");
        }
        return fold_range(tree_.root(), left, right);
    }

private:
    static std::size_t subtree_size(const Node* node) { return node != nullptr ? node->size : 0; }

    // Folds [left, right) of this subtree, reading whole-subtree aggregates when it can.
    static Value fold_range(const Node* node, std::size_t left, std::size_t right) {
        if (node == nullptr || left >= right) {
            return Aug::identity();
        }
        if (left == 0 && right == node->size) {
            return node->aug;
        }
        const std::size_t left_size = subtree_size(node->left);
        Value result = Aug::identity();
        if (left < left_size) {
            result = Aug::combine(result, fold_range(node->left, left, std::min(right, left_size)));
        }
        if (left <= left_size && left_size < right) {
            result = Aug::combine(result, Aug::of(node->item));
        }
        if (right > left_size + 1) {
            const std::size_t start = left > left_size + 1 ? left - left_size - 1 : 0;
            result = Aug::combine(result, fold_range(node->right, start, right - left_size - 1));
        }
        return result;
    }

    Tree tree_;
};

class TextBuffer {
public:
    explicit TextBuffer(std::string_view text = "") : buffer_(std::vector<char>(text.begin(), text.end())) {}

    std::size_t size() const { return buffer_.size(); }
    char at(std::size_t index) const { return buffer_.at(index); }
    void insert(std::size_t index, char ch) { buffer_.insert_at(index, ch); }
    void erase(std::size_t index) { buffer_.delete_at(index); }

    std::string substr(std::size_t left, std::size_t right) const {
        if (left > right || right > size()) {
            throw std::out_of_range("substr range out of range");
        }
        std::string out;
        out.reserve(right - left);
        if (left == right) {
            return out;
        }
        for (auto* node = buffer_.node_at(left); out.size() < right - left;
             node = Sequence::Tree::successor(node)) {
            out.push_back(node->item);
        }
        return out;
    }

    bool equal_ranges(std::size_t a, std::size_t b, std::size_t length) const {
        if (a + length > size() || b + length > size()) {
            throw std::out_of_range("range out of range");
        }
        return buffer_.fold(a, a + length) == buffer_.fold(b, b + length);
    }

private:
    // A polynomial hash over the traversal order. Storing base^length alongside the hash keeps
    // combine O(1) and makes the summary a proper monoid.
    struct Hash {
        struct Value {
            std::uint64_t hash = 0;
            std::uint64_t power = 1;
            friend bool operator==(const Value&, const Value&) = default;
        };
        static constexpr std::uint64_t kPrime = (std::uint64_t{1} << 61) - 1;
        static constexpr std::uint64_t kBase = 911382323ULL;

        static std::uint64_t mul(std::uint64_t a, std::uint64_t b) {
            unsigned __int128 product = static_cast<unsigned __int128>(a) * b;
            product = (product & kPrime) + (product >> 61);
            product = (product & kPrime) + (product >> 61);
            return static_cast<std::uint64_t>(product >= kPrime ? product - kPrime : product);
        }
        static std::uint64_t add(std::uint64_t a, std::uint64_t b) {
            const std::uint64_t sum = a + b;
            return sum >= kPrime ? sum - kPrime : sum;
        }

        static Value identity() { return {}; }
        static Value of(char ch) { return {static_cast<std::uint64_t>(ch) + 1, kBase}; }
        static Value combine(const Value& a, const Value& b) {
            return {add(mul(a.hash, b.power), b.hash), mul(a.power, b.power)};
        }
    };

    using Sequence = AvlSequence<char, Hash>;
    Sequence buffer_;
};
