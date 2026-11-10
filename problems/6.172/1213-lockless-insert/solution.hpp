#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

#include "practice.hpp"

// Stages 1 and 2
template <template <typename> class Atomic = std::atomic>
class LocklessTable {
public:
    explicit LocklessTable(std::size_t capacity) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { PRACTICE_TODO(); }
};

// Stage 3
template <template <typename> class Atomic = std::atomic>
class PublishedTable {
public:
    explicit PublishedTable(std::size_t capacity) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::string text) { PRACTICE_TODO(); }
    std::optional<std::string> find(std::uint64_t key) const { PRACTICE_TODO(); }
};
