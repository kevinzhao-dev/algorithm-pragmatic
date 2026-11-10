// search.hpp — Stages 2–5: minimax, alpha-beta, table, ordering, fork-join.
#pragma once

#include <cstdint>

#include "board.hpp"
#include "movegen.hpp"
#include "practice.hpp"
#include "tt.hpp"

struct SearchOptions {
    bool use_tt = false;
    bool use_ordering = false;
    int tt_bits = 16;
};

struct SearchStats {
    std::uint64_t nodes = 0;               // positions visited, including the root and leaves
    std::uint64_t cutoffs = 0;             // nodes that stopped early on a beta cutoff
    std::uint64_t first_move_cutoffs = 0;  // cutoffs caused by the first move tried
    std::uint64_t tt_probes = 0;
    std::uint64_t tt_hits = 0;     // probes that found this position
    std::uint64_t tt_cutoffs = 0;  // hits that ended the node without searching

    SearchStats& operator+=(const SearchStats& other) {
        nodes += other.nodes;
        cutoffs += other.cutoffs;
        first_move_cutoffs += other.first_move_cutoffs;
        tt_probes += other.tt_probes;
        tt_hits += other.tt_hits;
        tt_cutoffs += other.tt_cutoffs;
        return *this;
    }
};

struct SearchResult {
    int score = 0;
    Move best = kNoMove;  // kNoMove for a depth-zero or finished root
    SearchStats stats;
};

// Stage 2. Negamax with no pruning. If `nodes` is given, adds one per position visited.
inline int minimax(const Position& pos, int depth, std::uint64_t* nodes = nullptr) { PRACTICE_TODO(); }

class Engine {
public:
    explicit Engine(const SearchOptions& options = {}) {}

    // Stage 2 (Stage 3 and 4 honour the options). Fail-soft negamax alpha-beta.
    SearchResult search(const Position& pos, int depth, int alpha = -kScoreInfinity,
                        int beta = kScoreInfinity) {
        PRACTICE_TODO();
    }

    // Stage 3. Searches depth 1, 2, ..., max_depth; stats accumulate across iterations.
    SearchResult iterative(const Position& pos, int max_depth) { PRACTICE_TODO(); }

    // Stage 5. Fork-join search of a single depth; same score as search().
    SearchResult parallel(const Position& pos, int depth) { PRACTICE_TODO(); }

    // Stage 3. Forgets the table and any move-ordering history.
    void clear() { PRACTICE_TODO(); }
};
