#pragma once

#include <cstddef>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

#include "practice.hpp"
#include "sha256.hpp"

using Digest = sha256::Digest;

// Stage 1
inline Digest hash_leaf(std::string_view block) { PRACTICE_TODO(); }
inline Digest hash_node(const Digest& left, const Digest& right) { PRACTICE_TODO(); }
inline Digest hash_empty() { PRACTICE_TODO(); }

class MerkleTree {
public:
    explicit MerkleTree(std::vector<std::string> blocks) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    Digest root() const { PRACTICE_TODO(); }

    // Stage 2
    std::vector<Digest> proof(std::size_t index) const { PRACTICE_TODO(); }

    // Stage 3
    void update(std::size_t index, std::string block) { PRACTICE_TODO(); }
};

// Stage 2
inline bool verify_proof(const Digest& root, std::size_t n, std::size_t index,
                         std::string_view block, std::span<const Digest> proof) {
    PRACTICE_TODO();
}

// Stage 4
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
    VerifiedClient(std::size_t n, Digest root) { PRACTICE_TODO(); }

    std::string read(UntrustedServer& server, std::size_t i) { PRACTICE_TODO(); }
    void write(UntrustedServer& server, std::size_t i, std::string block) { PRACTICE_TODO(); }
    const Digest& root() const { PRACTICE_TODO(); }
};
