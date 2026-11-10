---
title: Furrow Search Engine (bitboard → alpha-beta → fork-join)
unit: 14 · Game-Playing Engine (Project 4)
source: 6.172 Project 4
pdf: materials/6.172/projects/p4/handout.pdf
read: materials/6.172/lectures/lec19.pdf, materials/6.172/lectures/lec20.pdf
kind: project
minutes: 900
sanitizer: tsan
timeout: 180
---

# 1411 · Furrow Search Engine

**Original problem:** [Project 4: Leiserchess](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/b13a935db7ea2a78a16582065bba55be_MIT6_172F18_project4.pdf) · **Read first:** [Leiserchess Codewalk](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/809f3351da7a2e5a6afc7c22e2e68e4d_MIT6_172F18_lec19.pdf), [Speculative Parallelism & Leiserchess](https://ocw.mit.edu/courses/6-172-performance-engineering-of-software-systems-fall-2018/076be084af69b7d645c30b1f337a3b08_MIT6_172F18_lec20.pdf)

> **Why this comes up in practice:** alpha-beta, transposition tables, move ordering, and speculative parallelism don't only show up in board games; they are also the common skeleton of database join ordering, planners, and formal verification. As in the original course project, the point of this exercise is to **make a search engine faster by watching its performance numbers**, not to memorize algorithms.

## Original problem

6.172 Project 4 is the course's final project, done in teams. Each team starts from a working, high-quality engine for Leiserchess, a laser-chess variant for two players (this exercise doesn't use its rules). The engine chooses moves with Principal Variation Search (PVS), an improved form of alpha-beta search, which itself prunes minimax. The job is to make that engine play stronger without breaking it. The handout points out that of two engines running the same search, the faster one wins, because it examines more positions in the same time; so it recommends performance-engineering the existing search and evaluation over inventing new heuristics. Teams work in the provided C codebase.

- Background: a game tree whose branching factor rules out a full search, alpha-beta pruning with a lower bound (alpha) and an upper bound (beta) on the score, and a static evaluator applied at the leaves of a fixed-depth search, where a fast evaluator that lets you search deeper often beats an accurate but slow one.
- Serial first: the first milestone allows no parallelism. Profile the engine, remove its hotspots, and plan further optimizations from measurements.
- Parallel next: search selected subtrees in parallel (speculative parallelism), abort the subtrees that a cutoff has made unnecessary as early as possible, avoid spending cores on subtrees that will probably be thrown away, and decide which races you can live with, both on alpha and beta and on shared tables such as the transposition table, killer moves, and move history.
- Judging: the final engine plays round-robin tournaments against other engines under Fischer time control. The handout suggests checking correctness by matching the reference engine's moves at the same search depth, debugging move generation with the engine's `perft` command, and comparing versions by Elo rating rather than by nodes per second.

## Your task

The board game was designed separately for this exercise (it uses none of the game rules from the course). All of its rules are written below, and `support/slow_board.hpp` has a "literal translation" of them as an array implementation. It is the only standard of correctness, and everything you write in this exercise must beat it on speed.

Where the course project hands you a large, finished engine to speed up, here you build a Furrow engine yourself in five stages (bitboard move generation, alpha-beta, a transposition table, move ordering, and a fork-join parallel search), and the tests hold every stage to exact scores and measured speed targets.

Write your code in `movegen.hpp`, `tt.hpp`, and `search.hpp` (`solution.hpp` just includes all three); the shared game types and constants are in `support/board.hpp`. Each stage lists the declarations it adds, copied from those files, under a comment that names the file.

## Furrow rules

A 7×7 board, with squares numbered `a1 = 0, b1 = 1, …, g7 = 48` (`board.hpp`). Each side has **3 plows**: South on `b1, d1, f1` and North on `b7, d7, f7`. South moves first.

- A move: pick one of your plows and slide it in a straight line in one of the eight directions, at least one square, stopping on an empty square. The slide **may pass over your own furrows** (but may not stop on one); it may not pass over an enemy furrow, nor over any plow.
- After the move: the starting square becomes **your own furrow**, and every enemy furrow on the four squares **above, below, left, and right** of the landing square flips to yours.
- **The game ends when the side to move has no legal move**, and the final furrow count decides the winner.

**Evaluation function** (for the side to move): `diff = own furrows − enemy furrows`, `m = number of own legal moves`, and `m′ = number of enemy legal moves if it were the enemy's turn`:
`m == 0` means the game is over: if `diff > 0`, return `kWinScore + diff`; if `diff < 0`, return `−kWinScore + diff`; if `diff == 0`, return 0. Otherwise return `kFurrowWeight × diff + m − m′` (the constants are in `board.hpp`).

`Position` is four bitboards plus `side` and `key` (`key` only matters from Stage 3 on, and `same_board` ignores it when comparing).

## Stage 1 — Bitboard move generation

```cpp
// movegen.hpp
void generate_moves(const Position& pos, MoveList& out);  // clears out, then fills it with every legal move
Position apply_move(const Position& pos, Move move);      // requires a legal move
std::uint64_t perft(const Position& pos, int depth);
int evaluate(const Position& pos);
```

Implement `generate_moves` (in any order you like), `apply_move` (copy out a new `Position`; no undo), `perft`, and `evaluate`. Write all of them with bitboards: precompute ray and neighbor tables, and use `countr_zero` / `countl_zero` to find the nearest blocker. Board edges are the classic trap: a step `east from the g file` must not wrap around to the `a` file. `perft` (from the start position) at depths 1–5 is 39, 1424, 56548, 2122948, and 82403728, which you can use for debugging.

`generate_moves` and `evaluate` must each be at least 4 times faster than the array implementation (compared within the same executable). `perft` doesn't need to actually expand its leaf nodes: whatever you can count with a popcount, don't enumerate.

## Stage 2 — Minimax and alpha-beta

```cpp
// search.hpp
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

    SearchStats& operator+=(const SearchStats& other);  // provided
};

struct SearchResult {
    int score = 0;
    Move best = kNoMove;  // kNoMove for a depth-zero or finished root
    SearchStats stats;
};

int minimax(const Position& pos, int depth, std::uint64_t* nodes = nullptr);

class Engine {
public:
    explicit Engine(const SearchOptions& options = {});

    SearchResult search(const Position& pos, int depth, int alpha = -kScoreInfinity,
                        int beta = kScoreInfinity);
};
```

`minimax(pos, depth, nodes)`: negamax without pruning, as the baseline. At depth 0 or in a finished game, return `evaluate`.

`Engine::search(pos, depth, alpha, beta)`: fail-soft alpha-beta (in negamax form). With a full window, the score **must equal minimax**; with a narrow window, the return value must follow the fail-soft contract (if the true value is ≤ α, return a value ≤ α; if it is ≥ β, return a value ≥ β). `SearchResult.best` must be a legal move that really achieves that score (return `kNoMove` at depth 0 or in a finished game). `stats.nodes` counts every node entered (including the root and the leaves).

## Stage 3 — Zobrist hashing and the transposition table

```cpp
// movegen.hpp
std::uint64_t zobrist_key(const Position& pos);  // full hash of the board and side to move, ignoring pos.key

// tt.hpp
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
    explicit TranspositionTable(int log2_entries);

    std::optional<TTEntry> probe(std::uint64_t key) const;
    void store(const TTEntry& entry);
    void clear();
    std::size_t size() const;
};

// search.hpp, in class Engine
SearchResult iterative(const Position& pos, int max_depth);
void clear();  // forgets the table and any move-ordering history
```

- `zobrist_key(pos)` computes the full hash, which must depend on the kind of piece (plow or furrow), the square, the owner, and the side to move. `apply_move` must maintain `key` incrementally. The root's `key` may be 0; `Engine` computes the first one itself.
- `TranspositionTable` uses `2^bits` slots (choose your own replacement policy), and `probe` may **only** return an entry whose key matches exactly (however small the table, it must never return another position's data).
- When `Engine` has `use_tt` turned on, use the TT's bounds to cut the search off, but **only when the entry's depth equals the current depth**. Real engines also accept deeper entries, but this exercise's acceptance criterion is "the score must equal minimax at the same depth", so you have to think the difference through (see the interview follow-ups). When the TT has no complete cutoff data, the best move it stores can still be used for ordering (Stage 4).
- `iterative(pos, d)`: run depths 1..d, and return the last iteration's result together with the accumulated stats.

## Stage 4 — Move ordering (judged by node counts)

When `use_ordering` is on, alpha-beta tries moves in this order: the TT best move, moves that flip more furrows (the analogue of captures), killer moves, and history. Of the targets to pass, the first two are judged only on node counts and statistics (fully deterministic), and the last one is a speed ratio within the same executable:

- At least 70% fewer nodes than plain alpha-beta, and with the TT added on top, at least 5 times fewer (relative to plain).
- Most cutoffs happen on the first move tried (first-move cutoff rate ≥ 85%).
- The tuned engine, with everything turned on, is at least 5 times faster than plain alpha-beta.

After building with `./practice test 1411`, running `build/release/bin/6.172/1411-furrow-engine/run --perf-only --stage 4 --verbose` prints an `engine report`: nodes, EBF, first-move cutoff rate, and TT hit rate. **Tune by watching the numbers**, not by gut feeling.

## Stage 5 — Fork-join search

```cpp
// search.hpp, in class Engine
SearchResult parallel(const Position& pos, int depth);
```

`Engine::parallel(pos, depth)` searches in parallel with `practice::fork2`. Its score must match what `search` returns (for any worker count set with `practice::set_workers`), and ThreadSanitizer verifies that it has zero data races. Search the first child first to establish alpha (Young Brothers Wait), and only then search the remaining siblings in parallel; then add alpha sharing and an abort mechanism, so that when a cutoff happens, the subtrees that are no longer needed get stopped.

TSan doesn't accept "benign races": the TT must use atomics (with XOR verification to throw away torn entries) or some other approach, and the same goes for shared counters and the history table. To pass: 8 workers must be at least 1.5 times faster, and the total node count must stay within 2.2 times the serial count with 4 workers and within 2.8 times with 8 workers (this measures speculative waste).

## Interview follow-ups

Once you're done, answer without looking at your code:

1. Real engines accept "deeper TT entries" for cutoffs. What are the benefits and the costs? Why does this conflict with this exercise's acceptance criterion that "the score equals fixed-depth minimax"?
2. Why does Young Brothers Wait search the first child serially? If you make everything parallel instead, how much does the node count swell in your measurements, and why?
3. Real engines' TTs often rely on data races to work. What does the C++ standard say about that? What did you use to make yours clean under TSan, and what did it cost?
