#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "sha256.hpp"

using Digest = sha256::Digest;

inline Digest hash_leaf(std::string_view block) {
    std::string buffer;
    buffer.reserve(block.size() + 1);
    buffer.push_back('\x00');
    buffer.append(block);
    return sha256::hash(buffer);
}

inline Digest hash_node(const Digest& left, const Digest& right) {
    std::string buffer;
    buffer.reserve(65);
    buffer.push_back('\x01');
    buffer.append(reinterpret_cast<const char*>(left.data()), left.size());
    buffer.append(reinterpret_cast<const char*>(right.data()), right.size());
    return sha256::hash(buffer);
}

inline Digest hash_empty() {
    const char tag = '\x02';
    return sha256::hash(std::string_view(&tag, 1));
}

class MerkleTree {
public:
    explicit MerkleTree(std::vector<std::string> blocks)
        : blocks_(std::move(blocks)), n_(blocks_.size()) {
        cap_ = 1;
        while (cap_ < n_) {
            cap_ *= 2;
        }
        tree_.assign(2 * cap_, Digest{});
        const Digest empty = hash_empty();
        for (std::size_t i = 0; i < cap_; ++i) {
            tree_[cap_ + i] = i < n_ ? hash_leaf(blocks_[i]) : empty;
        }
        for (std::size_t i = cap_ - 1; i >= 1; --i) {
            tree_[i] = hash_node(tree_[2 * i], tree_[2 * i + 1]);
        }
    }

    std::size_t size() const { return n_; }
    Digest root() const { return tree_[1]; }

    std::vector<Digest> proof(std::size_t index) const {
        std::vector<Digest> out;
        for (std::size_t pos = cap_ + index; pos > 1; pos /= 2) {
            out.push_back(tree_[pos ^ 1]);
        }
        return out;
    }

    void update(std::size_t index, std::string block) {
        blocks_[index] = std::move(block);
        std::size_t pos = cap_ + index;
        tree_[pos] = hash_leaf(blocks_[index]);
        for (pos /= 2; pos >= 1; pos /= 2) {
            tree_[pos] = hash_node(tree_[2 * pos], tree_[2 * pos + 1]);
        }
    }

private:
    std::vector<std::string> blocks_;
    std::size_t n_;
    std::size_t cap_ = 1;
    std::vector<Digest> tree_;  // heap layout, root at 1, leaves at cap_..2cap_-1
};

namespace merkle_detail {

inline Digest fold_path(Digest current, std::size_t index, std::span<const Digest> proof) {
    for (const Digest& sibling : proof) {
        current = (index & 1) ? hash_node(sibling, current) : hash_node(current, sibling);
        index /= 2;
    }
    return current;
}

inline std::size_t expected_depth(std::size_t n) {
    std::size_t cap = 1;
    std::size_t depth = 0;
    while (cap < n) {
        cap *= 2;
        ++depth;
    }
    return depth;
}

}  // namespace merkle_detail

inline bool verify_proof(const Digest& root, std::size_t n, std::size_t index,
                         std::string_view block, std::span<const Digest> proof) {
    if (index >= n || proof.size() != merkle_detail::expected_depth(n)) {
        return false;
    }
    return merkle_detail::fold_path(hash_leaf(block), index, proof) == root;
}

class TamperDetected : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

class UntrustedServer {
public:
    virtual ~UntrustedServer() = default;
    virtual std::string get_block(std::size_t i) const = 0;
    virtual std::vector<Digest> get_proof(std::size_t i) const = 0;
    virtual void put_block(std::size_t i, std::string block) = 0;
};

class VerifiedClient {
public:
    VerifiedClient(std::size_t n, Digest root) : n_(n), root_(root) {}

    std::string read(UntrustedServer& server, std::size_t i) {
        std::string block = server.get_block(i);
        const auto proof = server.get_proof(i);
        if (!verify_proof(root_, n_, i, block, proof)) {
            throw TamperDetected("block does not match the trusted root");
        }
        return block;
    }

    void write(UntrustedServer& server, std::size_t i, std::string block) {
        const std::string old_block = server.get_block(i);
        const auto proof = server.get_proof(i);
        if (!verify_proof(root_, n_, i, old_block, proof)) {
            throw TamperDetected("cannot verify the existing block");
        }
        root_ = merkle_detail::fold_path(hash_leaf(block), i, proof);
        server.put_block(i, std::move(block));
    }

    const Digest& root() const { return root_; }

private:
    std::size_t n_;
    Digest root_;
};
