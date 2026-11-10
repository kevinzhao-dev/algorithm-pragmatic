#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "08_node_pool.hpp"

template <template <typename> class Atomic = std::atomic>
class MpscQueue {
public:
    explicit MpscQueue(std::size_t capacity) : pool_(capacity + 1) {
        head_ = pool_.allocate();
        tail_.store(tagged(head_, 0), std::memory_order_relaxed);
    }

    bool enqueue(std::uint64_t value) {
        const std::uint32_t node = pool_.allocate();
        if (node == practice::kNilIndex) {
            return false;
        }
        pool_[node].value.store(value, std::memory_order_relaxed);
        const std::uint64_t stale = pool_[node].next.load(std::memory_order_relaxed);
        pool_[node].next.store(tagged(practice::kNilIndex, version_of(stale) + 1),
                               std::memory_order_release);

        std::uint64_t tail = 0;
        while (true) {
            tail = tail_.load(std::memory_order_acquire);
            const std::uint64_t next = pool_[index_of(tail)].next.load(std::memory_order_acquire);
            if (tail != tail_.load(std::memory_order_acquire)) {
                continue;
            }
            if (index_of(next) == practice::kNilIndex) {
                std::uint64_t expected = next;
                if (pool_[index_of(tail)].next.compare_exchange_strong(
                        expected, tagged(node, version_of(next) + 1), std::memory_order_acq_rel,
                        std::memory_order_acquire)) {
                    break;
                }
            } else {
                swing_tail(tail, index_of(next));
            }
        }
        swing_tail(tail, node);
        return true;
    }

    std::optional<std::uint64_t> dequeue() {
        const std::uint32_t head = head_;
        const std::uint64_t next = pool_[head].next.load(std::memory_order_acquire);
        if (index_of(next) == practice::kNilIndex) {
            return std::nullopt;
        }
        // The old dummy is about to be freed: the tail must not still be pointing at it.
        std::uint64_t tail = tail_.load(std::memory_order_acquire);
        while (index_of(tail) == head) {
            swing_tail(tail, index_of(next));
            tail = tail_.load(std::memory_order_acquire);
        }
        const std::uint64_t value = pool_[index_of(next)].value.load(std::memory_order_acquire);
        head_ = index_of(next);
        pool_.release(head);
        return value;
    }

private:
    struct Node {
        Atomic<std::uint64_t> next{tagged(practice::kNilIndex, 0)};
        Atomic<std::uint64_t> value{0};
    };

    static constexpr std::uint64_t tagged(std::uint32_t index, std::uint32_t version) {
        return (static_cast<std::uint64_t>(version) << 32) | index;
    }
    static constexpr std::uint32_t index_of(std::uint64_t word) {
        return static_cast<std::uint32_t>(word);
    }
    static constexpr std::uint32_t version_of(std::uint64_t word) {
        return static_cast<std::uint32_t>(word >> 32);
    }

    void swing_tail(std::uint64_t tail, std::uint32_t target) {
        std::uint64_t expected = tail;
        tail_.compare_exchange_strong(expected, tagged(target, version_of(tail) + 1),
                                      std::memory_order_acq_rel, std::memory_order_acquire);
    }

    practice::NodePool<Node> pool_;
    std::uint32_t head_ = practice::kNilIndex;  // only the consumer touches this
    Atomic<std::uint64_t> tail_{};
};
