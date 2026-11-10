// search.hpp — negamax alpha-beta with transposition table, move ordering and fork-join search.
#pragma once

#include <algorithm>
#include <array>
#include <atomic>
#include <cstdint>
#include <mutex>
#include <utility>
#include <vector>

#include "board.hpp"
#include "forkjoin.hpp"
#include "movegen.hpp"
#include "tt.hpp"

struct SearchOptions {
    bool use_tt = false;
    bool use_ordering = false;
    int tt_bits = 16;
};

struct SearchStats {
    std::uint64_t nodes = 0;
    std::uint64_t cutoffs = 0;
    std::uint64_t first_move_cutoffs = 0;
    std::uint64_t tt_probes = 0;
    std::uint64_t tt_hits = 0;
    std::uint64_t tt_cutoffs = 0;

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
    Move best = kNoMove;
    SearchStats stats;
};

// Plain negamax: no pruning and no heuristics. Counts every node.
inline int minimax(const Position& pos, int depth, std::uint64_t* nodes = nullptr) {
    if (nodes != nullptr) {
        ++*nodes;
    }
    if (depth == 0) {
        return evaluate(pos);
    }
    MoveList moves;
    generate_moves(pos, moves);
    if (moves.empty()) {
        return evaluate(pos);
    }
    int best = -kScoreInfinity;
    for (const Move move : moves) {
        best = std::max(best, -minimax(apply_move(pos, move), depth - 1, nodes));
    }
    return best;
}

class Engine {
public:
    explicit Engine(const SearchOptions& options = {})
        : options_(options), table_(options.use_tt ? options.tt_bits : 1) {
        contexts_.resize(1);
    }

    SearchResult search(const Position& pos, int depth, int alpha = -kScoreInfinity,
                        int beta = kScoreInfinity) {
        Context& context = contexts_[0];
        context.reset_stats();
        const Position keyed = with_key(pos);
        Move best = kNoMove;
        const int score = negamax(keyed, depth, alpha, beta, 0, context, nullptr, &best);
        return {score, best, context.stats};
    }

    // Iterative deepening up to max_depth. Stats accumulate over every iteration.
    SearchResult iterative(const Position& pos, int max_depth) {
        SearchResult last;
        SearchStats total;
        for (int depth = 1; depth <= max_depth; ++depth) {
            last = search(pos, depth);
            total += last.stats;
        }
        if (max_depth <= 0) {
            last = search(pos, 0);
            total = last.stats;
        }
        last.stats = total;
        return last;
    }

    // Parallel search of one depth. Scores match search(); stats are merged across workers.
    SearchResult parallel(const Position& pos, int depth) {
        contexts_.resize(std::max<std::size_t>(practice::workers(), 1));
        for (Context& context : contexts_) {
            context.reset_stats();
        }
        const Position keyed = with_key(pos);
        Move best = kNoMove;
        const AbortFlag root{};
        const int score = search_parallel(keyed, depth, -kScoreInfinity, kScoreInfinity, 0, root, &best);
        SearchResult result{score, best, {}};
        for (const Context& context : contexts_) {
            result.stats += context.stats;
        }
        return result;
    }

    void clear() {
        table_.clear();
        for (Context& context : contexts_) {
            context.clear_heuristics();
        }
    }

    TranspositionTable& table() { return table_; }

private:
    static constexpr int kMaxPly = 64;
    static constexpr int kKillers = 2;
    // Below this remaining depth a subtree is searched serially.
    static constexpr int kParallelDepth = 3;

    struct Context {
        SearchStats stats;
        std::array<std::array<Move, kKillers>, kMaxPly> killers{};
        std::array<std::array<std::array<std::int32_t, kSquares>, kSquares>, 2> history{};

        void reset_stats() { stats = {}; }
        void clear_heuristics() {
            killers = {};
            history = {};
        }
    };

    // Set when some ancestor's sibling produced a cutoff: the work beneath it is no longer needed.
    struct AbortFlag {
        std::atomic<bool> stop{false};
        const AbortFlag* parent = nullptr;

        bool triggered() const {
            for (const AbortFlag* flag = this; flag != nullptr; flag = flag->parent) {
                if (flag->stop.load(std::memory_order_relaxed)) {
                    return true;
                }
            }
            return false;
        }
    };

    int order_key(const Position& pos, Move move, Move tt_move, int ply, const Context& context) const {
        if (move == tt_move) {
            return 1 << 24;
        }
        const int flips = flips_of(pos, move);
        int key = flips << 20;
        const auto& killers = context.killers[static_cast<std::size_t>(std::min(ply, kMaxPly - 1))];
        if (flips == 0 && move == killers[0]) {
            key += 1 << 18;
        } else if (flips == 0 && move == killers[1]) {
            key += 1 << 17;
        }
        return key + history_of(pos, move, context);
    }

    static int history_of(const Position& pos, Move move, const Context& context) {
        return context.history[static_cast<std::size_t>(pos.side)][static_cast<std::size_t>(move.from)]
                              [static_cast<std::size_t>(move.to)];
    }

    void order_moves(MoveList& moves, const Position& pos, Move tt_move, int ply, const Context& context) const {
        if (!options_.use_ordering) {
            if (tt_move) {
                auto* found = std::find(moves.begin(), moves.end(), tt_move);
                if (found != moves.end()) {
                    std::iter_swap(moves.begin(), found);
                }
            }
            return;
        }
        std::array<int, MoveList::kCapacity> keys{};
        for (int i = 0; i < moves.size(); ++i) {
            keys[static_cast<std::size_t>(i)] = order_key(pos, moves[i], tt_move, ply, context);
        }
        for (int i = 1; i < moves.size(); ++i) {
            const Move move = moves[i];
            const int key = keys[static_cast<std::size_t>(i)];
            int j = i - 1;
            while (j >= 0 && keys[static_cast<std::size_t>(j)] < key) {
                moves[j + 1] = moves[j];
                keys[static_cast<std::size_t>(j + 1)] = keys[static_cast<std::size_t>(j)];
                --j;
            }
            moves[j + 1] = move;
            keys[static_cast<std::size_t>(j + 1)] = key;
        }
    }

    void remember_cutoff(const Position& pos, Move move, int depth, int ply, int move_index, Context& context) {
        ++context.stats.cutoffs;
        if (move_index == 0) {
            ++context.stats.first_move_cutoffs;
        }
        if (!options_.use_ordering) {
            return;
        }
        auto& killers = context.killers[static_cast<std::size_t>(std::min(ply, kMaxPly - 1))];
        if (flips_of(pos, move) == 0 && killers[0] != move) {
            killers[1] = killers[0];
            killers[0] = move;
        }
        auto& slot = context.history[static_cast<std::size_t>(pos.side)][static_cast<std::size_t>(move.from)]
                                    [static_cast<std::size_t>(move.to)];
        slot = std::min(slot + depth * depth, 1 << 16);
    }

    // Consults the table. Returns true if the entry settles this node, with *score set. Adjusts
    // alpha and beta and reports the stored best move for ordering.
    bool probe_table(std::uint64_t key, int depth, int& alpha, int& beta, Move& tt_move, int* score, Context& context) {
        ++context.stats.tt_probes;
        const auto entry = table_.probe(key);
        if (!entry) {
            return false;
        }
        ++context.stats.tt_hits;
        tt_move = entry->best;
        if (entry->depth != depth) {
            return false;
        }
        if (entry->bound == Bound::Exact) {
            *score = entry->score;
            ++context.stats.tt_cutoffs;
            return true;
        }
        if (entry->bound == Bound::Lower) {
            alpha = std::max(alpha, entry->score);
        } else {
            beta = std::min(beta, entry->score);
        }
        if (alpha >= beta) {
            *score = entry->score;
            ++context.stats.tt_cutoffs;
            return true;
        }
        return false;
    }

    void store_table(std::uint64_t key, int depth, int score, int alpha, int beta, Move best) {
        const Bound bound = score <= alpha ? Bound::Upper : (score >= beta ? Bound::Lower : Bound::Exact);
        table_.store({key, score, depth, bound, best});
    }

    // abort: an optional flag polled at interior nodes so that parallel search can stop early.
    int negamax(const Position& pos, int depth, int alpha, int beta, int ply, Context& context,
                const AbortFlag* abort, Move* root_best) {
        ++context.stats.nodes;
        if (depth == 0) {
            return evaluate(pos);
        }
        MoveList moves;
        generate_moves(pos, moves);
        if (moves.empty()) {
            return evaluate(pos);
        }

        Move tt_move = kNoMove;
        if (options_.use_tt) {
            int stored = 0;
            if (probe_table(pos.key, depth, alpha, beta, tt_move, &stored, context)) {
                if (root_best != nullptr) {
                    *root_best = tt_move;
                }
                return stored;
            }
        }
        order_moves(moves, pos, tt_move, ply, context);

        const int window_alpha = alpha;
        int best_score = -kScoreInfinity;
        Move best_move = moves[0];
        for (int i = 0; i < moves.size(); ++i) {
            const Move move = moves[i];
            const int score = -negamax(apply_move(pos, move), depth - 1, -beta, -alpha, ply + 1, context, abort, nullptr);
            if (abort != nullptr && abort->triggered()) {
                return best_score;
            }
            if (score > best_score) {
                best_score = score;
                best_move = move;
            }
            alpha = std::max(alpha, score);
            if (alpha >= beta) {
                remember_cutoff(pos, move, depth, ply, i, context);
                break;
            }
        }
        if (options_.use_tt) {
            store_table(pos.key, depth, best_score, window_alpha, beta, best_move);
        }
        if (root_best != nullptr) {
            *root_best = best_move;
        }
        return best_score;
    }

    // Young-brothers-wait: the first move is searched alone to establish a bound; the remaining
    // moves may then run in parallel because a cutoff is unlikely once the bound is known.
    int search_parallel(const Position& pos, int depth, int alpha, int beta, int ply, const AbortFlag& abort, Move* root_best) {
        Context& context = current_context();
        if (depth <= kParallelDepth) {
            return negamax(pos, depth, alpha, beta, ply, context, &abort, root_best);
        }
        ++context.stats.nodes;
        MoveList moves;
        generate_moves(pos, moves);
        if (moves.empty()) {
            return evaluate(pos);
        }

        Move tt_move = kNoMove;
        if (options_.use_tt) {
            int stored = 0;
            if (probe_table(pos.key, depth, alpha, beta, tt_move, &stored, context)) {
                if (root_best != nullptr) {
                    *root_best = tt_move;
                }
                return stored;
            }
        }
        order_moves(moves, pos, tt_move, ply, context);

        const int window_alpha = alpha;
        const int first = -search_parallel(apply_move(pos, moves[0]), depth - 1, -beta, -alpha, ply + 1, abort, nullptr);
        if (abort.triggered()) {
            return first;
        }
        if (first >= beta) {
            remember_cutoff(pos, moves[0], depth, ply, 0, current_context());
            if (root_best != nullptr) {
                *root_best = moves[0];
            }
            if (options_.use_tt) {
                store_table(pos.key, depth, first, window_alpha, beta, moves[0]);
            }
            return first;
        }

        Shared shared;
        shared.alpha.store(std::max(alpha, first), std::memory_order_relaxed);
        shared.best_score = first;
        shared.best_move = moves[0];
        shared.abort.parent = &abort;
        search_siblings(pos, moves, 1, moves.size(), depth, beta, ply, shared);

        if (abort.triggered()) {
            return shared.best_score;
        }
        if (root_best != nullptr) {
            *root_best = shared.best_move;
        }
        if (options_.use_tt) {
            store_table(pos.key, depth, shared.best_score, window_alpha, beta, shared.best_move);
        }
        return shared.best_score;
    }

    struct Shared {
        std::atomic<int> alpha{0};
        AbortFlag abort;
        std::mutex mutex;
        int best_score = -kScoreInfinity;
        Move best_move = kNoMove;
    };

    void search_siblings(const Position& pos, const MoveList& moves, int begin, int end, int depth, int beta, int ply, Shared& shared) {
        if (end - begin > 1) {
            const int middle = begin + (end - begin) / 2;
            practice::fork2([&] { search_siblings(pos, moves, begin, middle, depth, beta, ply, shared); },
                            [&] { search_siblings(pos, moves, middle, end, depth, beta, ply, shared); });
            return;
        }
        if (shared.abort.triggered()) {
            return;
        }
        const Move move = moves[begin];
        const int window = shared.alpha.load(std::memory_order_relaxed);
        const int score = -search_parallel(apply_move(pos, move), depth - 1, -beta, -window, ply + 1, shared.abort, nullptr);
        if (shared.abort.triggered()) {
            return;
        }
        Context& context = current_context();
        const std::lock_guard lock(shared.mutex);
        if (score > shared.best_score) {
            shared.best_score = score;
            shared.best_move = move;
        }
        if (score > shared.alpha.load(std::memory_order_relaxed)) {
            shared.alpha.store(score, std::memory_order_relaxed);
        }
        if (score >= beta) {
            remember_cutoff(pos, move, depth, ply, begin, context);
            shared.abort.stop.store(true, std::memory_order_relaxed);
        }
    }

    Context& current_context() { return contexts_[practice::worker_index()]; }

    SearchOptions options_;
    TranspositionTable table_;
    std::vector<Context> contexts_;
};
