#pragma once

#include <cstddef>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

struct Topology {
    int feeds;
    int racks;
    std::vector<std::pair<int, int>> links;
};

// Stage 1
struct BackupChoice {
    int feed;
    int racks_covered;
};

inline BackupChoice choose_backup_feed(const Topology& topology) { PRACTICE_TODO(); }

// Stage 2
inline std::vector<int> racks_without_power(const Topology& topology,
                                            std::span<const std::size_t> cut_links) {
    PRACTICE_TODO();
}

// Stage 3
inline std::vector<int> blast_radius(const Topology& topology) { PRACTICE_TODO(); }
