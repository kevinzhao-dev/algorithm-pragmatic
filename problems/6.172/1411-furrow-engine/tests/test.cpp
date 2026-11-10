#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <iostream>
#include <limits>
#include <set>
#include <thread>
#include <tuple>
#include <unordered_set>
#include <vector>

#include "board.hpp"
#include "corpus.hpp"
#include "forkjoin.hpp"
#include "practice.hpp"
#include "slow_board.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

std::vector<Move> sorted_moves(const Position& pos) {
    MoveList list;
    generate_moves(pos, list);
    std::vector<Move> moves(list.begin(), list.end());
    std::sort(moves.begin(), moves.end());
    return moves;
}

std::vector<Move> reference_moves(const Position& pos) { return slow::legal_moves(slow::from_position(pos)); }

Move mv(const char* text) {
    return {static_cast<std::int8_t>(square_at(text[0] - 'a', text[1] - '1')),
            static_cast<std::int8_t>(square_at(text[3] - 'a', text[4] - '1'))};
}

// Builds a position from a picture: top row is rank 7, leftmost column is file a.
//   S / N  South / North plow      s / n  South / North furrow      .  empty
Position diagram(const std::array<const char*, 7>& rows, int side = 0) {
    Position pos;
    pos.side = side;
    for (int r = 0; r < 7; ++r) {
        const int rank = 6 - r;
        for (int file = 0; file < 7; ++file) {
            const char c = rows[static_cast<std::size_t>(r)][file];
            const Bitboard mask = bit(square_at(file, rank));
            switch (c) {
                case 'S':
                    pos.plows[0] |= mask;
                    break;
                case 'N':
                    pos.plows[1] |= mask;
                    break;
                case 's':
                    pos.furrows[0] |= mask;
                    break;
                case 'n':
                    pos.furrows[1] |= mask;
                    break;
                default:
                    break;
            }
        }
    }
    return pos;
}

const std::vector<Position>& sample_positions() {
    static const std::vector<Position> positions = corpus::positions(2026, 120);
    return positions;
}

// A narrow slice of the corpus deep enough into the game that branching is smaller.
std::vector<Position> late_positions(std::size_t count, std::uint64_t seed = 77) {
    std::vector<Position> out;
    corpus::SplitMix rng(seed);
    for (std::size_t i = 0; i < count; ++i) {
        out.push_back(corpus::random_game(rng, 18 + static_cast<int>(i % 16)));
    }
    return out;
}

constexpr SearchOptions kPlain{};
constexpr SearchOptions kOrdered{false, true, 16};
constexpr SearchOptions kTableOnly{true, false, 18};
constexpr SearchOptions kFull{true, true, 18};

template <typename Body>
double best_of(int rounds, Body&& body) {
    double best = std::numeric_limits<double>::infinity();
    for (int round = 0; round < rounds; ++round) {
        const practice::Stopwatch watch;
        body();
        best = std::min(best, watch.elapsed_ms());
    }
    return best;
}

}  // namespace

// ---------------------------------------------------------------------------------------------
// Stage 1 — bitboards, move generation, evaluation
// ---------------------------------------------------------------------------------------------

TEST(1, "the start position has 39 moves") {
    const auto moves = sorted_moves(start_position());
    CHECK_EQ(moves.size(), 39);
    CHECK_EQ(moves, reference_moves(start_position()));
}

TEST(1, "a lone plow reaches all 24 squares of its lines from the centre") {
    const Position pos = diagram({".......", ".......", ".......", "...S...", ".......", ".......", "......."});
    const auto moves = sorted_moves(pos);
    CHECK_EQ(moves.size(), 24);
    CHECK_EQ(moves, reference_moves(pos));
}

TEST(1, "a plow in a corner slides only along its three lines") {
    const Position pos = diagram({".......", ".......", ".......", ".......", ".......", ".......", "S......"});
    CHECK_EQ(sorted_moves(pos).size(), 18);
}

TEST(1, "own furrows are crossed but cannot be landed on") {
    const Position pos = diagram({".......", ".......", ".......", ".s.S...", ".......", ".......", "......."});
    const auto moves = sorted_moves(pos);
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-b4")) == moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-c4")) != moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-a4")) != moves.end());
    CHECK_EQ(moves, reference_moves(pos));
}

TEST(1, "enemy furrows stop the slide before them") {
    const Position pos = diagram({".......", ".......", ".......", ".n.S.n.", ".......", ".......", "......."});
    const auto moves = sorted_moves(pos);
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-c4")) != moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-b4")) == moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-a4")) == moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-e4")) != moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d4-f4")) == moves.end());
    CHECK_EQ(moves, reference_moves(pos));
}

TEST(1, "plows of either side stop the slide before them") {
    const Position pos = diagram({".......", ".......", "...N...", ".......", "...S...", ".......", "...S..."});
    const auto moves = sorted_moves(pos);
    CHECK(std::find(moves.begin(), moves.end(), mv("d3-d4")) != moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d3-d5")) == moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d3-d2")) != moves.end());
    CHECK(std::find(moves.begin(), moves.end(), mv("d3-d1")) == moves.end());
    CHECK_EQ(moves, reference_moves(pos));
}

TEST(1, "slides never wrap around a board edge") {
    const Position east_edge = diagram({".......", ".......", ".......", "......S", "S......", ".......", "......."});
    CHECK_EQ(sorted_moves(east_edge), reference_moves(east_edge));
    const Position corners = diagram({"S.....S", ".......", ".......", ".......", ".......", ".......", "S.....S"});
    CHECK_EQ(sorted_moves(corners), reference_moves(corners));
    for (const Position& pos : sample_positions()) {
        CHECK_EQ(sorted_moves(pos), reference_moves(pos));
    }
}

TEST(1, "a surrounded side has no moves and that is not an error") {
    const Position pos = diagram({".......", ".......", ".......", ".......", ".......", "nnn....", "nSn...."});
    CHECK(sorted_moves(pos).empty());
}

TEST(1, "moves match the slow reference over whole random games") {
    corpus::SplitMix rng(5);
    for (int game = 0; game < 80; ++game) {
        Position pos = start_position();
        slow::Board board = slow::from_position(pos);
        for (int ply = 0; ply < 60; ++ply) {
            const auto expected = slow::legal_moves(board);
            INFO("game ", game, " ply ", ply);
            REQUIRE_EQ(sorted_moves(pos), expected);
            if (expected.empty()) {
                break;
            }
            const Move move = expected[static_cast<std::size_t>(rng.range(0, static_cast<int>(expected.size()) - 1))];
            pos = apply_move(pos, move);
            board = slow::apply(board, move);
            REQUIRE(same_board(pos, slow::to_position(board)));
        }
    }
}

TEST(1, "a move leaves a furrow behind and moves the plow") {
    const Position pos = diagram({".......", ".......", ".......", ".......", ".......", ".......", "S......"});
    const Position next = apply_move(pos, mv("a1-a4"));
    const Position expected = diagram({".......", ".......", ".......", "S......", ".......", ".......", "s......"}, 1);
    CHECK(same_board(next, expected));
}

TEST(1, "landing flips orthogonally adjacent enemy furrows only") {
    const Position pos = diagram({".......", ".......", "..nnn..", "..n.n..", "..nnn..", ".......", "S......"});
    const Position next = apply_move(pos, mv("a1-d4"));
    // After a1-d4 the plow is at d4: c4, e4, d3, d5 flip; the diagonal furrows stay North's.
    const Position expected =
        diagram({".......", ".......", "..nsn..", "..sSs..", "..nsn..", ".......", "s......"}, 1);
    CHECK(same_board(next, expected));
}

TEST(1, "flips never wrap around a board edge") {
    // A landing on g3 has no eastern neighbour; a4 is the square a careless "+1" would reach.
    const Position east = diagram({".......", ".......", ".......", "n......", ".......", "......S", "......."});
    const Position next = apply_move(east, mv("g2-g3"));
    CHECK(same_board(next, slow::to_position(slow::apply(slow::from_position(east), mv("g2-g3")))));
    CHECK_EQ(next.furrows[1], east.furrows[1]);
    // Likewise a landing on a5 has no western neighbour; g4 is what a careless "-1" would reach.
    const Position west = diagram({".......", ".......", ".......", "S.....n", ".......", ".......", "......."});
    const Position west_next = apply_move(west, mv("a4-a5"));
    CHECK(same_board(west_next, slow::to_position(slow::apply(slow::from_position(west), mv("a4-a5")))));
    CHECK_EQ(west_next.furrows[1], west.furrows[1]);
}

TEST(1, "a move never overlaps two kinds of piece") {
    corpus::SplitMix rng(9);
    for (int game = 0; game < 40; ++game) {
        Position pos = start_position();
        for (int ply = 0; ply < 60; ++ply) {
            const auto moves = sorted_moves(pos);
            if (moves.empty()) {
                break;
            }
            pos = apply_move(pos, moves[static_cast<std::size_t>(rng.range(0, static_cast<int>(moves.size()) - 1))]);
            const Bitboard all[] = {pos.plows[0], pos.plows[1], pos.furrows[0], pos.furrows[1]};
            Bitboard seen = 0;
            for (const Bitboard board : all) {
                CHECK_EQ(seen & board, 0);
                CHECK_EQ(board & ~kAllSquares, 0);
                seen |= board;
            }
            CHECK_EQ(std::popcount(pos.plows[0]), 3);
            CHECK_EQ(std::popcount(pos.plows[1]), 3);
        }
    }
}

TEST(1, "the side to move alternates") {
    const Position next = apply_move(start_position(), mv("b1-b2"));
    CHECK_EQ(next.side, 1);
    CHECK_EQ(apply_move(next, mv("b7-b6")).side, 0);
}

TEST(1, "perft from the start position") {
    const Position start = start_position();
    CHECK_EQ(perft(start, 0), 1);
    CHECK_EQ(perft(start, 1), 39);
    CHECK_EQ(perft(start, 2), 1424);
    CHECK_EQ(perft(start, 3), 56548);
    CHECK_EQ(perft(start, 4), 2122948);
}

TEST(1, "perft counts every line to depth on random positions") {
    int checked = 0;
    for (const Position& pos : sample_positions()) {
        INFO("position ", checked);
        const auto board = slow::from_position(pos);
        CHECK_EQ(perft(pos, 1), slow::perft(board, 1));
        CHECK_EQ(perft(pos, 2), slow::perft(board, 2));
        if (checked % 4 == 0) {
            CHECK_EQ(perft(pos, 3), slow::perft(board, 3));
        }
        ++checked;
    }
}

TEST(1, "perft on a finished game counts nothing below the root") {
    const Position pos = diagram({".......", ".......", ".......", ".......", ".......", "nnn....", "nSn...."});
    CHECK_EQ(perft(pos, 1), 0);
    CHECK_EQ(perft(pos, 4), 0);
    CHECK_EQ(perft(pos, 0), 1);
}

TEST(1, "evaluation matches the slow reference on random positions") {
    for (const Position& pos : sample_positions()) {
        CHECK_EQ(evaluate(pos), slow::evaluate(slow::from_position(pos)));
    }
    corpus::SplitMix rng(11);
    for (int i = 0; i < 400; ++i) {
        const Position pos = corpus::random_game(rng, rng.range(0, 46));
        REQUIRE_EQ(evaluate(pos), slow::evaluate(slow::from_position(pos)));
    }
}

TEST(1, "evaluation is antisymmetric in the side to move") {
    corpus::SplitMix rng(17);
    for (int i = 0; i < 300; ++i) {
        Position pos = corpus::random_game(rng, rng.range(0, 30));
        MoveList moves;
        generate_moves(pos, moves);
        if (moves.empty()) {
            continue;
        }
        Position flipped = pos;
        flipped.side ^= 1;
        MoveList other;
        generate_moves(flipped, other);
        if (!other.empty()) {
            CHECK_EQ(evaluate(flipped), -evaluate(pos));
        }
    }
}

TEST(1, "a finished game is scored by the furrow count") {
    // South has no moves and more furrows than North.
    const Position south_stuck_winning =
        diagram({"nnnnnnn", "ss.....", "NNN....", "s......", "s......", "sss....", "sSs...."});
    CHECK_EQ(evaluate(south_stuck_winning), slow::evaluate(slow::from_position(south_stuck_winning)));
    const Position blocked = diagram({".......", ".......", ".......", ".......", ".......", "nnn....", "nSn...."});
    // South has no move and two fewer furrows: a loss.
    CHECK_EQ(evaluate(blocked), -kWinScore - 5);
    CHECK_EQ(evaluate(blocked), slow::evaluate(slow::from_position(blocked)));
}

PERF(1, "generate_moves is at least 4x faster than the array reference", 20'000) {
    const auto& positions = sample_positions();
    std::uint64_t sink = 0;
    const double fast_ms = best_of(5, [&] {
        MoveList moves;
        for (int repeat = 0; repeat < 1000; ++repeat) {
            for (const Position& pos : positions) {
                generate_moves(pos, moves);
                sink += static_cast<std::uint64_t>(moves.size());
            }
        }
    });
    const double slow_ms = best_of(2, [&] {
        for (int repeat = 0; repeat < 100; ++repeat) {
            for (const Position& pos : positions) {
                sink += slow::legal_moves(slow::from_position(pos)).size();
            }
        }
    });
    INFO("fast ", fast_ms, " ms (1000 rounds), slow ", slow_ms, " ms (100 rounds), sink ", sink);
    CHECK_GE(slow_ms * 10 / fast_ms, 4.0);
}

PERF(1, "bitboard evaluation is at least 4x faster than the array reference", 20'000) {
    const auto& positions = sample_positions();
    long long sink = 0;
    double fast_ms = best_of(3, [&] {
        for (int repeat = 0; repeat < 200; ++repeat) {
            for (const Position& pos : positions) {
                sink += evaluate(pos);
            }
        }
    });
    double slow_ms = best_of(3, [&] {
        for (int repeat = 0; repeat < 20; ++repeat) {
            for (const Position& pos : positions) {
                sink += slow::evaluate(slow::from_position(pos));
            }
        }
    });
    INFO("fast ", fast_ms, " ms (200 rounds), slow ", slow_ms, " ms (20 rounds), sink ", sink);
    CHECK_GE(slow_ms * 10 / fast_ms, 4.0);
}

// ---------------------------------------------------------------------------------------------
// Stage 2 — minimax and alpha-beta
// ---------------------------------------------------------------------------------------------

TEST(2, "minimax at depth zero is the static evaluation") {
    const Position pos = start_position();
    CHECK_EQ(minimax(pos, 0), evaluate(pos));
}

TEST(2, "minimax of a finished game is its final score at any depth") {
    const Position stuck = diagram({".......", ".......", ".......", ".......", ".......", "nnn....", "nSn...."});
    CHECK_EQ(minimax(stuck, 5), evaluate(stuck));
}

TEST(2, "minimax matches the slow reference on a corpus") {
    const auto& positions = sample_positions();
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& pos = positions[i];
        INFO("position ", i);
        const auto board = slow::from_position(pos);
        CHECK_EQ(minimax(pos, 1), slow::minimax(board, 1));
        CHECK_EQ(minimax(pos, 2), slow::minimax(board, 2));
        if (i % 24 == 0) {
            CHECK_EQ(minimax(pos, 3), slow::minimax(board, 3));
        }
    }
}

TEST(2, "minimax counts every node when asked") {
    std::uint64_t nodes = 0;
    minimax(start_position(), 2, &nodes);
    CHECK_EQ(nodes, 1 + 39 + 1424);
}

TEST(2, "a full-window alpha-beta returns exactly the minimax value") {
    const auto& positions = sample_positions();
    Engine engine(kPlain);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& pos = positions[i];
        INFO("position ", i);
        for (int depth = 1; depth <= 3; ++depth) {
            CHECK_EQ(engine.search(pos, depth).score, minimax(pos, depth));
        }
    }
    const auto deeper = late_positions(16);
    for (std::size_t i = 0; i < deeper.size(); ++i) {
        INFO("late position ", i);
        CHECK_EQ(engine.search(deeper[i], 4).score, minimax(deeper[i], 4));
    }
}

TEST(2, "alpha-beta at depth zero is the static evaluation, with no move") {
    const Position pos = sample_positions()[3];
    const SearchResult result = Engine(kPlain).search(pos, 0);
    CHECK_EQ(result.score, evaluate(pos));
    CHECK(!result.best);
    CHECK_EQ(result.stats.nodes, 1);
}

TEST(2, "a finished game returns its score and no move") {
    const Position stuck = diagram({".......", ".......", ".......", ".......", ".......", "nnn....", "nSn...."});
    const SearchResult result = Engine(kPlain).search(stuck, 6);
    CHECK_EQ(result.score, evaluate(stuck));
    CHECK(!result.best);
    CHECK_EQ(result.stats.nodes, 1);
}

TEST(2, "the reported move is legal and achieves the score") {
    Engine engine(kPlain);
    const auto positions = late_positions(30);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& pos = positions[i];
        INFO("position ", i);
        const int depth = 1 + static_cast<int>(i % 4);
        const SearchResult result = engine.search(pos, depth);
        const auto legal = sorted_moves(pos);
        if (legal.empty()) {
            CHECK(!result.best);
        } else {
            REQUIRE(std::find(legal.begin(), legal.end(), result.best) != legal.end());
            CHECK_EQ(-minimax(apply_move(pos, result.best), depth - 1), result.score);
        }
    }
}

TEST(2, "scores stay consistent across one ply") {
    Engine engine(kPlain);
    corpus::SplitMix rng(3);
    for (int trial = 0; trial < 60; ++trial) {
        const Position pos = corpus::random_game(rng, 16);
        const auto moves = sorted_moves(pos);
        if (moves.empty()) {
            continue;
        }
        const SearchResult root = engine.search(pos, 3);
        int best_child = -kScoreInfinity;
        for (const Move move : moves) {
            best_child = std::max(best_child, -engine.search(apply_move(pos, move), 2).score);
        }
        INFO("trial ", trial);
        CHECK_EQ(best_child, root.score);
    }
}

TEST(2, "windows keep the fail-soft contract") {
    Rng rng;
    Engine engine(kPlain);
    const auto positions = late_positions(50);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& pos = positions[i];
        const int depth = 3;
        const int exact = minimax(pos, depth);
        for (int window = 0; window < 8; ++window) {
            const int center = exact + static_cast<int>(rng.uniform(-40, 40));
            const int width = static_cast<int>(rng.uniform(1, 30));
            const int alpha = center - width;
            const int beta = center + width;
            const int value = engine.search(pos, depth, alpha, beta).score;
            INFO("position ", i, " exact ", exact, " window (", alpha, ",", beta, ") value ", value);
            if (exact <= alpha) {
                CHECK_LE(value, alpha);
                CHECK_GE(value, exact);
            } else if (exact >= beta) {
                CHECK_GE(value, beta);
                CHECK_LE(value, exact);
            } else {
                CHECK_EQ(value, exact);
            }
        }
    }
}

TEST(2, "alpha-beta visits far fewer nodes than minimax") {
    Engine engine(kPlain);
    std::uint64_t minimax_nodes = 0;
    std::uint64_t search_nodes = 0;
    const auto positions = late_positions(20);
    for (const Position& pos : positions) {
        minimax(pos, 4, &minimax_nodes);
        search_nodes += engine.search(pos, 4).stats.nodes;
    }
    INFO("minimax ", minimax_nodes, " alpha-beta ", search_nodes);
    CHECK_LT(search_nodes * 2, minimax_nodes);
}

TEST(2, "node counts are deterministic and include the root") {
    const Position pos = sample_positions()[5];
    CHECK_EQ(Engine(kPlain).search(pos, 3).stats.nodes, Engine(kPlain).search(pos, 3).stats.nodes);
    CHECK_GE(Engine(kPlain).search(pos, 1).stats.nodes, 2);
}

// ---------------------------------------------------------------------------------------------
// Stage 3 — Zobrist hashing and the transposition table
// ---------------------------------------------------------------------------------------------

TEST(3, "the Zobrist key depends on the side to move, the piece kind and the owner") {
    Position south = diagram({".......", ".......", ".......", "...s...", ".......", ".......", "......."});
    Position north = diagram({".......", ".......", ".......", "...n...", ".......", ".......", "......."});
    Position plow = diagram({".......", ".......", ".......", "...S...", ".......", ".......", "......."});
    Position other = diagram({".......", ".......", ".......", "..s....", ".......", ".......", "......."});
    Position turn = south;
    turn.side = 1;
    const std::set<std::uint64_t> keys{zobrist_key(south), zobrist_key(north), zobrist_key(plow), zobrist_key(other),
                                       zobrist_key(turn)};
    CHECK_EQ(keys.size(), 5);
    CHECK_EQ(zobrist_key(start_position()), zobrist_key(start_position()));
}

TEST(3, "apply_move keeps the key equal to a full recomputation") {
    corpus::SplitMix rng(21);
    for (int game = 0; game < 60; ++game) {
        Position pos = start_position();
        pos.key = zobrist_key(pos);
        for (int ply = 0; ply < 60; ++ply) {
            const auto moves = sorted_moves(pos);
            if (moves.empty()) {
                break;
            }
            pos = apply_move(pos, moves[static_cast<std::size_t>(rng.range(0, static_cast<int>(moves.size()) - 1))]);
            INFO("game ", game, " ply ", ply);
            REQUIRE_EQ(pos.key, zobrist_key(pos));
        }
    }
}

TEST(3, "two move orders that reach the same board share one key") {
    Position first = start_position();
    first.key = zobrist_key(first);
    Position second = first;
    first = apply_move(first, mv("b1-b3"));
    first = apply_move(first, mv("b7-b5"));
    first = apply_move(first, mv("d1-d3"));
    second = apply_move(second, mv("d1-d3"));
    second = apply_move(second, mv("b7-b5"));
    second = apply_move(second, mv("b1-b3"));
    REQUIRE(same_board(first, second));
    CHECK_EQ(first.key, second.key);
}

TEST(3, "distinct positions get distinct keys") {
    std::unordered_set<std::uint64_t> keys;
    std::vector<Position> distinct;
    corpus::SplitMix rng(99);
    for (int i = 0; i < 6'000; ++i) {
        const Position pos = corpus::random_game(rng, rng.range(0, 40));
        keys.insert(zobrist_key(pos));
        distinct.push_back(pos);
    }
    std::set<std::tuple<Bitboard, Bitboard, Bitboard, Bitboard, int>> boards;
    for (const Position& pos : distinct) {
        boards.insert({pos.plows[0], pos.plows[1], pos.furrows[0], pos.furrows[1], pos.side});
    }
    CHECK_EQ(keys.size(), boards.size());
}

TEST(3, "the table returns what was stored") {
    TranspositionTable table(10);
    CHECK_GE(table.size(), 1024);
    const TTEntry entry{0x1234'5678'9ABC'DEF0ULL, -123, 7, Bound::Lower, mv("a1-a2")};
    table.store(entry);
    const auto found = table.probe(entry.key);
    REQUIRE(found.has_value());
    CHECK_EQ(found->key, entry.key);
    CHECK_EQ(found->score, -123);
    CHECK_EQ(found->depth, 7);
    CHECK(found->bound == Bound::Lower);
    CHECK_EQ(found->best, entry.best);
}

TEST(3, "an empty table finds nothing and clear empties it") {
    TranspositionTable table(8);
    CHECK(!table.probe(42).has_value());
    table.store({42, 1, 1, Bound::Exact, kNoMove});
    table.clear();
    CHECK(!table.probe(42).has_value());
}

TEST(3, "negative scores and every bound survive the round trip") {
    TranspositionTable table(12);
    Rng rng;
    for (int i = 0; i < 2000; ++i) {
        const std::uint64_t key = rng.engine()();
        const Bound bound = static_cast<Bound>(rng.uniform(0, 2));
        const int score = static_cast<int>(rng.uniform(-10100, 10100));
        const int depth = static_cast<int>(rng.uniform(0, 40));
        const TTEntry entry{key, score, depth, bound, mv("g7-a1")};
        table.store(entry);
        const auto found = table.probe(key);
        REQUIRE(found.has_value());
        CHECK_EQ(found->score, score);
        CHECK_EQ(found->depth, depth);
        CHECK(found->bound == bound);
    }
}

TEST(3, "a tiny table never answers with another key's entry") {
    TranspositionTable table(1);
    Rng rng;
    std::vector<std::uint64_t> keys;
    for (int i = 0; i < 400; ++i) {
        keys.push_back(rng.engine()());
        table.store({keys.back(), i, 1, Bound::Exact, kNoMove});
        for (const std::uint64_t key : keys) {
            if (const auto found = table.probe(key)) {
                CHECK_EQ(found->key, key);
            }
        }
    }
}

TEST(3, "search with the table returns exact scores for every table size") {
    const auto positions = late_positions(24);
    for (const int bits : {1, 6, 12, 18}) {
        Engine engine({true, false, bits});
        for (std::size_t i = 0; i < positions.size(); ++i) {
            INFO("position ", i, " bits ", bits);
            const int depth = 1 + static_cast<int>(i % 4);
            CHECK_EQ(engine.search(positions[i], depth).score, minimax(positions[i], depth));
        }
    }
}

TEST(3, "a table entry from a deeper search never answers a shallower one") {
    Engine engine(kTableOnly);
    const auto positions = late_positions(20);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        engine.search(positions[i], 4);
        CHECK_EQ(engine.search(positions[i], 2).score, minimax(positions[i], 2));
    }
}

TEST(3, "narrow windows stay correct with the table enabled") {
    Rng rng;
    Engine engine(kTableOnly);
    const auto positions = late_positions(40);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& pos = positions[i];
        const int exact = minimax(pos, 3);
        for (int window = 0; window < 8; ++window) {
            const int center = exact + static_cast<int>(rng.uniform(-40, 40));
            const int width = static_cast<int>(rng.uniform(1, 30));
            const int alpha = center - width;
            const int beta = center + width;
            const int value = engine.search(pos, 3, alpha, beta).score;
            INFO("position ", i, " exact ", exact, " window (", alpha, ",", beta, ") value ", value);
            if (exact <= alpha) {
                CHECK_LE(value, alpha);
            } else if (exact >= beta) {
                CHECK_GE(value, beta);
            } else {
                CHECK_EQ(value, exact);
            }
        }
        engine.clear();
    }
}

TEST(3, "searching the same position again is answered by the table") {
    Engine engine(kFull);
    const Position pos = sample_positions()[8];
    const SearchResult first = engine.search(pos, 4);
    const SearchResult second = engine.search(pos, 4);
    CHECK_EQ(second.score, first.score);
    CHECK_GT(second.stats.tt_hits, 0);
    CHECK_LT(second.stats.nodes * 10, first.stats.nodes);
    CHECK_EQ(second.best, first.best);
}

TEST(3, "clear makes the engine forget") {
    Engine engine(kFull);
    const Position pos = sample_positions()[9];
    const SearchResult first = engine.search(pos, 3);
    engine.clear();
    const SearchResult after_clear = engine.search(pos, 3);
    CHECK_EQ(after_clear.stats.nodes, first.stats.nodes);
}

TEST(3, "the search reports table probes and hits") {
    Engine engine(kTableOnly);
    const SearchResult result = engine.search(late_positions(1)[0], 4);
    CHECK_GT(result.stats.tt_probes, 0);
    CHECK_GT(result.stats.tt_hits, 0);
    CHECK_LE(result.stats.tt_hits, result.stats.tt_probes);
}

TEST(3, "iterative deepening ends with the exact value of its deepest iteration") {
    const auto positions = late_positions(16);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        Engine engine(kTableOnly);
        CHECK_EQ(engine.iterative(positions[i], 4).score, minimax(positions[i], 4));
    }
}

PERF(3, "the transposition table cuts the tree on a mid-game corpus", 60'000) {
    const auto positions = late_positions(20, 2024);
    std::uint64_t plain_nodes = 0;
    std::uint64_t table_nodes = 0;
    for (const Position& pos : positions) {
        plain_nodes += Engine(kPlain).search(pos, 6).stats.nodes;
        Engine engine(SearchOptions{true, false, 20});
        table_nodes += engine.search(pos, 6).stats.nodes;
    }
    INFO("plain ", plain_nodes, " nodes, with table ", table_nodes, " nodes");
    CHECK_LT(static_cast<double>(table_nodes), static_cast<double>(plain_nodes) * 0.97);
}

// ---------------------------------------------------------------------------------------------
// Stage 4 — move ordering and iterative deepening, judged by node counts
// ---------------------------------------------------------------------------------------------

namespace {

std::uint64_t corpus_nodes(const std::vector<Position>& positions, int depth, const SearchOptions& options) {
    std::uint64_t total = 0;
    for (const Position& pos : positions) {
        Engine engine(options);
        total += engine.search(pos, depth).stats.nodes;
    }
    return total;
}

}  // namespace

TEST(4, "ordering does not change the search result") {
    const auto positions = late_positions(16);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        CHECK_EQ(Engine(kOrdered).search(positions[i], 4).score, minimax(positions[i], 4));
        CHECK_EQ(Engine(kFull).search(positions[i], 4).score, minimax(positions[i], 4));
    }
}

TEST(4, "ordering prunes a majority of the plain alpha-beta tree") {
    const auto positions = late_positions(12);
    const std::uint64_t plain = corpus_nodes(positions, 4, kPlain);
    const std::uint64_t ordered = corpus_nodes(positions, 4, kOrdered);
    INFO("plain ", plain, " ordered ", ordered);
    CHECK_LT(ordered * 2, plain);
}

TEST(4, "most cutoffs happen on the first move tried") {
    const auto positions = late_positions(12);
    SearchStats total;
    for (const Position& pos : positions) {
        total += Engine(kFull).search(pos, 4).stats;
    }
    INFO("first-move cutoffs ", total.first_move_cutoffs, " of ", total.cutoffs);
    REQUIRE(total.cutoffs > 0);
    CHECK_GE(static_cast<double>(total.first_move_cutoffs) / static_cast<double>(total.cutoffs), 0.85);
}

TEST(4, "the table's best move is tried first on a re-search") {
    Engine engine(kFull);
    const Position pos = late_positions(1, 301)[0];
    engine.search(pos, 3);
    const SearchResult deeper = engine.search(pos, 4);
    CHECK_EQ(deeper.score, minimax(pos, 4));
}

TEST(4, "iterative deepening costs little and keeps the exact value") {
    const auto positions = late_positions(10);
    std::uint64_t single = 0;
    std::uint64_t deepening = 0;
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        const int exact = minimax(positions[i], 4);
        Engine direct(kFull);
        single += direct.search(positions[i], 4).stats.nodes;
        Engine iter(kFull);
        const SearchResult result = iter.iterative(positions[i], 4);
        CHECK_EQ(result.score, exact);
        deepening += result.stats.nodes;
    }
    INFO("single ", single, " iterative ", deepening);
    CHECK_LT(deepening, single * 2);
}

PERF(4, "ordering removes at least 70% of the plain alpha-beta nodes", 60'000) {
    const auto positions = late_positions(30, 44);
    const std::uint64_t plain = corpus_nodes(positions, 5, kPlain);
    const std::uint64_t ordered = corpus_nodes(positions, 5, kOrdered);
    INFO("plain ", plain, " nodes, ordered ", ordered, " nodes");
    CHECK_LT(static_cast<double>(ordered), static_cast<double>(plain) * 0.30);
}

PERF(4, "table plus ordering reaches at least 5x fewer nodes than plain alpha-beta", 60'000) {
    const auto positions = late_positions(20, 45);
    const std::uint64_t plain = corpus_nodes(positions, 6, kPlain);
    const std::uint64_t full = corpus_nodes(positions, 6, SearchOptions{true, true, 20});
    INFO("plain ", plain, " nodes, full ", full, " nodes");
    CHECK_LT(full * 5, plain);
}

PERF(4, "the tuned engine searches a fixed depth at least 5x faster than plain alpha-beta", 60'000) {
    const auto positions = late_positions(12, 46);
    long long sink = 0;
    const double plain_ms = best_of(2, [&] {
        for (const Position& pos : positions) {
            sink += Engine(kPlain).search(pos, 7).score;
        }
    });
    const double tuned_ms = best_of(3, [&] {
        for (const Position& pos : positions) {
            Engine engine(SearchOptions{true, true, 20});
            sink += engine.search(pos, 7).score;
        }
    });
    INFO("plain ", plain_ms, " ms, tuned ", tuned_ms, " ms, sink ", sink);
    CHECK_GE(plain_ms / tuned_ms, 5.0);
}

// Prints the numbers an engine author reads (run the binary by hand with --perf-only --verbose).
PERF(4, "engine report", 60'000) {
    const auto positions = late_positions(20, 47);
    const auto report = [&](const char* name, const SearchOptions& options, int depth) {
        SearchStats total;
        double ms = 0;
        for (const Position& pos : positions) {
            Engine engine(options);
            const practice::Stopwatch watch;
            total += engine.search(pos, depth).stats;
            ms += watch.elapsed_ms();
        }
        const double cutoffs = static_cast<double>(total.cutoffs);
        const double ebf = std::pow(static_cast<double>(total.nodes) / 20.0, 1.0 / depth);
        std::cout << "  " << name << " depth " << depth << ": nodes " << total.nodes << ", " << ms << " ms, "
                  << "EBF " << ebf << ", first-move cutoffs "
                  << (cutoffs > 0 ? 100.0 * static_cast<double>(total.first_move_cutoffs) / cutoffs : 0.0) << "%";
        if (options.use_tt) {
            std::cout << ", table hits "
                      << 100.0 * static_cast<double>(total.tt_hits) / static_cast<double>(std::max<std::uint64_t>(total.tt_probes, 1))
                      << "% of " << total.tt_probes << " probes";
        }
        std::cout << "\n";
    };
    for (const int depth : {5, 6}) {
        report("plain", kPlain, depth);
        report("ordered", kOrdered, depth);
        report("table", SearchOptions{true, false, 20}, depth);
        report("ordered+table", SearchOptions{true, true, 20}, depth);
    }
}

// ---------------------------------------------------------------------------------------------
// Stage 5 — fork-join search
// ---------------------------------------------------------------------------------------------

TEST(5, "parallel search returns the serial value for any worker count") {
    const auto positions = late_positions(10, 73);
    for (const unsigned workers : {1U, 2U, 4U, 8U}) {
        practice::set_workers(workers);
        for (std::size_t i = 0; i < positions.size(); ++i) {
            INFO("workers ", workers, " position ", i);
            const int exact = minimax(positions[i], 4);
            Engine engine(SearchOptions{true, true, 16});
            CHECK_EQ(engine.parallel(positions[i], 4).score, exact);
        }
    }
}

TEST(5, "parallel search reports a legal best move") {
    practice::set_workers(8);
    const auto positions = late_positions(12, 74);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        const Position& pos = positions[i];
        INFO("position ", i);
        Engine engine(SearchOptions{true, true, 16});
        const SearchResult result = engine.parallel(pos, 5);
        const auto legal = sorted_moves(pos);
        if (legal.empty()) {
            CHECK(!result.best);
            continue;
        }
        REQUIRE(std::find(legal.begin(), legal.end(), result.best) != legal.end());
        Engine verify(SearchOptions{true, true, 16});
        CHECK_EQ(-verify.search(apply_move(pos, result.best), 4).score, result.score);
    }
}

TEST(5, "parallel search handles depth zero and finished games") {
    practice::set_workers(4);
    Engine engine(SearchOptions{true, true, 12});
    const Position pos = late_positions(1, 75)[0];
    CHECK_EQ(engine.parallel(pos, 0).score, evaluate(pos));
    const Position stuck = diagram({".......", ".......", ".......", ".......", ".......", "nnn....", "nSn...."});
    CHECK_EQ(engine.parallel(stuck, 5).score, evaluate(stuck));
    CHECK(!engine.parallel(stuck, 5).best);
}

TEST(5, "repeated parallel searches agree with serial on the same table sizes") {
    practice::set_workers(8);
    const auto positions = late_positions(6, 76);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        const int exact = minimax(positions[i], 4);
        for (const int bits : {1, 10, 16}) {
            Engine engine(SearchOptions{true, true, bits});
            for (int round = 0; round < 3; ++round) {
                CHECK_EQ(engine.parallel(positions[i], 4).score, exact);
            }
        }
    }
}

TEST(5, "parallel search without table or ordering is still exact") {
    practice::set_workers(4);
    const auto positions = late_positions(8, 78);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        Engine engine(kPlain);
        CHECK_EQ(engine.parallel(positions[i], 4).score, minimax(positions[i], 4));
    }
}

TEST(5, "parallel search counts its nodes") {
    practice::set_workers(4);
    Engine engine(SearchOptions{true, true, 16});
    const SearchResult result = engine.parallel(late_positions(1, 79)[0], 5);
    CHECK_GT(result.stats.nodes, 100);
}

PERF(5, "parallel search returns exact values at depth 7", 30'000) {
    practice::set_workers(std::min(8U, std::max(1U, std::thread::hardware_concurrency())));
    const auto positions = late_positions(6, 80);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        Engine serial(SearchOptions{true, true, 20});
        const int expected = serial.search(positions[i], 7).score;
        Engine parallel(SearchOptions{true, true, 20});
        CHECK_EQ(parallel.parallel(positions[i], 7).score, expected);
    }
}

PERF(5, "eight workers are at least 1.5x faster than one", 60'000) {
    if (std::thread::hardware_concurrency() < 8) {
        return;
    }
    const auto positions = late_positions(6, 81);
    long long sink = 0;
    double one = std::numeric_limits<double>::infinity();
    double many = std::numeric_limits<double>::infinity();
    for (int round = 0; round < 3; ++round) {
        practice::set_workers(1);
        one = std::min(one, best_of(1, [&] {
                           for (const Position& pos : positions) {
                               Engine engine(SearchOptions{true, true, 20});
                               sink += engine.parallel(pos, 8).score;
                           }
                       }));
        practice::set_workers(8);
        many = std::min(many, best_of(1, [&] {
                            for (const Position& pos : positions) {
                                Engine engine(SearchOptions{true, true, 20});
                                sink += engine.parallel(pos, 8).score;
                            }
                        }));
    }
    practice::set_workers(1);
    INFO("one worker ", one, " ms, eight ", many, " ms, sink ", sink);
    CHECK_GE(one / many, 1.5);
}

PERF(5, "speculation stays bounded: parallel search does not explode the node count", 60'000) {
    if (std::thread::hardware_concurrency() < 8) {
        return;
    }
    const auto positions = late_positions(6, 82);
    std::uint64_t serial_nodes = 0;
    for (const Position& pos : positions) {
        Engine serial(SearchOptions{true, true, 20});
        serial_nodes += serial.search(pos, 8).stats.nodes;
    }
    const auto parallel_nodes_at = [&](unsigned workers) {
        practice::set_workers(workers);
        std::uint64_t total = 0;
        for (const Position& pos : positions) {
            Engine parallel(SearchOptions{true, true, 20});
            total += parallel.parallel(pos, 8).stats.nodes;
        }
        return total;
    };
    const std::uint64_t at_four = parallel_nodes_at(4);
    const std::uint64_t at_eight = parallel_nodes_at(8);
    practice::set_workers(1);
    INFO("serial ", serial_nodes, " nodes; 4 workers ", at_four, "; 8 workers ", at_eight);
    CHECK_LT(static_cast<double>(at_four), static_cast<double>(serial_nodes) * 2.2);
    CHECK_LT(static_cast<double>(at_eight), static_cast<double>(serial_nodes) * 2.8);
}

PERF(5, "parallel search stays exact across many repetitions", 30'000) {
    practice::set_workers(std::min(8U, std::max(1U, std::thread::hardware_concurrency())));
    const auto positions = late_positions(8, 83);
    for (std::size_t i = 0; i < positions.size(); ++i) {
        INFO("position ", i);
        Engine serial(SearchOptions{true, true, 18});
        const int expected = serial.search(positions[i], 6).score;
        for (int round = 0; round < 40; ++round) {
            Engine engine(SearchOptions{true, true, 18});
            CHECK_EQ(engine.parallel(positions[i], 6).score, expected);
        }
    }
    practice::set_workers(1);
}
