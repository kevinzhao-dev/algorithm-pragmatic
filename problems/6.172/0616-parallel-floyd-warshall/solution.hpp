#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "01_exec.hpp"
#include "practice.hpp"

inline constexpr std::int64_t kNoEdge = std::numeric_limits<std::int64_t>::max() / 4;

// Stage 1
template <typename Exec>
void floyd_warshall(Exec& exec, std::span<std::int64_t> dist, std::size_t n) {
    PRACTICE_TODO();
}

// Stage 2
template <typename Exec>
void floyd_warshall_paths(Exec& exec, std::span<std::int64_t> dist, std::span<std::int64_t> next,
                          std::size_t n) {
    PRACTICE_TODO();
}

inline std::vector<std::size_t> reconstruct_path(std::span<const std::int64_t> next, std::size_t n,
                                                 std::size_t s, std::size_t t) {
    PRACTICE_TODO();
}

inline bool has_negative_cycle(std::span<const std::int64_t> dist, std::size_t n) {
    PRACTICE_TODO();
}

// Stage 3
template <typename Exec>
void transitive_closure(Exec& exec, std::span<std::uint64_t> rows, std::size_t n) {
    PRACTICE_TODO();
}
