// 08_node_pool.hpp — a fixed array of nodes addressed by 32-bit index, reused first-in first-out.
//
// Stands in for new_node() / free_node() in the data-synchronization exercises. Indices fit in
// half of a 64-bit word, so an index and a version counter can be updated together with one CAS.
// FIFO reuse is deliberate: a node comes back in a different position, which is what exposes ABA.
// The pool's own lock is a plain spin flag: it never appears as a dsched scheduling point, so
// exploration stays focused on the algorithm under test.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <vector>

namespace practice {

inline constexpr std::uint32_t kNilIndex = 0xFFFF'FFFFu;

template <typename Node>
class NodePool {
public:
    explicit NodePool(std::size_t capacity) : nodes_(capacity) {
        for (std::size_t i = 0; i < capacity; ++i) {
            free_.push_back(static_cast<std::uint32_t>(i));
        }
    }

    NodePool(const NodePool&) = delete;
    NodePool& operator=(const NodePool&) = delete;

    // Returns kNilIndex when every node is in use.
    std::uint32_t allocate() {
        const Guard guard(flag_);
        if (free_.empty()) {
            return kNilIndex;
        }
        const std::uint32_t index = free_.front();
        free_.pop_front();
        return index;
    }

    void release(std::uint32_t index) {
        const Guard guard(flag_);
        free_.push_back(index);
    }

    Node& operator[](std::uint32_t index) { return nodes_[index]; }
    const Node& operator[](std::uint32_t index) const { return nodes_[index]; }
    std::size_t capacity() const { return nodes_.size(); }

private:
    struct Guard {
        explicit Guard(std::atomic_flag& flag) : flag_(flag) {
            while (flag_.test_and_set(std::memory_order_acquire)) {
            }
        }
        ~Guard() { flag_.clear(std::memory_order_release); }
        std::atomic_flag& flag_;
    };

    std::vector<Node> nodes_;
    std::deque<std::uint32_t> free_;
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

}  // namespace practice
