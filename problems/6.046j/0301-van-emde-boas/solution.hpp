#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "practice.hpp"

class VanEmdeBoas {
public:
    // Stage 1
    explicit VanEmdeBoas(int universe_bits) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::optional<std::uint64_t> min() const { PRACTICE_TODO(); }
    std::optional<std::uint64_t> max() const { PRACTICE_TODO(); }
    bool contains(std::uint64_t x) const { PRACTICE_TODO(); }
    bool insert(std::uint64_t x) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> successor(std::uint64_t x) const { PRACTICE_TODO(); }

    // Stage 2
    bool erase(std::uint64_t x) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> predecessor(std::uint64_t x) const { PRACTICE_TODO(); }

    // Stage 3
    std::size_t node_count() const { PRACTICE_TODO(); }

    // Cost model used by the tests: the number of vEB nodes entered so far.
    std::uint64_t visits() const { PRACTICE_TODO(); }
};

// Stage 4
class RouteTable {
public:
    bool add(std::uint32_t first, std::uint32_t last, int port) { PRACTICE_TODO(); }
    bool remove(std::uint32_t first) { PRACTICE_TODO(); }
    std::optional<int> lookup(std::uint32_t ip) const { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
