#pragma once

#include <optional>
#include <span>
#include <vector>

#include "practice.hpp"

struct Edge {
    int u = 0;
    int v = 0;
    long long w = 0;
};

// Stage 1
inline bool has_unique_mst(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }

// Stage 2
struct BoruvkaResult {
    std::vector<int> edges;
    int phases = 0;
};
inline BoruvkaResult boruvka(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }

// Stage 3
inline std::vector<int> divide_and_conquer_mst(int n, std::span<const Edge> edges) {
    PRACTICE_TODO();
}
struct DcTrap {
    int n = 0;
    std::vector<Edge> edges;
};
inline DcTrap divide_and_conquer_trap() { PRACTICE_TODO(); }

// Stage 4
inline std::vector<int> cycle_breaking_mst(int n, std::span<const Edge> edges) { PRACTICE_TODO(); }

// Stage 5
class IncrementalMst {
public:
    explicit IncrementalMst(int n) { PRACTICE_TODO(); }

    std::optional<int> add_edge(int u, int v, long long w) { PRACTICE_TODO(); }
    long long weight() const { PRACTICE_TODO(); }
    std::vector<int> edges() const { PRACTICE_TODO(); }
};
