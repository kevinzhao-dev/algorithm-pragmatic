#pragma once

#include <atomic>
#include <cstddef>
#include <mutex>
#include <optional>

#include "08_node_pool.hpp"
#include "practice.hpp"

// Stages 1 and 2
template <typename T, typename Mutex = std::mutex, template <typename> class Atomic = std::atomic>
class TwoLockQueue {
public:
    explicit TwoLockQueue(std::size_t capacity) { PRACTICE_TODO(); }

    bool enqueue(T value) { PRACTICE_TODO(); }
    std::optional<T> dequeue() { PRACTICE_TODO(); }
};

// Stage 3: at most one thread ever calls enqueue.
template <typename T, typename Mutex = std::mutex, template <typename> class Atomic = std::atomic>
class SingleEnqueuerQueue {
public:
    explicit SingleEnqueuerQueue(std::size_t capacity) { PRACTICE_TODO(); }

    bool enqueue(T value) { PRACTICE_TODO(); }
    std::optional<T> dequeue() { PRACTICE_TODO(); }
};
