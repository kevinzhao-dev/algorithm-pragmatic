#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct VebNode {
    long long key = 0;
    int left = -1;
    int right = -1;
};

// Stage 1
inline std::vector<VebNode> build_veb_layout(std::span<const long long> sorted_keys) {
    PRACTICE_TODO();
}

// Stage 2
inline int search_blocks(std::span<const VebNode> nodes, long long key, std::size_t block_bytes) {
    PRACTICE_TODO();
}
inline std::vector<VebNode> build_bfs_layout(std::span<const long long> sorted_keys) {
    PRACTICE_TODO();
}

// Stage 3
class VebIndex {
public:
    explicit VebIndex(std::vector<long long> sorted_keys) { PRACTICE_TODO(); }

    bool contains(long long key) const { PRACTICE_TODO(); }
    std::optional<long long> predecessor(long long key) const { PRACTICE_TODO(); }
};
