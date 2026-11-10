#pragma once

#include <cstddef>
#include <cstdint>
#include <mutex>
#include <optional>

#include "practice.hpp"

// Stage 1
template <typename Mutex = std::mutex>
class CoarseTable {
public:
    explicit CoarseTable(std::size_t capacity) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { PRACTICE_TODO(); }
};

// Stage 2
template <typename Mutex = std::mutex>
class StripedTable {
public:
    StripedTable(std::size_t capacity, std::size_t stripes) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { PRACTICE_TODO(); }
    const void* stripe_address(std::size_t stripe) const { PRACTICE_TODO(); }
};

// Stage 3: the deadlock-prone hand-over-hand variant, on purpose.
template <typename Mutex = std::mutex>
class FairTable {
public:
    FairTable(std::size_t capacity, std::size_t stripes) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { PRACTICE_TODO(); }
};

// Stage 4
template <typename Mutex = std::mutex>
class OrderedFairTable {
public:
    OrderedFairTable(std::size_t capacity, std::size_t stripes) { PRACTICE_TODO(); }

    bool insert(std::uint64_t key, std::uint64_t value) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> find(std::uint64_t key) const { PRACTICE_TODO(); }
};
