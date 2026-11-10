#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <span>
#include <vector>

#include "01_exec.hpp"

inline constexpr std::int64_t kNoEdge = std::numeric_limits<std::int64_t>::max() / 4;

namespace detail_0616 {

// Relaxes all (i, j) through intermediate k. Row k and column k are never written, so reads of
// d[i][k] and d[k][j] cannot race with the writes to other cells.
template <typename Exec, typename OnImprove>
void relax_through(Exec& exec, std::span<std::int64_t> dist, std::size_t n, std::size_t k,
                   OnImprove&& on_improve) {
    exec.parallel_for(0, n, [&](std::size_t i) {
        if (i == k) {
            return;
        }
        exec.parallel_for(0, n, [&, i](std::size_t j) {
            if (j == k) {
                return;
            }
            exec.charge();
            const std::int64_t first = dist[i * n + k];
            const std::int64_t second = dist[k * n + j];
            if (first == kNoEdge || second == kNoEdge) {
                return;
            }
            const std::int64_t candidate = first + second;
            if (candidate < dist[i * n + j]) {
                dist[i * n + j] = candidate;
                on_improve(i, j);
            }
        });
    });
}

}  // namespace detail_0616

template <typename Exec>
void floyd_warshall(Exec& exec, std::span<std::int64_t> dist, std::size_t n) {
    for (std::size_t k = 0; k < n; ++k) {
        detail_0616::relax_through(exec, dist, n, k, [](std::size_t, std::size_t) {});
    }
}

template <typename Exec>
void floyd_warshall_paths(Exec& exec, std::span<std::int64_t> dist, std::span<std::int64_t> next,
                          std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = 0; j < n; ++j) {
            next[i * n + j] =
                (i != j && dist[i * n + j] != kNoEdge) ? static_cast<std::int64_t>(j) : -1;
        }
    }
    for (std::size_t k = 0; k < n; ++k) {
        detail_0616::relax_through(exec, dist, n, k, [&](std::size_t i, std::size_t j) {
            next[i * n + j] = next[i * n + k];
        });
    }
}

inline std::vector<std::size_t> reconstruct_path(std::span<const std::int64_t> next, std::size_t n,
                                                 std::size_t s, std::size_t t) {
    if (s == t) {
        return {s};
    }
    if (next[s * n + t] == -1) {
        return {};
    }
    std::vector<std::size_t> path = {s};
    std::size_t at = s;
    while (at != t) {
        at = static_cast<std::size_t>(next[at * n + t]);
        path.push_back(at);
    }
    return path;
}

inline bool has_negative_cycle(std::span<const std::int64_t> dist, std::size_t n) {
    for (std::size_t i = 0; i < n; ++i) {
        if (dist[i * n + i] < 0) {
            return true;
        }
    }
    return false;
}

template <typename Exec>
void transitive_closure(Exec& exec, std::span<std::uint64_t> rows, std::size_t n) {
    const std::size_t words = (n + 63) / 64;
    for (std::size_t k = 0; k < n; ++k) {
        const std::uint64_t mask = std::uint64_t{1} << (k % 64);
        const std::size_t word_of_k = k / 64;
        const std::uint64_t* const source = rows.data() + k * words;
        exec.parallel_for(0, n, [&, k](std::size_t i) {
            if (i == k) {
                return;  // row k is the source of this round and must stay untouched
            }
            std::uint64_t* const target = rows.data() + i * words;
            if ((target[word_of_k] & mask) != 0) {
                for (std::size_t w = 0; w < words; ++w) {
                    target[w] |= source[w];
                }
            }
        }, 16);
    }
}
