// tt.hpp — Stage 3: the transposition table.
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>

#include "board.hpp"
#include "practice.hpp"

enum class Bound : std::uint8_t { Exact, Lower, Upper };

struct TTEntry {
    std::uint64_t key = 0;
    int score = 0;
    int depth = 0;
    Bound bound = Bound::Exact;
    Move best = kNoMove;
};

class TranspositionTable {
public:
    explicit TranspositionTable(int log2_entries) { PRACTICE_TODO(); }

    std::optional<TTEntry> probe(std::uint64_t key) const { PRACTICE_TODO(); }
    void store(const TTEntry& entry) { PRACTICE_TODO(); }
    void clear() { PRACTICE_TODO(); }
    std::size_t size() const { PRACTICE_TODO(); }
};
