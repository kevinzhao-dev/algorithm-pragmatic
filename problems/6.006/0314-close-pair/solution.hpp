#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <utility>

#include "practice.hpp"

using IndexPair = std::pair<std::size_t, std::size_t>;

// Stage 1
inline std::optional<IndexPair> close_pair_expected(std::span<const std::uint64_t> b, std::uint64_t r) {
    PRACTICE_TODO();
}

// Stage 2
inline std::optional<IndexPair> close_pair_worst_case(std::span<const std::uint64_t> b, std::uint64_t r) {
    PRACTICE_TODO();
}

// Stage 3
class WindowPairMatcher {
public:
    WindowPairMatcher(std::size_t window, std::uint64_t target) { PRACTICE_TODO(); }

    std::optional<std::size_t> offer(std::uint64_t value) { PRACTICE_TODO(); }
    std::size_t tracked() const { PRACTICE_TODO(); }
};
