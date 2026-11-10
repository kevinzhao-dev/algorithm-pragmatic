// slow_board.hpp — a plain array-based implementation of the Furrow rules.
//
// Deliberately naive and written directly from the rules. It defines correct behaviour for
// move generation and evaluation; the tests compare your bitboard engine against it.
#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>

#include "board.hpp"

namespace slow {

enum Cell : std::int8_t { kEmpty, kPlow0, kPlow1, kFurrow0, kFurrow1 };

struct Board {
    std::array<std::int8_t, kSquares> cells{};
    int side = 0;
};

inline Board from_position(const Position& pos) {
    Board board;
    board.side = pos.side;
    for (int square = 0; square < kSquares; ++square) {
        const Bitboard mask = bit(square);
        auto& cell = board.cells[static_cast<std::size_t>(square)];
        cell = (pos.plows[0] & mask) != 0     ? kPlow0
               : (pos.plows[1] & mask) != 0   ? kPlow1
               : (pos.furrows[0] & mask) != 0 ? kFurrow0
               : (pos.furrows[1] & mask) != 0 ? kFurrow1
                                              : kEmpty;
    }
    return board;
}

inline Position to_position(const Board& board) {
    Position pos;
    pos.side = board.side;
    for (int square = 0; square < kSquares; ++square) {
        const Bitboard mask = bit(square);
        switch (board.cells[static_cast<std::size_t>(square)]) {
            case kPlow0:
                pos.plows[0] |= mask;
                break;
            case kPlow1:
                pos.plows[1] |= mask;
                break;
            case kFurrow0:
                pos.furrows[0] |= mask;
                break;
            case kFurrow1:
                pos.furrows[1] |= mask;
                break;
            default:
                break;
        }
    }
    return pos;
}

inline constexpr int kDirections[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}};

inline bool on_board(int file, int rank) { return file >= 0 && file < kFiles && rank >= 0 && rank < kRanks; }

// Every square `side`'s plows could slide to, whether or not it is that side's turn.
inline std::vector<Move> moves_for(const Board& board, int side) {
    const std::int8_t my_plow = side == 0 ? kPlow0 : kPlow1;
    const std::int8_t my_furrow = side == 0 ? kFurrow0 : kFurrow1;
    std::vector<Move> moves;
    for (int square = 0; square < kSquares; ++square) {
        if (board.cells[static_cast<std::size_t>(square)] != my_plow) {
            continue;
        }
        for (const auto& direction : kDirections) {
            int file = file_of(square) + direction[0];
            int rank = rank_of(square) + direction[1];
            while (on_board(file, rank)) {
                const std::int8_t cell = board.cells[static_cast<std::size_t>(square_at(file, rank))];
                if (cell == kEmpty) {
                    moves.push_back({static_cast<std::int8_t>(square), static_cast<std::int8_t>(square_at(file, rank))});
                } else if (cell != my_furrow) {
                    break;  // Plows and enemy furrows stop the slide; own furrows are crossed.
                }
                file += direction[0];
                rank += direction[1];
            }
        }
    }
    std::sort(moves.begin(), moves.end());
    return moves;
}

inline std::vector<Move> legal_moves(const Board& board) { return moves_for(board, board.side); }

inline Board apply(const Board& board, Move move) {
    Board next = board;
    const int side = board.side;
    const std::int8_t my_plow = side == 0 ? kPlow0 : kPlow1;
    const std::int8_t my_furrow = side == 0 ? kFurrow0 : kFurrow1;
    const std::int8_t their_furrow = side == 0 ? kFurrow1 : kFurrow0;
    next.cells[static_cast<std::size_t>(move.from)] = my_furrow;
    next.cells[static_cast<std::size_t>(move.to)] = my_plow;
    static constexpr int orthogonal[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
    for (const auto& direction : orthogonal) {
        const int file = file_of(move.to) + direction[0];
        const int rank = rank_of(move.to) + direction[1];
        if (on_board(file, rank) && next.cells[static_cast<std::size_t>(square_at(file, rank))] == their_furrow) {
            next.cells[static_cast<std::size_t>(square_at(file, rank))] = my_furrow;
        }
    }
    next.side = 1 - side;
    return next;
}

inline std::uint64_t perft(const Board& board, int depth) {
    if (depth == 0) {
        return 1;
    }
    std::uint64_t nodes = 0;
    for (const Move move : legal_moves(board)) {
        nodes += perft(apply(board, move), depth - 1);
    }
    return nodes;
}

// Scores the position for the side to move. Terminal when that side has no legal moves.
inline int evaluate(const Board& board) {
    int mine = 0;
    int theirs = 0;
    for (const std::int8_t cell : board.cells) {
        mine += cell == (board.side == 0 ? kFurrow0 : kFurrow1) ? 1 : 0;
        theirs += cell == (board.side == 0 ? kFurrow1 : kFurrow0) ? 1 : 0;
    }
    const int difference = mine - theirs;
    const int my_moves = static_cast<int>(moves_for(board, board.side).size());
    if (my_moves == 0) {
        return difference > 0 ? kWinScore + difference : (difference < 0 ? -kWinScore + difference : 0);
    }
    const int their_moves = static_cast<int>(moves_for(board, 1 - board.side).size());
    return kFurrowWeight * difference + my_moves - their_moves;
}

// Plain negamax: the value every faster search must reproduce exactly.
inline int minimax(const Board& board, int depth) {
    if (depth == 0) {
        return evaluate(board);
    }
    const std::vector<Move> moves = legal_moves(board);
    if (moves.empty()) {
        return evaluate(board);
    }
    int best = -kScoreInfinity;
    for (const Move move : moves) {
        best = std::max(best, -minimax(apply(board, move), depth - 1));
    }
    return best;
}

}  // namespace slow
