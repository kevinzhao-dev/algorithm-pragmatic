// movegen.hpp — Stage 1: bitboard move generation and evaluation; Stage 3 adds Zobrist keys.
#pragma once

#include <cstdint>

#include "board.hpp"
#include "practice.hpp"

// Stage 3. Full hash of the board and side to move, ignoring pos.key.
inline std::uint64_t zobrist_key(const Position& pos) { PRACTICE_TODO(); }

// Stage 1. Clears `out` and fills it with every legal move for pos.side.
inline void generate_moves(const Position& pos, MoveList& out) { PRACTICE_TODO(); }

// Stage 1. Requires a legal move. From Stage 3 on, also updates next.key incrementally.
inline Position apply_move(const Position& pos, Move move) { PRACTICE_TODO(); }

// Stage 1. Number of leaf nodes of the game tree to `depth`.
inline std::uint64_t perft(const Position& pos, int depth) { PRACTICE_TODO(); }

// Stage 1. The value of the position for the side to move, per the README.
inline int evaluate(const Position& pos) { PRACTICE_TODO(); }
