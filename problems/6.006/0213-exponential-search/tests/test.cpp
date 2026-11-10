#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

// Answers relative to a hidden target, counts probes and rejects out-of-range positions.
class Oracle {
public:
    Oracle(std::int64_t target, std::int64_t low, std::int64_t high)
        : target_(target), low_(low), high_(high) {}

    Direction operator()(std::int64_t position) {
        ++probes_;
        CHECK(position >= low_ && position < high_);
        if (position == target_) {
            return Direction::Here;
        }
        return position < target_ ? Direction::Greater : Direction::Less;
    }

    int probes() const { return probes_; }

private:
    std::int64_t target_;
    std::int64_t low_;
    std::int64_t high_;
    int probes_ = 0;
};

int log2_ceil(double value) { return static_cast<int>(std::ceil(std::log2(value))); }

}  // namespace

TEST(1, "finds every target in a small range") {
    for (std::int64_t n = 1; n <= 64; ++n) {
        for (std::int64_t target = 0; target < n; ++target) {
            Oracle oracle(target, 0, n);
            INFO("n=", n, " target=", target);
            REQUIRE_EQ(locate_in_range(0, n, std::ref(oracle)), target);
            REQUIRE(oracle.probes() <= log2_ceil(static_cast<double>(n) + 1) + 1);
        }
    }
}

TEST(1, "ranges need not start at zero") {
    Oracle oracle(105, 100, 110);
    CHECK_EQ(locate_in_range(100, 110, std::ref(oracle)), 105);
}

TEST(1, "a single-element range probes once") {
    Oracle oracle(7, 7, 8);
    CHECK_EQ(locate_in_range(7, 8, std::ref(oracle)), 7);
    CHECK_EQ(oracle.probes(), 1);
}

TEST(1, "huge ranges stay logarithmic") {
    const std::int64_t n = std::int64_t{1} << 60;
    for (std::int64_t target : {std::int64_t{0}, n - 1, n / 2, n / 3}) {
        Oracle oracle(target, 0, n);
        CHECK_EQ(locate_in_range(0, n, std::ref(oracle)), target);
        CHECK_LE(oracle.probes(), 62);
    }
}

TEST(2, "finds every target near the start") {
    for (std::int64_t n = 1; n <= 200; ++n) {
        for (std::int64_t target = 0; target < n; ++target) {
            Oracle oracle(target, 0, n);
            INFO("n=", n, " target=", target);
            REQUIRE_EQ(locate_from_start(n, std::ref(oracle)), target);
        }
    }
}

TEST(2, "probes depend on the target, not on n") {
    const std::int64_t n = std::int64_t{1} << 60;
    for (std::int64_t target : {std::int64_t{0}, std::int64_t{1}, std::int64_t{2}, std::int64_t{1'000}, std::int64_t{1'000'000}}) {
        Oracle oracle(target, 0, n);
        INFO("target=", target);
        CHECK_EQ(locate_from_start(n, std::ref(oracle)), target);
        CHECK_LE(oracle.probes(), 3 * log2_ceil(static_cast<double>(target) + 2) + 4);
    }
}

TEST(2, "targets near the far end are still found correctly") {
    const std::int64_t n = 1'000'000;
    Oracle oracle(n - 1, 0, n);
    CHECK_EQ(locate_from_start(n, std::ref(oracle)), n - 1);
}

TEST(2, "random targets within the probe budget") {
    Rng rng;
    for (int round = 0; round < 20'000; ++round) {
        const std::int64_t n = rng.uniform(1, std::int64_t{1} << 50);
        const std::int64_t magnitude = rng.uniform(0, 40);
        const std::int64_t target = std::min(n - 1, (std::int64_t{1} << magnitude) + rng.uniform(0, 3));
        Oracle oracle(target, 0, n);
        INFO("n=", n, " target=", target);
        REQUIRE_EQ(locate_from_start(n, std::ref(oracle)), target);
        REQUIRE(oracle.probes() <= 3 * log2_ceil(static_cast<double>(target) + 2) + 4);
    }
}

TEST(3, "finds every target in small ranges") {
    for (std::int64_t n = 1; n <= 150; ++n) {
        for (std::int64_t target = 0; target < n; ++target) {
            Oracle oracle(target, 0, n);
            INFO("n=", n, " target=", target);
            REQUIRE_EQ(locate_two_sided(n, std::ref(oracle)), target);
        }
    }
}

TEST(3, "targets near either end cost only O(log k)") {
    const std::int64_t n = std::int64_t{1} << 60;
    for (std::int64_t k : {std::int64_t{0}, std::int64_t{1}, std::int64_t{5}, std::int64_t{40}, std::int64_t{1'000}, std::int64_t{1'000'000}}) {
        for (const std::int64_t target : {k, n - 1 - k}) {
            Oracle oracle(target, 0, n);
            INFO("k=", k, " target=", target);
            REQUIRE_EQ(locate_two_sided(n, std::ref(oracle)), target);
            CHECK_LE(oracle.probes(), 5 * log2_ceil(static_cast<double>(k) + 2) + 10);
        }
    }
}

TEST(3, "targets in the middle are found") {
    const std::int64_t n = 1'000'000;
    for (std::int64_t target = 0; target < n; target += 12'345) {
        Oracle oracle(target, 0, n);
        REQUIRE_EQ(locate_two_sided(n, std::ref(oracle)), target);
    }
}

TEST(3, "random targets within the probe budget") {
    Rng rng;
    for (int round = 0; round < 20'000; ++round) {
        const std::int64_t n = rng.uniform(2, std::int64_t{1} << 50);
        const std::int64_t half = n / 2;
        const std::int64_t k = std::min(half, (std::int64_t{1} << rng.uniform(0, 40)) + rng.uniform(0, 3));
        const std::int64_t target = rng.coin() ? k : n - 1 - k;
        Oracle oracle(target, 0, n);
        INFO("n=", n, " target=", target);
        REQUIRE_EQ(locate_two_sided(n, std::ref(oracle)), target);
        const std::int64_t actual_k = std::min(target, n - 1 - target);
        REQUIRE(oracle.probes() <= 5 * log2_ceil(static_cast<double>(actual_k) + 2) + 10);
    }
}

TEST(4, "seek on an empty span") {
    const std::vector<int> empty;
    FingerCursor<int> cursor(empty);
    CHECK_EQ(cursor.seek(5), 0);
    CHECK_EQ(cursor.position(), 0);
}

TEST(4, "seek returns the first index that is not smaller") {
    const std::vector<int> values = {10, 20, 20, 30};
    FingerCursor<int> cursor(values);
    CHECK_EQ(cursor.position(), 0);
    CHECK_EQ(cursor.seek(20), 1);
    CHECK_EQ(cursor.position(), 1);
    CHECK_EQ(cursor.seek(25), 3);
    CHECK_EQ(cursor.seek(100), 4);
    CHECK_EQ(cursor.position(), 4);
    CHECK_EQ(cursor.seek(5), 0);
    CHECK_EQ(cursor.seek(20), 1);
    CHECK_EQ(cursor.seek(20), 1);  // seeking the same key stays put
}

TEST(4, "random seeks in both directions match std::lower_bound") {
    Rng rng;
    for (int round = 0; round < 500; ++round) {
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 200)), 0, 150);
        std::sort(values.begin(), values.end());
        FingerCursor<int> cursor(values);
        for (int step = 0; step < 200; ++step) {
            const int key = static_cast<int>(rng.uniform(-2, 152));
            const auto expected = static_cast<std::size_t>(
                std::lower_bound(values.begin(), values.end(), key) - values.begin());
            INFO("round ", round, " step ", step, " key=", key);
            REQUIRE_EQ(cursor.seek(key), expected);
            REQUIRE_EQ(cursor.position(), expected);
        }
    }
}

TEST(4, "comparisons scale with the distance moved") {
    using Item = Counted<int>;
    Rng rng;
    const std::size_t n = 1'048'576;
    std::vector<Item> values;
    values.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        values.emplace_back(static_cast<int>(i));
    }
    const std::span<const Item> view(values);
    FingerCursor<Item> cursor(view);
    Item::comparisons() = 0;

    // Short hops around the middle of a large array, in both directions.
    double budget = 0;
    std::size_t position = 0;
    for (int step = 0; step < 5'000; ++step) {
        const auto target = static_cast<std::size_t>(rng.uniform(0, 1'048'575));
        const std::size_t nearby =
            step % 2 == 0 ? target
                          : std::min(n - 1, position + static_cast<std::size_t>(rng.uniform(0, 3)));
        const std::size_t distance = nearby > position ? nearby - position : position - nearby;
        budget += 4.0 * std::log2(static_cast<double>(distance) + 2.0) + 10.0;
        cursor.seek(Item(static_cast<int>(nearby)));
        position = nearby;
    }
    CHECK_LE(static_cast<double>(Item::comparisons()), budget);
}

TEST(4, "ascending queries over a large array are close to linear") {
    using Item = Counted<int>;
    const std::size_t n = 1'000'000;
    std::vector<Item> values;
    values.reserve(n);
    for (std::size_t i = 0; i < n; ++i) {
        values.emplace_back(static_cast<int>(2 * i));
    }
    const std::span<const Item> view(values);
    FingerCursor<Item> cursor(view);
    Item::comparisons() = 0;
    std::size_t last = 0;
    for (std::size_t i = 0; i < 100'000; ++i) {
        last = cursor.seek(Item(static_cast<int>(2 * i + 1)));
    }
    CHECK_LE(Item::comparisons(), 100'000U * 12);
    CHECK_EQ(last, 100'000);
}

// Binary searching from scratch costs about 20 comparisons per short hop; galloping costs about
// 4, but the point of this test is that the seeks are not linear in the length.
PERF(4, "millions of short hops on a large array", 1200) {
    std::vector<int> values(5'000'000);
    for (std::size_t i = 0; i < values.size(); ++i) {
        values[i] = static_cast<int>(i * 2);
    }
    FingerCursor<int> cursor(values);
    Rng rng;
    long long sum = 0;
    std::size_t where = 2'500'000;
    for (int i = 0; i < 10'000'000; ++i) {
        const auto hop = rng.uniform(-3, 3);
        where = static_cast<std::size_t>(
            std::clamp<std::int64_t>(static_cast<std::int64_t>(where) + hop, 0, 4'999'999));
        sum += static_cast<long long>(cursor.seek(static_cast<int>(where * 2)));
    }
    CHECK_GT(sum, 0);
}
