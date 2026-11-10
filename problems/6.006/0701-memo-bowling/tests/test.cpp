#include <cstdint>
#include <span>
#include <stdexcept>
#include <utility>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Exhaustive recursion over every way to treat pin i: skip, alone, or with the next pin.
std::int64_t brute_force(std::span<const int> pins, std::size_t i = 0) {
    if (i >= pins.size()) {
        return 0;
    }
    std::int64_t best = std::max(brute_force(pins, i + 1), pins[i] + brute_force(pins, i + 1));
    if (i + 1 < pins.size()) {
        best = std::max(best, static_cast<std::int64_t>(pins[i]) * pins[i + 1] +
                                  brute_force(pins, i + 2));
    }
    return best;
}

// Validates a plan and returns the score it earns.
std::int64_t score_of(std::span<const int> pins, const std::vector<Throw>& plan) {
    std::vector<bool> used(pins.size(), false);
    std::int64_t score = 0;
    for (const Throw& hit : plan) {
        REQUIRE(hit.first < pins.size());
        REQUIRE(hit.last < pins.size());
        REQUIRE(hit.last == hit.first || hit.last == hit.first + 1);
        REQUIRE(!used[hit.first]);
        used[hit.first] = true;
        if (hit.last == hit.first) {
            score += pins[hit.first];
        } else {
            REQUIRE(!used[hit.last]);
            used[hit.last] = true;
            score += static_cast<std::int64_t>(pins[hit.first]) * pins[hit.last];
        }
    }
    return score;
}

}  // namespace

TEST(1, "a relation without recursion is computed once") {
    Memo<int, int> memo;
    int calls = 0;
    const auto relation = [&](int key, auto&) {
        ++calls;
        return key * 2;
    };
    CHECK_EQ(memo.get(21, relation), 42);
    CHECK_EQ(memo.get(21, relation), 42);
    CHECK_EQ(calls, 1);
    CHECK_EQ(memo.size(), 1);
}

TEST(1, "fibonacci(90) computes each subproblem exactly once") {
    Memo<int, std::int64_t> memo;
    int calls = 0;
    const auto counted = [&](int n, auto& recurse) -> std::int64_t {
        ++calls;
        return n < 2 ? n : recurse(n - 1) + recurse(n - 2);
    };
    CHECK_EQ(memo.get(90, counted), std::int64_t{2880067194370816120});
    CHECK_EQ(calls, 91);
    CHECK_EQ(memo.size(), 91);
}

TEST(1, "references stay valid after many later inserts") {
    Memo<int, std::int64_t> memo;
    const auto identity = [](int key, auto&) -> std::int64_t { return key; };
    const std::int64_t& first = memo.get(1, identity);
    const std::int64_t* address = &first;
    for (int i = 2; i < 20'000; ++i) {
        memo.get(i, identity);
    }
    CHECK_EQ(first, 1);
    CHECK(address == &memo.get(1, identity));
}

TEST(1, "composite keys work with a custom hash") {
    struct PairHash {
        std::size_t operator()(const std::pair<int, int>& p) const {
            return std::hash<std::int64_t>{}((static_cast<std::int64_t>(p.first) << 32) ^ p.second);
        }
    };
    Memo<std::pair<int, int>, std::int64_t, PairHash> memo;
    const auto lattice_paths = [](const std::pair<int, int>& at, auto& recurse) -> std::int64_t {
        const auto [right, down] = at;
        if (right == 0 || down == 0) {
            return 1;
        }
        return recurse({right - 1, down}) + recurse({right, down - 1});
    };
    CHECK_EQ(memo.get({16, 16}, lattice_paths), std::int64_t{601080390});
    CHECK_EQ(memo.size(), 17 * 17 - 1);  // (0, 0) is never reached: both of its callers are base cases
}

TEST(1, "a cycle throws logic_error and leaves nothing half-done") {
    Memo<int, int> memo;
    const auto looping = [](int key, auto& recurse) { return recurse(key == 1 ? 2 : 1); };
    CHECK_THROWS_AS(memo.get(1, looping), std::logic_error);
    CHECK_EQ(memo.size(), 0);

    const auto fixed = [](int key, auto&) { return key; };
    CHECK_EQ(memo.get(1, fixed), 1);
    CHECK_EQ(memo.get(2, fixed), 2);
}

TEST(1, "a self-loop counts as a cycle") {
    Memo<int, int> memo;
    const auto self_loop = [](int key, auto& recurse) { return recurse(key); };
    CHECK_THROWS_AS(memo.get(5, self_loop), std::logic_error);
}

TEST(1, "an exception keeps finished work and leaves the failing key retriable") {
    Memo<int, int> memo;
    bool fail = true;
    const auto relation = [&](int key, auto& recurse) -> int {
        if (key == 0) {
            return 0;
        }
        const int below = recurse(key - 1);
        if (key == 3 && fail) {
            throw std::runtime_error("transient");
        }
        return below + 1;
    };
    CHECK_THROWS_AS(memo.get(5, relation), std::runtime_error);
    // Keys 0, 1 and 2 finished before the failure.
    CHECK_EQ(memo.size(), 3);

    fail = false;
    CHECK_EQ(memo.get(5, relation), 5);
    CHECK_EQ(memo.size(), 6);
}

TEST(2, "no pins scores zero") {
    CHECK_EQ(bowling_max_score({}), 0);
}

TEST(2, "a single pin is hit only when that gains something") {
    const std::vector<int> positive{4};
    const std::vector<int> negative{-4};
    CHECK_EQ(bowling_max_score(positive), 4);
    CHECK_EQ(bowling_max_score(negative), 0);
}

TEST(2, "two negative pins are worth hitting together") {
    const std::vector<int> pins{-3, -4};
    CHECK_EQ(bowling_max_score(pins), 12);
}

TEST(2, "an odd run of negative pins leaves the least useful one") {
    const std::vector<int> pins{-2, -3, -4};
    CHECK_EQ(bowling_max_score(pins), 12);
}

TEST(2, "ones are better hit separately") {
    const std::vector<int> pins{1, 1, 1};
    CHECK_EQ(bowling_max_score(pins), 3);
}

TEST(2, "pair of large pins does not overflow 32-bit arithmetic") {
    const std::vector<int> pins(200'000, 1'000'000);
    CHECK_EQ(bowling_max_score(pins), std::int64_t{100'000} * 1'000'000'000'000);
}

TEST(2, "random small inputs match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        const auto pins = rng.ints(static_cast<std::size_t>(rng.uniform(0, 9)), -6, 6);
        INFO("trial ", trial, " pins ", pins);
        REQUIRE_EQ(bowling_max_score(pins), brute_force(pins));
    }
}

PERF(2, "two million pins in linear time", 400) {
    Rng rng;
    const auto pins = rng.ints(2'000'000, -1000, 1000);
    CHECK_GT(bowling_max_score(pins), 0);
}

TEST(3, "no pins means an empty plan") {
    CHECK(bowling_plan({}).empty());
}

TEST(3, "the plan pairs two negatives") {
    const std::vector<int> pins{-5, -2};
    const auto plan = bowling_plan(pins);
    REQUIRE_EQ(plan.size(), 1);
    CHECK_EQ(plan[0].first, 0);
    CHECK_EQ(plan[0].last, 1);
}

TEST(3, "all non-positive isolated pins need no throws") {
    const std::vector<int> pins{-5, 0, -2};
    CHECK_EQ(score_of(pins, bowling_plan(pins)), 0);
}

TEST(3, "random plans are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 3000; ++trial) {
        const auto pins = rng.ints(static_cast<std::size_t>(rng.uniform(0, 9)), -6, 6);
        INFO("trial ", trial, " pins ", pins);
        REQUIRE_EQ(score_of(pins, bowling_plan(pins)), brute_force(pins));
    }
}

TEST(3, "plans on long inputs reach the optimal score") {
    Rng rng;
    const auto pins = rng.ints(100'000, -1000, 1000);
    CHECK_EQ(score_of(pins, bowling_plan(pins)), bowling_max_score(pins));
}

PERF(3, "one million pins in linear time", 400) {
    Rng rng;
    const auto pins = rng.ints(1'000'000, -1000, 1000);
    CHECK_GT(bowling_plan(pins).size(), 0);
}

TEST(4, "an empty stream reports zero") {
    BowlingStream stream;
    CHECK_EQ(stream.best(), 0);
}

TEST(4, "best matches the batch answer after every push") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const auto pins = rng.ints(static_cast<std::size_t>(rng.uniform(1, 60)), -100, 100);
        BowlingStream stream;
        std::vector<int> seen;
        for (const int pin : pins) {
            INFO("trial ", trial, " seen ", seen);
            stream.push(pin);
            seen.push_back(pin);
            REQUIRE_EQ(stream.best(), bowling_max_score(seen));
        }
    }
}

TEST(4, "the latest pin can still pair with the one before it") {
    BowlingStream stream;
    stream.push(-3);
    CHECK_EQ(stream.best(), 0);
    stream.push(-4);
    CHECK_EQ(stream.best(), 12);
}

TEST(4, "pushing never allocates") {
    Rng rng;
    const auto pins = rng.ints(100'000, -1000, 1000);
    heap_meter::Scope scope;
    BowlingStream stream;
    for (const int pin : pins) {
        stream.push(pin);
    }
    CHECK_EQ(scope.peak_bytes(), 0);
    CHECK_GT(stream.best(), 0);
}

PERF(4, "two million pushes stay O(1) each", 300) {
    Rng rng;
    const auto pins = rng.ints(2'000'000, -1000, 1000);
    BowlingStream stream;
    for (const int pin : pins) {
        stream.push(pin);
    }
    CHECK_GT(stream.best(), 0);
}
