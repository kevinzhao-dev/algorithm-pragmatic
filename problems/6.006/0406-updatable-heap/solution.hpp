#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "practice.hpp"

template <typename T, typename Compare = std::less<T>>
class UpdatableHeap {
public:
    class Handle {
    public:
        Handle() = default;
        friend bool operator==(const Handle&, const Handle&) { PRACTICE_TODO(); }
    };

    // Stage 1
    explicit UpdatableHeap(Compare compare = Compare{}) { PRACTICE_TODO(); }

    Handle push(T value) { PRACTICE_TODO(); }
    const T& top() const { PRACTICE_TODO(); }
    Handle top_handle() const { PRACTICE_TODO(); }
    T pop() { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    bool contains(Handle handle) const { PRACTICE_TODO(); }
    const T& get(Handle handle) const { PRACTICE_TODO(); }

    // Stage 2
    void update(Handle handle, T value) { PRACTICE_TODO(); }

    // Stage 3
    T erase(Handle handle) { PRACTICE_TODO(); }
    std::vector<Handle> build(std::vector<T> items) { PRACTICE_TODO(); }
};

// Stage 4
class DeadlineScheduler {
public:
    bool submit(const std::string& name, std::int64_t deadline) { PRACTICE_TODO(); }
    bool reschedule(const std::string& name, std::int64_t deadline) { PRACTICE_TODO(); }
    bool cancel(const std::string& name) { PRACTICE_TODO(); }
    std::optional<std::string> next() { PRACTICE_TODO(); }
    std::optional<std::string> peek() const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
