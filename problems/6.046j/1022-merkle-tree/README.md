---
title: Merkle Tree (verified storage and freshness)
unit: 10 · Distributed Algorithms (PS10)
source: 6.046J Recitation 11, Lecture 21
pdf: materials/6.046j/recitations/rec11.pdf
read: materials/6.046j/recitations/rec11.pdf, materials/6.046j/lectures/lec21.pdf
kind: code
minutes: 110
---

# 1022 · Merkle Tree

**Based on:** [Cryptography: More Primitives](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/c9057a2600a14987be5eea240c10fe45_MIT6_046JS15_Recitation11.pdf) · **Read first:** [Cryptography: More Primitives](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/c9057a2600a14987be5eea240c10fe45_MIT6_046JS15_Recitation11.pdf), [Cryptography: Hash Functions](https://ocw.mit.edu/courses/6-046j-design-and-analysis-of-algorithms-spring-2015/6741d65be662edac7c169c4081b3bd9a_MIT6_046JS15_lec21.pdf)

> **Why this comes up in practice:** when you keep data in untrusted storage (S3, a remote disk, a blockchain node), you need to verify that the content hasn't been tampered with and **is not an old version**. Certificate Transparency, Bitcoin's SPV, and dm-verity all use the same structure: locally you store just one root hash.

## Your task

`support/sha256.hpp` (provided) has `sha256::hash(std::string_view)` and `sha256::Digest` (`std::array<unsigned char, 32>`). `sha256::calls()` counts the calls, and the tests use it to confirm that an update really is O(log n).

Following Recitation 11 and Lecture 21, you build a Merkle tree with membership proofs and logarithmic updates, then a client that reads and writes through a server it doesn't trust.

Write your code in `solution.hpp`. Each stage lists the declarations it adds, copied from that file.

Read one stage at a time.

## Stage 1 — Building the tree

```cpp
using Digest = sha256::Digest;

Digest hash_leaf(std::string_view block);
Digest hash_node(const Digest& left, const Digest& right);
Digest hash_empty();

class MerkleTree {
public:
    explicit MerkleTree(std::vector<std::string> blocks);

    std::size_t size() const;
    Digest root() const;
};
```

The specification (follow it exactly; the tests compare roots):

- `Digest hash_leaf(std::string_view block)`: `SHA256(0x00 || block)`.
- `Digest hash_node(const Digest& l, const Digest& r)`: `SHA256(0x01 || l || r)`.
- `Digest hash_empty()`: `SHA256(0x02)`.

`MerkleTree(std::vector<std::string> blocks)`: `n = blocks.size() ≥ 1`. Pad the leaves up to `cap` (the smallest power of 2 that is `≥ n`), using `hash_empty()` for the extra leaves. `Digest root() const` and `std::size_t size() const` (the original `n`).

## Stage 2 — Membership proofs

```cpp
bool verify_proof(const Digest& root, std::size_t n, std::size_t index,
                  std::string_view block, std::span<const Digest> proof);

// in class MerkleTree
std::vector<Digest> proof(std::size_t index) const;
```

`std::vector<Digest> proof(std::size_t index) const`: the siblings along the path from the leaf to the root (length `⌈lg cap⌉`, and length 0 when `cap = 1`).

`bool verify_proof(const Digest& root, std::size_t n, std::size_t index, std::string_view block, std::span<const Digest> proof)`: a pure function that doesn't need the whole tree. It must return `false` when `index >= n` or the proof has the wrong length.

## Stage 3 — Updates

```cpp
// in class MerkleTree
void update(std::size_t index, std::string block);
```

`void update(std::size_t index, std::string block)`: recompute only the nodes on the path. Each call makes at most `lg cap + 2` calls to `sha256::hash`.

## Stage 4 — Real-world extension: reading and writing on an untrusted server

```cpp
class UntrustedServer {   // the tests provide implementations, which may lie
public:
    virtual ~UntrustedServer() = default;
    virtual std::string get_block(std::size_t i) const = 0;
    virtual std::vector<Digest> get_proof(std::size_t i) const = 0;
    virtual void put_block(std::size_t i, std::string block) = 0;
};

class VerifiedClient {
public:
    VerifiedClient(std::size_t n, Digest root);
    std::string read(UntrustedServer& server, std::size_t i);
    void write(UntrustedServer& server, std::size_t i, std::string block);
    const Digest& root() const;
};
```

The client trusts only the root it holds (O(1) trusted storage). `read` must verify the block and proof that the server returns, and throw `TamperDetected` otherwise. `write` must also first verify the proof of the old block, compute the new root from that same proof, and only then hand the new block to the server; if verification fails, `write` also throws `TamperDetected`, and the root must not change.

The tests also count hashes: with n = 4096, a `read` may make at most ⌈lg n⌉ + 2 calls to `sha256::hash` and a `write` at most 3⌈lg n⌉ + 6. These counts include the honest server's calls to your own `MerkleTree::proof` and `update`, so `proof()` must return stored digests without hashing, and `write` verifies only once.

(This solves the freshness problem from the recitation: an attacker who replays an old version won't match the new root.)

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Why do leaves and internal nodes use different prefixes (domain separation)? What attack becomes possible without them?
2. Why tie the proof length to `n`? What happens if the verifier doesn't know `n`?
3. Compare a Merkle tree with storing n hashes directly or storing one big hash: what are the trusted storage, verification time, and update time of each of the three approaches?
