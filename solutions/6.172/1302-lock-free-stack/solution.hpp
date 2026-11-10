#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "08_node_pool.hpp"

template <template <typename> class Atomic = std::atomic>
class LockFreeStack {
public:
    explicit LockFreeStack(std::size_t capacity) : pool_(capacity) {}

    bool push(std::uint64_t value) {
        const std::uint32_t node = pool_.allocate();
        if (node == practice::kNilIndex) {
            return false;
        }
        pool_[node].value = value;
        std::uint64_t head = head_.load(std::memory_order_acquire);
        while (true) {
            pool_[node].next.store(index_of(head), std::memory_order_release);
            if (head_.compare_exchange_weak(head, tagged(node, version_of(head) + 1),
                                            std::memory_order_acq_rel, std::memory_order_acquire)) {
                return true;
            }
        }
    }

    std::optional<std::uint64_t> pop() {
        std::uint64_t head = head_.load(std::memory_order_acquire);
        while (true) {
            const std::uint32_t node = index_of(head);
            if (node == practice::kNilIndex) {
                return std::nullopt;
            }
            // This node may already belong to someone else, so `next` must be atomic.
            const std::uint32_t next = pool_[node].next.load(std::memory_order_acquire);
            if (head_.compare_exchange_weak(head, tagged(next, version_of(head) + 1),
                                            std::memory_order_acq_rel, std::memory_order_acquire)) {
                const std::uint64_t value = pool_[node].value;
                pool_.release(node);
                return value;
            }
        }
    }

private:
    struct Node {
        Atomic<std::uint32_t> next{practice::kNilIndex};
        std::uint64_t value = 0;
    };

    static std::uint64_t tagged(std::uint32_t index, std::uint32_t version) {
        return (static_cast<std::uint64_t>(version) << 32) | index;
    }
    static std::uint32_t index_of(std::uint64_t word) { return static_cast<std::uint32_t>(word); }
    static std::uint32_t version_of(std::uint64_t word) {
        return static_cast<std::uint32_t>(word >> 32);
    }

    practice::NodePool<Node> pool_;
    Atomic<std::uint64_t> head_{tagged(practice::kNilIndex, 0)};
};
