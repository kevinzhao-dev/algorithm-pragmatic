#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "practice.hpp"

inline constexpr std::uint64_t kHashPrime = (std::uint64_t{1} << 61) - 1;

// Stages 1 and 2
template <typename V>
class PerfectHashMap {
public:
    using Item = std::pair<std::uint64_t, V>;

    explicit PerfectHashMap(std::vector<Item> items, std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    std::size_t total_slots() const { PRACTICE_TODO(); }

    // Stage 2
    const V* find(std::uint64_t key) const { PRACTICE_TODO(); }
    bool contains(std::uint64_t key) const { PRACTICE_TODO(); }

    // Cost model used by the tests: the number of table slots examined so far.
    std::uint64_t probes() const { PRACTICE_TODO(); }
};

// Stage 3
template <typename V>
class StaticStringMap {
public:
    using Item = std::pair<std::string, V>;

    explicit StaticStringMap(std::vector<Item> items, std::uint64_t seed = 6046) { PRACTICE_TODO(); }

    std::size_t size() const { PRACTICE_TODO(); }
    const V* find(std::string_view key) const { PRACTICE_TODO(); }
};
