#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "practice.hpp"

// How many high-order bits become the cluster number when a level splits `bits` bits.
struct SqrtSplit {
    static constexpr int high_bits(int bits) { return bits - bits / 2; }
};

struct CubeRootSplit {
    static constexpr int high_bits(int bits) { return std::max(1, bits / 3); }
};

template <typename Split, bool ExcludeMax>
class Veb {
public:
    explicit Veb(int universe_bits) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    bool empty() const { PRACTICE_TODO(); }
    std::optional<std::uint64_t> min() const { PRACTICE_TODO(); }
    std::optional<std::uint64_t> max() const { PRACTICE_TODO(); }
    bool contains(std::uint64_t x) const { PRACTICE_TODO(); }
    bool insert(std::uint64_t x) { PRACTICE_TODO(); }
    bool erase(std::uint64_t x) { PRACTICE_TODO(); }
    std::optional<std::uint64_t> successor(std::uint64_t x) const { PRACTICE_TODO(); }
    std::optional<std::uint64_t> predecessor(std::uint64_t x) const { PRACTICE_TODO(); }
    std::size_t node_count() const { PRACTICE_TODO(); }

    // Cost model used by the tests: the number of vEB nodes entered so far.
    std::uint64_t visits() const { PRACTICE_TODO(); }
};
