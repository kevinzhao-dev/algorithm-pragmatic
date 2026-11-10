#pragma once

#include <array>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"

struct Graph {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
};

// Stage 1
inline bool is_clique(const Graph& g, std::span<const int> vertices) { PRACTICE_TODO(); }
inline bool is_independent_set(const Graph& g, std::span<const int> vertices) { PRACTICE_TODO(); }
inline bool is_vertex_cover(const Graph& g, std::span<const int> vertices) { PRACTICE_TODO(); }
inline bool is_set_cover(int universe, const std::vector<std::vector<int>>& sets,
                         std::span<const int> chosen) {
    PRACTICE_TODO();
}

// Stage 2
struct CliqueInstance {
    Graph g;
    int k = 0;
};
struct IsInstance {
    Graph g;
    int k = 0;
};
struct VcInstance {
    Graph g;
    int k = 0;
};
struct SetCoverInstance {
    int universe = 0;
    std::vector<std::vector<int>> sets;
    int k = 0;
};
inline IsInstance clique_to_is(const CliqueInstance& src) { PRACTICE_TODO(); }
inline VcInstance is_to_vc(const IsInstance& src) { PRACTICE_TODO(); }
inline SetCoverInstance vc_to_set_cover(const VcInstance& src) { PRACTICE_TODO(); }
inline std::vector<int> lift_is_to_clique(const CliqueInstance& src, std::span<const int> witness) {
    PRACTICE_TODO();
}
inline std::vector<int> lift_vc_to_is(const IsInstance& src, std::span<const int> witness) {
    PRACTICE_TODO();
}
inline std::vector<int> lift_set_cover_to_vc(const VcInstance& src, const SetCoverInstance& dst,
                                             std::span<const int> chosen) {
    PRACTICE_TODO();
}

// Stage 3
struct SubsetSumInstance {
    std::vector<long long> a;
    long long t = 0;
};
struct PartitionInstance {
    std::vector<long long> a;
};
inline PartitionInstance subset_sum_to_partition(const SubsetSumInstance& src) { PRACTICE_TODO(); }
inline std::vector<int> lift_partition_to_subset_sum(const SubsetSumInstance& src,
                                                     const PartitionInstance& dst,
                                                     std::span<const int> side) {
    PRACTICE_TODO();
}

// Stage 4
struct Digraph {
    int n = 0;
    std::vector<std::pair<int, int>> edges;
};
inline Digraph ham_cycle_to_path(const Digraph& g) { PRACTICE_TODO(); }
inline std::vector<int> lift_path_to_cycle(const Digraph& src, const Digraph& dst,
                                           std::span<const int> path) {
    PRACTICE_TODO();
}

// Stage 5
struct ThreeDm {
    int n = 0;
    std::vector<std::array<int, 3>> triples;
};
inline SubsetSumInstance three_dm_to_subset_sum(const ThreeDm& t) { PRACTICE_TODO(); }
inline std::vector<int> lift_subset_sum_to_3dm(const ThreeDm& src, const SubsetSumInstance& dst,
                                               std::span<const int> chosen) {
    PRACTICE_TODO();
}
