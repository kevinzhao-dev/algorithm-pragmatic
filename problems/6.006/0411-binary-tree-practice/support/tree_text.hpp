// tree_text.hpp — fixed helpers for this exercise: the node type plus a tiny text format
// for writing trees in tests and printing them while debugging.
//
//   leaf with key 5:        (. 5 .)
//   5 with left child 3:    ((. 3 .) 5 .)
//   empty tree:             .
#pragma once

#include <cctype>
#include <cstddef>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

struct TreeNode {
    explicit TreeNode(int k) : key(k) {}
    int key;
    std::unique_ptr<TreeNode> left;
    std::unique_ptr<TreeNode> right;
};

using Tree = std::unique_ptr<TreeNode>;

namespace tree_text_detail {

inline void skip_spaces(std::string_view text, std::size_t& pos) {
    while (pos < text.size() && std::isspace(static_cast<unsigned char>(text[pos])) != 0) {
        ++pos;
    }
}

inline Tree parse_subtree(std::string_view text, std::size_t& pos) {
    skip_spaces(text, pos);
    if (pos >= text.size()) {
        throw std::invalid_argument("unexpected end of tree text");
    }
    if (text[pos] == '.') {
        ++pos;
        return nullptr;
    }
    if (text[pos] != '(') {
        throw std::invalid_argument("expected '(' or '.'");
    }
    ++pos;
    Tree left = parse_subtree(text, pos);
    skip_spaces(text, pos);
    std::size_t consumed = 0;
    const int key = std::stoi(std::string(text.substr(pos)), &consumed);
    pos += consumed;
    Tree right = parse_subtree(text, pos);
    skip_spaces(text, pos);
    if (pos >= text.size() || text[pos] != ')') {
        throw std::invalid_argument("expected ')'");
    }
    ++pos;
    auto node = std::make_unique<TreeNode>(key);
    node->left = std::move(left);
    node->right = std::move(right);
    return node;
}

}  // namespace tree_text_detail

// Parses "(left key right)" with "." for an empty subtree.
inline Tree parse_tree(std::string_view text) {
    std::size_t pos = 0;
    Tree tree = tree_text_detail::parse_subtree(text, pos);
    tree_text_detail::skip_spaces(text, pos);
    if (pos != text.size()) {
        throw std::invalid_argument("trailing text after tree");
    }
    return tree;
}

inline std::string to_text(const TreeNode* node) {
    if (node == nullptr) {
        return ".";
    }
    return "(" + to_text(node->left.get()) + " " + std::to_string(node->key) + " " +
           to_text(node->right.get()) + ")";
}

inline Tree clone_tree(const TreeNode* node) {
    if (node == nullptr) {
        return nullptr;
    }
    auto copy = std::make_unique<TreeNode>(node->key);
    copy->left = clone_tree(node->left.get());
    copy->right = clone_tree(node->right.get());
    return copy;
}
