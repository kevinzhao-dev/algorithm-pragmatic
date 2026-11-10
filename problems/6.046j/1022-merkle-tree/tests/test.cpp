#include <algorithm>
#include <functional>
#include <string>
#include <vector>

#include "practice.hpp"
#include "sha256.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::string hex(const Digest& d) {
    static const char* digits = "0123456789abcdef";
    std::string out;
    for (unsigned char byte : d) {
        out.push_back(digits[byte >> 4]);
        out.push_back(digits[byte & 15]);
    }
    return out;
}

// Reference Merkle root following the README's rules.
Digest reference_root(const std::vector<std::string>& blocks) {
    const std::size_t n = blocks.size();
    std::size_t cap = 1;
    while (cap < n) {
        cap *= 2;
    }
    std::vector<Digest> level(cap);
    for (std::size_t i = 0; i < cap; ++i) {
        if (i < n) {
            std::string data(1, '\x00');
            data += blocks[i];
            level[i] = sha256::hash(data);
        } else {
            const char tag = '\x02';
            level[i] = sha256::hash(std::string_view(&tag, 1));
        }
    }
    while (level.size() > 1) {
        std::vector<Digest> next;
        for (std::size_t i = 0; i < level.size(); i += 2) {
            std::string data(1, '\x01');
            data.append(reinterpret_cast<const char*>(level[i].data()), 32);
            data.append(reinterpret_cast<const char*>(level[i + 1].data()), 32);
            next.push_back(sha256::hash(data));
        }
        level = std::move(next);
    }
    return level[0];
}

std::vector<std::string> random_blocks(Rng& rng, int n) {
    std::vector<std::string> blocks(n);
    for (auto& block : blocks) {
        block = rng.text(static_cast<std::size_t>(rng.uniform(0, 80)));
    }
    return blocks;
}

class HonestServer final : public UntrustedServer {
public:
    explicit HonestServer(std::vector<std::string> blocks)
        : blocks_(blocks), tree_(std::move(blocks)) {}

    std::string get_block(std::size_t i) const override { return blocks_[i]; }
    std::vector<Digest> get_proof(std::size_t i) const override { return tree_.proof(i); }
    void put_block(std::size_t i, std::string block) override {
        blocks_[i] = block;
        tree_.update(i, std::move(block));
    }

    const MerkleTree& tree() const { return tree_; }
    std::vector<std::string>& blocks() { return blocks_; }

private:
    std::vector<std::string> blocks_;
    MerkleTree tree_;
};

// Serves whatever it is told, regardless of the truth.
class ForgingServer final : public UntrustedServer {
public:
    std::string block;
    std::vector<Digest> proof_data;
    std::string get_block(std::size_t) const override { return block; }
    std::vector<Digest> get_proof(std::size_t) const override { return proof_data; }
    void put_block(std::size_t, std::string b) override { block = std::move(b); }
};

}  // namespace

TEST(1, "sha256 support matches known digests") {
    CHECK_EQ(hex(sha256::hash("")),
             "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
    CHECK_EQ(hex(sha256::hash("abc")),
             "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
    const std::string million(1'000'000, 'a');
    CHECK_EQ(hex(sha256::hash(million)),
             "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(1, "domain-separated hashes follow the specification") {
    std::string leaf(1, '\x00');
    leaf += "hello";
    CHECK_EQ(hash_leaf("hello"), sha256::hash(leaf));
    const char tag = '\x02';
    CHECK_EQ(hash_empty(), sha256::hash(std::string_view(&tag, 1)));
    const Digest a = hash_leaf("a");
    const Digest b = hash_leaf("b");
    std::string node(1, '\x01');
    node.append(reinterpret_cast<const char*>(a.data()), 32);
    node.append(reinterpret_cast<const char*>(b.data()), 32);
    CHECK_EQ(hash_node(a, b), sha256::hash(node));
    CHECK(hash_node(a, b) != hash_node(b, a));
}

TEST(1, "a single block has a root that is its leaf hash") {
    MerkleTree tree({"only"});
    CHECK_EQ(tree.size(), 1);
    CHECK_EQ(tree.root(), hash_leaf("only"));
}

TEST(1, "roots match the reference for all small sizes") {
    Rng rng;
    for (int n = 1; n <= 40; ++n) {
        for (int trial = 0; trial < 5; ++trial) {
            const auto blocks = random_blocks(rng, n);
            MerkleTree tree(blocks);
            INFO("n=", n);
            CHECK_EQ(tree.root(), reference_root(blocks));
            CHECK_EQ(tree.size(), n);
        }
    }
}

TEST(1, "blocks are bound to their position") {
    MerkleTree one({"x", "y"});
    MerkleTree two({"y", "x"});
    CHECK(one.root() != two.root());
}

TEST(2, "every leaf has a valid proof") {
    Rng rng;
    for (int n : {1, 2, 3, 5, 8, 17, 64, 100}) {
        const auto blocks = random_blocks(rng, n);
        MerkleTree tree(blocks);
        const Digest root = tree.root();
        std::size_t cap = 1, depth = 0;
        while (cap < static_cast<std::size_t>(n)) {
            cap *= 2;
            ++depth;
        }
        for (int i = 0; i < n; ++i) {
            const auto proof = tree.proof(i);
            INFO("n=", n, " i=", i);
            CHECK_EQ(proof.size(), depth);
            CHECK(verify_proof(root, n, i, blocks[i], proof));
        }
    }
}

TEST(2, "tampered data or proofs are rejected") {
    Rng rng;
    const auto blocks = random_blocks(rng, 20);
    MerkleTree tree(blocks);
    const Digest root = tree.root();
    for (int i = 0; i < 20; ++i) {
        auto proof = tree.proof(i);
        INFO("i=", i);
        CHECK(!verify_proof(root, 20, i, blocks[i] + "!", proof));
        CHECK(!verify_proof(root, 20, (i + 1) % 20, blocks[i], proof));
        proof[static_cast<std::size_t>(rng.uniform(0, static_cast<int>(proof.size()) - 1))][3] ^= 1;
        CHECK(!verify_proof(root, 20, i, blocks[i], proof));
    }
}

TEST(2, "a proof of the wrong length is rejected") {
    MerkleTree tree({"a", "b", "c", "d"});
    const Digest root = tree.root();
    auto proof = tree.proof(1);
    CHECK(!verify_proof(root, 4, 1, "b", std::span<const Digest>(proof).subspan(1)));
    proof.push_back(Digest{});
    CHECK(!verify_proof(root, 4, 1, "b", proof));
}

TEST(2, "an index outside the list is rejected") {
    MerkleTree tree({"a", "b", "c"});
    CHECK(!verify_proof(tree.root(), 3, 3, "c", tree.proof(2)));
    CHECK(!verify_proof(tree.root(), 3, 100, "c", tree.proof(2)));
}

TEST(2, "an inner node cannot be presented as a leaf") {
    MerkleTree tree({"a", "b", "c", "d"});
    const auto proof = tree.proof(0);
    const Digest left = hash_leaf("a");
    const Digest right = hash_leaf("b");
    std::string fake(reinterpret_cast<const char*>(left.data()), 32);
    fake.append(reinterpret_cast<const char*>(right.data()), 32);
    // Claim the node over leaves 0-1 is a data block at index 0 of a two-leaf tree.
    CHECK(!verify_proof(tree.root(), 2, 0, fake, std::span<const Digest>(proof).subspan(1)));
    CHECK(!verify_proof(tree.root(), 4, 0, fake, std::span<const Digest>(proof).subspan(1)));
}

TEST(3, "updates change the root to the reference value") {
    Rng rng;
    for (int n : {1, 3, 8, 21}) {
        auto blocks = random_blocks(rng, n);
        MerkleTree tree(blocks);
        for (int step = 0; step < 50; ++step) {
            const int i = static_cast<int>(rng.uniform(0, n - 1));
            blocks[i] = rng.text(static_cast<std::size_t>(rng.uniform(0, 30)));
            tree.update(i, blocks[i]);
            INFO("n=", n, " step ", step);
            CHECK_EQ(tree.root(), reference_root(blocks));
            CHECK(verify_proof(tree.root(), n, i, blocks[i], tree.proof(i)));
        }
    }
}

TEST(3, "an update recomputes only one path") {
    for (std::size_t n : {std::size_t{2}, std::size_t{1000}, std::size_t{1} << 16}) {
        MerkleTree tree(std::vector<std::string>(n, "x"));
        std::size_t depth = 0;
        for (std::size_t cap = 1; cap < n; cap *= 2) {
            ++depth;
        }
        const auto before = sha256::calls();
        tree.update(n / 2, "changed");
        INFO("n=", n);
        CHECK_LE(sha256::calls() - before, static_cast<std::int64_t>(depth) + 2);
    }
}

TEST(4, "an honest server round-trips reads and writes") {
    Rng rng;
    auto blocks = random_blocks(rng, 37);
    HonestServer server(blocks);
    VerifiedClient client(37, server.tree().root());
    for (int step = 0; step < 300; ++step) {
        const auto i = static_cast<std::size_t>(rng.uniform(0, 36));
        INFO("step ", step);
        if (rng.coin()) {
            CHECK_EQ(client.read(server, i), blocks[i]);
        } else {
            blocks[i] = rng.text(5);
            client.write(server, i, blocks[i]);
            CHECK_EQ(client.root(), server.tree().root());
        }
    }
}

TEST(4, "a tampered block is detected on read") {
    auto blocks = std::vector<std::string>{"a", "b", "c"};
    HonestServer server(blocks);
    VerifiedClient client(3, server.tree().root());
    server.blocks()[1] = "evil";
    CHECK_THROWS_AS(client.read(server, 1), TamperDetected);
}

TEST(4, "replaying an old version is detected (freshness)") {
    auto blocks = std::vector<std::string>{"a", "b", "c", "d"};
    HonestServer server(blocks);
    VerifiedClient client(4, server.tree().root());
    // Attacker saves the consistent old pair for block 2.
    const std::string old_block = server.get_block(2);
    const auto old_proof = server.get_proof(2);
    client.write(server, 2, "new");
    CHECK_EQ(client.read(server, 2), "new");

    ForgingServer forger;
    forger.block = old_block;
    forger.proof_data = old_proof;
    CHECK_THROWS_AS(client.read(forger, 2), TamperDetected);
    CHECK_THROWS_AS(client.write(forger, 2, "whatever"), TamperDetected);
    CHECK_EQ(client.root(), server.tree().root());  // a failed write must not move the root
}

TEST(4, "writes verify the old block before trusting the proof") {
    auto blocks = std::vector<std::string>{"a", "b", "c", "d"};
    HonestServer server(blocks);
    VerifiedClient client(4, server.tree().root());
    const Digest root_before = client.root();
    ForgingServer forger;
    forger.block = "a";
    forger.proof_data = server.get_proof(0);
    forger.proof_data[0][0] ^= 1;  // proof corrupted
    CHECK_THROWS_AS(client.write(forger, 0, "x"), TamperDetected);
    CHECK_EQ(client.root(), root_before);
}

TEST(4, "operations use a logarithmic number of hashes") {
    Rng rng;
    const std::size_t n = 1 << 12;
    HonestServer server(std::vector<std::string>(n, "payload"));
    VerifiedClient client(n, server.tree().root());
    const std::size_t depth = 12;
    for (int step = 0; step < 200; ++step) {
        const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<int>(n) - 1));
        auto before = sha256::calls();
        client.read(server, i);
        INFO("step ", step);
        CHECK_LE(sha256::calls() - before, static_cast<std::int64_t>(depth) + 2);
        before = sha256::calls();
        client.write(server, i, "updated");
        // write: verify (~depth+1) + new root (~depth+1) + server's own update (~depth+1)
        CHECK_LE(sha256::calls() - before, 3 * static_cast<std::int64_t>(depth) + 6);
    }
}

PERF(3, "large tree build and thousands of updates", 2500) {
    Rng rng;
    const std::size_t n = 1 << 16;
    MerkleTree tree(std::vector<std::string>(n, "block"));
    for (int step = 0; step < 50'000; ++step) {
        tree.update(static_cast<std::size_t>(rng.uniform(0, static_cast<int>(n) - 1)), "next");
    }
    CHECK(tree.root() != Digest{});
}
