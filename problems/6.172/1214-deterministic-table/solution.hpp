#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <vector>

#include "forkjoin.hpp"
#include "practice.hpp"

template <template <typename> class Atomic = std::atomic>
class DeterministicTable {
public:
    explicit DeterministicTable(std::size_t capacity) { PRACTICE_TODO(); }

    // Stages 1 and 2
    void insert(std::uint64_t key) { PRACTICE_TODO(); }
    std::vector<std::uint64_t> layout() const { PRACTICE_TODO(); }
    std::pair<bool, std::size_t> lookup(std::uint64_t key) const { PRACTICE_TODO(); }
    bool contains(std::uint64_t key) const { PRACTICE_TODO(); }

    // Stage 3
    void insert_all(std::span<const std::uint64_t> keys) { PRACTICE_TODO(); }
};
