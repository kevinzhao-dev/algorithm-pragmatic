#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "08_node_pool.hpp"
#include "practice.hpp"

template <template <typename> class Atomic = std::atomic>
class LockFreeStack {
public:
    explicit LockFreeStack(std::size_t capacity) { PRACTICE_TODO(); }

    bool push(std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> pop() { PRACTICE_TODO(); }
};
