#include <algorithm>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <utility>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Records = std::vector<int>;
using Wires = std::vector<std::pair<std::size_t, std::size_t>>;

// Exhaustive search over which records remain; a move removes one record or a currently
// adjacent pair, exactly as the rules say.
std::int64_t exhaustive(const Records& records, bool must_clear) {
    const std::size_t n = records.size();
    std::vector<std::optional<std::int64_t>> memo(std::size_t{1} << n);
    const std::function<std::int64_t(unsigned)> solve = [&](unsigned remaining) -> std::int64_t {
        if (remaining == 0) {
            return 0;
        }
        if (memo[remaining].has_value()) {
            return *memo[remaining];
        }
        std::int64_t best = must_clear ? std::numeric_limits<std::int64_t>::min() : 0;
        for (std::size_t i = 0; i < n; ++i) {
            if (!((remaining >> i) & 1)) {
                continue;
            }
            best = std::max(best, records[i] + solve(remaining ^ (1u << i)));
            for (std::size_t j = i + 1; j < n; ++j) {
                if ((remaining >> j) & 1) {
                    best = std::max(best,
                                    static_cast<std::int64_t>(records[i]) * records[j] +
                                        solve(remaining ^ (1u << i) ^ (1u << j)));
                    break;  // anything past the next remaining record is not adjacent
                }
            }
        }
        memo[remaining] = best;
        return best;
    };
    return solve(n == 0 ? 0u : (1u << n) - 1);
}

std::int64_t execute(const Records& records, const std::vector<Removal>& removals) {
    std::vector<bool> alive(records.size(), true);
    std::int64_t score = 0;
    for (const Removal& removal : removals) {
        REQUIRE(removal.first < records.size());
        REQUIRE(removal.second < records.size());
        REQUIRE(alive[removal.first]);
        REQUIRE(alive[removal.second]);
        REQUIRE(removal.first <= removal.second);
        for (std::size_t k = removal.first + 1; k < removal.second; ++k) {
            REQUIRE(!alive[k]);
        }
        if (removal.first == removal.second) {
            score += records[removal.first];
            alive[removal.first] = false;
        } else {
            score += static_cast<std::int64_t>(records[removal.first]) * records[removal.second];
            alive[removal.first] = false;
            alive[removal.second] = false;
        }
    }
    return score;
}

bool on_circle_adjacent(std::size_t a, std::size_t b, std::size_t n) {
    const std::size_t gap = b - a;
    return gap == 1 || gap == n - 1;
}

bool crosses(std::pair<std::size_t, std::size_t> p, std::pair<std::size_t, std::size_t> q) {
    return (p.first < q.first && q.first < p.second && p.second < q.second) ||
           (q.first < p.first && p.first < q.second && q.second < p.second);
}

// Maximum number of valid wires, by trying every set of non-crossing pairs.
std::size_t exhaustive_wires(const std::vector<char>& sources) {
    const std::size_t n = sources.size();
    std::size_t best = 0;
    std::vector<bool> used(n, false);
    Wires chosen;
    const std::function<void(std::size_t)> extend = [&](std::size_t i) {
        best = std::max(best, chosen.size());
        for (std::size_t a = i; a < n; ++a) {
            if (used[a]) {
                continue;
            }
            for (std::size_t b = a + 1; b < n; ++b) {
                if (used[b] || sources[a] == sources[b] || on_circle_adjacent(a, b, n)) {
                    continue;
                }
                if (std::ranges::any_of(chosen, [&](auto wire) { return crosses(wire, {a, b}); })) {
                    continue;
                }
                used[a] = used[b] = true;
                chosen.emplace_back(a, b);
                extend(a + 1);
                chosen.pop_back();
                used[a] = used[b] = false;
            }
        }
    };
    extend(0);
    return best;
}

void check_wires(const std::vector<char>& sources, const Wires& wires) {
    const std::size_t n = sources.size();
    std::vector<bool> used(n, false);
    for (const auto& [a, b] : wires) {
        REQUIRE(a < b);
        REQUIRE(b < n);
        REQUIRE(!used[a]);
        REQUIRE(!used[b]);
        REQUIRE(sources[a] != sources[b]);
        REQUIRE(!on_circle_adjacent(a, b, n));
        used[a] = used[b] = true;
    }
    for (std::size_t i = 0; i < wires.size(); ++i) {
        for (std::size_t j = i + 1; j < wires.size(); ++j) {
            REQUIRE(!crosses(wires[i], wires[j]));
        }
    }
}

// Wires takes std::span<const bool>, which needs contiguous bool storage.
std::vector<char> as_chars(const std::vector<bool>& bits) {
    return std::vector<char>(bits.begin(), bits.end());
}

Wires wires_for(const std::vector<bool>& bits) {
    const std::unique_ptr<bool[]> storage(new bool[bits.size()]);
    for (std::size_t i = 0; i < bits.size(); ++i) {
        storage[i] = bits[i];
    }
    return max_noncrossing_wires(std::span<const bool>(storage.get(), bits.size()));
}

}  // namespace

TEST(1, "no records score nothing") {
    CHECK_EQ(max_compaction_score({}), 0);
}

TEST(1, "a negative record can be left alone") {
    const Records records{-4};
    CHECK_EQ(max_compaction_score(records), 0);
}

TEST(1, "adjacent records can pair") {
    const Records records{2, 3};
    CHECK_EQ(max_compaction_score(records), 6);
}

TEST(1, "two records separated by another can pair after it is removed") {
    const Records records{-5, -1, -5};
    // Remove -1 first (costs 1), then the outer pair is adjacent and scores 25.
    CHECK_EQ(max_compaction_score(records), 24);
}

TEST(1, "nested pairs combine") {
    const Records records{2, 3, 3, 2};
    // Inner pair 3 * 3, then the outer pair 2 * 2.
    CHECK_EQ(max_compaction_score(records), 13);
}

TEST(1, "random records match the exhaustive game") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Records records = rng.ints(static_cast<std::size_t>(rng.uniform(0, 11)), -9, 9);
        INFO("records ", records);
        REQUIRE_EQ(max_compaction_score(records), exhaustive(records, false));
    }
}

PERF(1, "eight hundred records in cubic time", 1500) {
    Rng rng;
    const Records records = rng.ints(800, -1000, 1000);
    CHECK_GT(max_compaction_score(records), 0);
}

TEST(2, "every record must go even if it hurts") {
    const Records records{-5};
    CHECK_EQ(max_full_compaction_score(records), -5);
}

TEST(2, "an empty sequence is already clear") {
    CHECK_EQ(max_full_compaction_score({}), 0);
}

TEST(2, "pairing can rescue negatives") {
    const Records records{-3, -4};
    CHECK_EQ(max_full_compaction_score(records), 12);
}

TEST(2, "random records match the exhaustive game") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Records records = rng.ints(static_cast<std::size_t>(rng.uniform(0, 11)), -9, 9);
        INFO("records ", records);
        REQUIRE_EQ(max_full_compaction_score(records), exhaustive(records, true));
    }
}

TEST(3, "an empty plan for no records") {
    const CompactionPlan plan = compaction_plan({});
    CHECK_EQ(plan.score, 0);
    CHECK(plan.removals.empty());
}

TEST(3, "the outer pair must come after its inside") {
    const Records records{2, 3, 3, 2};
    const CompactionPlan plan = compaction_plan(records);
    CHECK_EQ(plan.score, 13);
    CHECK_EQ(execute(records, plan.removals), 13);
}

TEST(3, "singles are allowed in the plan") {
    const Records records{1, 1, 1};
    const CompactionPlan plan = compaction_plan(records);
    CHECK_EQ(plan.score, 3);
    CHECK_EQ(execute(records, plan.removals), 3);
}

TEST(3, "random plans can really be executed and earn the optimum") {
    Rng rng;
    for (int trial = 0; trial < 1500; ++trial) {
        const Records records = rng.ints(static_cast<std::size_t>(rng.uniform(0, 11)), -9, 9);
        INFO("records ", records);
        const CompactionPlan plan = compaction_plan(records);
        REQUIRE_EQ(plan.score, exhaustive(records, false));
        REQUIRE_EQ(execute(records, plan.removals), plan.score);
    }
}

TEST(4, "fewer than three ports cannot be wired") {
    CHECK(wires_for({}).empty());
    CHECK(wires_for({true, false}).empty());
}

TEST(4, "neighbors on the circle can never be wired") {
    CHECK(wires_for({true, false, true}).empty());
}

TEST(4, "the opposite ports of a square can be wired") {
    const std::vector<bool> ports{true, true, false, false};
    const auto wires = wires_for(ports);
    REQUIRE_EQ(wires.size(), 1);
    check_wires(as_chars(ports), wires);
}

TEST(4, "wires must not cross") {
    const std::vector<bool> ports{true, false, false, true, true, false};
    const auto wires = wires_for(ports);
    check_wires(as_chars(ports), wires);
    CHECK_EQ(wires.size(), exhaustive_wires(as_chars(ports)));
}

TEST(4, "the wrap-around neighbors cannot be wired") {
    // The only source/unit pair that is not adjacent along the line is (0, 3), but on the circle
    // those two ports are neighbors as well.
    const std::vector<bool> ports{true, false, true, false};
    CHECK(wires_for(ports).empty());
}

TEST(4, "random boards match trying every set of wires") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 11));
        std::vector<bool> ports(n);
        for (std::size_t i = 0; i < n; ++i) {
            ports[i] = rng.coin();
        }
        const auto chars = as_chars(ports);
        INFO("ports ", ports);
        const auto wires = wires_for(ports);
        check_wires(chars, wires);
        REQUIRE_EQ(wires.size(), exhaustive_wires(chars));
    }
}

PERF(4, "six hundred ports in cubic time", 1500) {
    Rng rng;
    std::vector<bool> ports(600);
    for (std::size_t i = 0; i < 600; ++i) {
        ports[i] = rng.coin();
    }
    CHECK_GT(wires_for(ports).size(), 100);
}
