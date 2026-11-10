#include <algorithm>
#include <bit>
#include <cstdint>
#include <limits>
#include <map>
#include <numeric>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Values = std::vector<int>;

// Exhaustive game tree on a line. mover 0 maximizes player 0's total, mover 1 minimizes it.
std::int64_t line_minimax(const Values& values, std::size_t first, std::size_t last, int mover) {
    const std::size_t length = last - first;
    if (length == 1) {
        return mover == 0 ? values[first] : 0;
    }
    std::int64_t best = mover == 0 ? std::numeric_limits<std::int64_t>::min()
                                   : std::numeric_limits<std::int64_t>::max();
    const auto consider = [&](std::int64_t outcome) {
        best = mover == 0 ? std::max(best, outcome) : std::min(best, outcome);
    };
    for (std::size_t cut = first + 1; cut < last; ++cut) {
        std::int64_t left_sum = 0;
        for (std::size_t i = first; i < cut; ++i) {
            left_sum += values[i];
        }
        std::int64_t right_sum = 0;
        for (std::size_t i = cut; i < last; ++i) {
            right_sum += values[i];
        }
        consider((mover == 0 ? left_sum : 0) + line_minimax(values, cut, last, 1 - mover));
        consider((mover == 0 ? right_sum : 0) + line_minimax(values, first, cut, 1 - mover));
    }
    return best;
}

// Plays the ring directly from the half-plane rule, with no notion of "contiguous arcs":
// state is a bitmask of remaining slices.
class RingSimulator {
public:
    explicit RingSimulator(const Values& slices) : slices_(slices) {}

    std::int64_t solve() { return run((1u << slices_.size()) - 1, 0); }

private:
    std::int64_t run(unsigned mask, int mover) {
        if (std::popcount(mask) == 1) {
            return mover == 0 ? slices_[static_cast<std::size_t>(std::countr_zero(mask))] : 0;
        }
        const auto key = std::make_pair(mask, mover);
        if (const auto it = memo_.find(key); it != memo_.end()) {
            return it->second;
        }
        const int size = static_cast<int>(slices_.size());
        const int half = size / 2;
        std::int64_t best = mover == 0 ? std::numeric_limits<std::int64_t>::min()
                                       : std::numeric_limits<std::int64_t>::max();
        for (int angle = 0; angle < size; ++angle) {
            unsigned side = 0;
            for (int i = 0; i < half; ++i) {
                side |= 1u << ((angle + i) % size);
            }
            const unsigned eaten = mask & side;
            const unsigned rest = mask & ~side;
            if (eaten == 0 || rest == 0) {
                continue;  // the line must have uneaten slices on both sides
            }
            std::int64_t gain = 0;
            for (int i = 0; i < size; ++i) {
                if ((eaten >> i) & 1) {
                    gain += slices_[static_cast<std::size_t>(i)];
                }
            }
            const std::int64_t outcome = (mover == 0 ? gain : 0) + run(rest, 1 - mover);
            best = mover == 0 ? std::max(best, outcome) : std::min(best, outcome);
        }
        memo_[key] = best;
        return best;
    }

    const Values& slices_;
    std::map<std::pair<unsigned, int>, std::int64_t> memo_;
};

std::int64_t arc_sum(const Values& slices, Arc arc) {
    std::int64_t total = 0;
    for (std::size_t i = 0; i < arc.length; ++i) {
        total += slices[(arc.start + i) % slices.size()];
    }
    return total;
}

// Lowest total a bot can end up with from here, over every opponent line of play.
std::int64_t worst_case(const RingGame& game, const Values& slices, Arc remaining,
                        bool bot_to_move) {
    if (remaining.length == 0) {
        return 0;
    }
    const std::size_t size = slices.size();
    if (bot_to_move) {
        const Arc claim = game.best_claim(remaining);
        REQUIRE(claim.start < size);
        if (remaining.length == size) {
            REQUIRE_EQ(claim.length, size / 2);
        } else if (remaining.length == 1) {
            REQUIRE_EQ(claim.length, 1);
            REQUIRE_EQ(claim.start, remaining.start);
        } else {
            REQUIRE(claim.length >= 1);
            REQUIRE(claim.length < remaining.length);
            const bool is_prefix = claim.start == remaining.start;
            const bool is_suffix =
                claim.start == (remaining.start + remaining.length - claim.length) % size;
            REQUIRE(is_prefix || is_suffix);
        }
        const std::size_t left = remaining.length - claim.length;
        Arc next{(claim.start + claim.length) % size, left};
        if (remaining.length != size && claim.start != remaining.start) {
            next.start = remaining.start;
        }
        return arc_sum(slices, claim) + worst_case(game, slices, next, false);
    }
    if (remaining.length == 1) {
        return 0;
    }
    std::int64_t worst = std::numeric_limits<std::int64_t>::max();
    for (std::size_t cut = 1; cut < remaining.length; ++cut) {
        worst = std::min(worst, worst_case(game, slices,
                                           {(remaining.start + cut) % size, remaining.length - cut},
                                           true));
        worst = std::min(worst, worst_case(game, slices, {remaining.start, cut}, true));
    }
    return worst;
}

Values random_ring(Rng& rng, std::size_t max_half) {
    return rng.ints(2 * static_cast<std::size_t>(rng.uniform(1, static_cast<std::int64_t>(max_half))),
                    -10, 15);
}

}  // namespace

TEST(1, "no slices score nothing") {
    CHECK_EQ(line_game_value({}), 0);
}

TEST(1, "a single slice must be eaten") {
    const Values values{-3};
    CHECK_EQ(line_game_value(values), -3);
}

TEST(1, "with two slices the first player keeps the larger") {
    const Values values{2, 9};
    CHECK_EQ(line_game_value(values), 9);
}

TEST(1, "with three slices cutting the good one off first can lose it") {
    // Taking {4} or {1} alone hands the opponent the 10; claiming {4, 10} in one cut keeps it.
    const Values values{4, 10, 1};
    CHECK_EQ(line_game_value(values), 14);
}

TEST(1, "all slices are eaten even when they are all negative") {
    const Values values{-1, -2, -3};
    // The first player cannot opt out; the least bad option is to claim {-1, -2}.
    CHECK_EQ(line_game_value(values), -3);
}

TEST(1, "random lines match the exhaustive game tree") {
    Rng rng;
    for (int trial = 0; trial < 800; ++trial) {
        const Values values = rng.ints(static_cast<std::size_t>(rng.uniform(1, 9)), -10, 15);
        INFO("values ", values);
        REQUIRE_EQ(line_game_value(values), line_minimax(values, 0, values.size(), 0));
    }
}

PERF(1, "sixteen hundred slices in cubic time", 3500) {
    Rng rng;
    const Values values = rng.ints(1600, -1000, 1000);
    const std::int64_t total = std::accumulate(values.begin(), values.end(), std::int64_t{0});
    CHECK_GE(2 * line_game_value(values), total);
}

TEST(2, "an odd or empty ring is rejected") {
    const Values odd{1, 2, 3};
    CHECK_THROWS_AS(ring_game_value(odd), std::invalid_argument);
    CHECK_THROWS_AS(ring_game_value({}), std::invalid_argument);
}

TEST(2, "with two slices the first player keeps the larger") {
    const Values slices{3, 8};
    CHECK_EQ(ring_game_value(slices), 8);
}

TEST(2, "with four slices the first cut is any adjacent pair") {
    // Claim {7, 1} and the opponent gets to choose from {2, 9}; claiming {9, 7} leaves {1, 2}.
    const Values slices{7, 1, 2, 9};
    CHECK_EQ(ring_game_value(slices), 16 + 1);
}

TEST(2, "random rings match simulating the half-plane rule") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const Values slices = random_ring(rng, 6);
        INFO("slices ", slices);
        RingSimulator simulator(slices);
        REQUIRE_EQ(ring_game_value(slices), simulator.solve());
    }
}

PERF(2, "a ring of sixteen hundred slices in cubic time", 4000) {
    Rng rng;
    const Values slices = rng.ints(1600, -1000, 1000);
    const std::int64_t total = std::accumulate(slices.begin(), slices.end(), std::int64_t{0});
    CHECK_GE(2 * ring_game_value(slices), total);
}

TEST(3, "value agrees with the simulator") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const Values slices = random_ring(rng, 6);
        INFO("slices ", slices);
        RingSimulator simulator(slices);
        REQUIRE_EQ(RingGame(slices).value(), simulator.solve());
    }
}

TEST(3, "an odd ring is rejected") {
    const Values odd{1, 2, 3};
    CHECK_THROWS_AS(RingGame(odd), std::invalid_argument);
}

TEST(3, "the opening claim is exactly half of the ring") {
    const Values slices{3, -2, 8, 1, 5, -7};
    const RingGame game(slices);
    const Arc claim = game.best_claim({0, slices.size()});
    CHECK_EQ(claim.length, 3);
    CHECK_LT(claim.start, slices.size());
}

TEST(3, "with a single slice the claim is that slice") {
    const Values slices{5, 9, -1, 0};
    const RingGame game(slices);
    const Arc claim = game.best_claim({3, 1});
    CHECK_EQ(claim.start, 3);
    CHECK_EQ(claim.length, 1);
}

TEST(3, "the bot secures at least the game value against every opponent") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const Values slices = random_ring(rng, 5);
        const RingGame game(slices);
        INFO("slices ", slices);
        REQUIRE_EQ(worst_case(game, slices, {0, slices.size()}, true), game.value());
    }
}

TEST(3, "arcs that wrap around the end of the ring are handled") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const Values slices = random_ring(rng, 5);
        const std::size_t half = slices.size() / 2;
        const RingGame game(slices);
        for (std::size_t start = 0; start < slices.size(); ++start) {
            const Arc remaining{start, half};
            Values arc_values;
            for (std::size_t i = 0; i < half; ++i) {
                arc_values.push_back(slices[(start + i) % slices.size()]);
            }
            INFO("slices ", slices, " start ", start);
            const std::int64_t optimal = line_game_value(arc_values);
            REQUIRE_EQ(worst_case(game, slices, remaining, true), optimal);
        }
    }
}
