#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>
#include <utility>

#include "08_node_pool.hpp"

namespace detail {

template <typename T, template <typename> class Atomic>
struct QueueNode {
    Atomic<std::uint32_t> next{practice::kNilIndex};
    T value{};
};

// The head side is the same in both queues.
template <typename T, typename Mutex, template <typename> class Atomic>
std::optional<T> pop_front(practice::NodePool<QueueNode<T, Atomic>>& pool, std::uint32_t& head,
                           Mutex& head_lock) {
    std::unique_lock lock(head_lock);
    const std::uint32_t node = head;
    const std::uint32_t next = pool[node].next.load(std::memory_order_acquire);
    if (next == practice::kNilIndex) {
        return std::nullopt;
    }
    std::optional<T> value(std::move(pool[next].value));
    head = next;
    lock.unlock();
    pool.release(node);
    return value;
}

}  // namespace detail

template <typename T, typename Mutex = std::mutex, template <typename> class Atomic = std::atomic>
class TwoLockQueue {
public:
    explicit TwoLockQueue(std::size_t capacity) : pool_(capacity + 1) {
        head_ = pool_.allocate();
        tail_ = head_;
    }

    bool enqueue(T value) {
        const std::uint32_t node = pool_.allocate();
        if (node == practice::kNilIndex) {
            return false;
        }
        pool_[node].value = std::move(value);
        pool_[node].next.store(practice::kNilIndex, std::memory_order_relaxed);
        const std::lock_guard lock(tail_lock_);
        pool_[tail_].next.store(node, std::memory_order_release);
        tail_ = node;
        return true;
    }

    std::optional<T> dequeue() { return detail::pop_front<T>(pool_, head_, head_lock_); }

private:
    practice::NodePool<detail::QueueNode<T, Atomic>> pool_;
    std::uint32_t head_ = practice::kNilIndex;
    std::uint32_t tail_ = practice::kNilIndex;
    Mutex head_lock_;
    Mutex tail_lock_;
};

template <typename T, typename Mutex = std::mutex, template <typename> class Atomic = std::atomic>
class SingleEnqueuerQueue {
public:
    explicit SingleEnqueuerQueue(std::size_t capacity) : pool_(capacity + 1) {
        head_ = pool_.allocate();
        tail_ = head_;
    }

    bool enqueue(T value) {
        const std::uint32_t node = pool_.allocate();
        if (node == practice::kNilIndex) {
            return false;
        }
        pool_[node].value = std::move(value);
        pool_[node].next.store(practice::kNilIndex, std::memory_order_relaxed);
        pool_[tail_].next.store(node, std::memory_order_release);
        tail_ = node;  // only this thread reads or writes tail_
        return true;
    }

    std::optional<T> dequeue() { return detail::pop_front<T>(pool_, head_, head_lock_); }

private:
    practice::NodePool<detail::QueueNode<T, Atomic>> pool_;
    std::uint32_t head_ = practice::kNilIndex;
    std::uint32_t tail_ = practice::kNilIndex;
    Mutex head_lock_;
};
