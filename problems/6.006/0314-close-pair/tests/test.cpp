#include <algorithm>
#include <cstdint>
#include <limits>
#include <optional>
#include <set>
#include <span>
#include <unordered_map>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;
using Values = std::vector<std::uint64_t>;

namespace {

bool brute_has_close_pair(const Values& b, std::uint64_t r) {
    const std::size_t n = b.size();
    for (std::size_t i = 0; i < n; ++i) {
        for (std::size_t j = i + 1; j < n && (j - i) * 10 < n; ++j) {
            // Compare in 128 bits so the check itself cannot overflow.
            if (static_cast<unsigned __int128>(b[i]) + b[j] == r) {
                return true;
            }
        }
    }
    return false;
}

template <typename Fn>
void check_answer(const Values& b, std::uint64_t r, Fn solve) {
    const auto got = solve(std::span<const std::uint64_t>(b), r);
    const bool expected = brute_has_close_pair(b, r);
    REQUIRE_EQ(got.has_value(), expected);
    if (got.has_value()) {
        const auto [i, j] = *got;
        REQUIRE(i < j);
        REQUIRE(j < b.size());
        REQUIRE_EQ(static_cast<unsigned __int128>(b[i]) + b[j], static_cast<unsigned __int128>(r));
        REQUIRE((j - i) * 10 < b.size());
    }
}

template <typename Fn>
void common_edge_cases(Fn solve) {
    // A pair at distance 1 is allowed only once 10 < n.
    Values ten(10);
    for (std::size_t i = 0; i < 10; ++i) {
        ten[i] = 100 + i * 7;
    }
    check_answer(ten, ten[3] + ten[4], solve);

    Values eleven = ten;
    eleven.push_back(1000);
    check_answer(eleven, eleven[3] + eleven[4], solve);

    // Value r / 2 present but only once: it must not pair with itself.
    Values lonely{50, 7, 200, 3000, 41, 900, 123, 77, 1, 999, 555, 8};
    check_answer(lonely, 100, solve);

    // Values larger than r exist, and subtraction must not wrap around.
    Values mix{5, std::numeric_limits<std::uint64_t>::max(), 3, 8, 21, 34, 55, 89, 144, 233, 377};
    check_answer(mix, 8, solve);

    // Two values whose true sum is 2^64 + 5 wrap around to exactly 5 in 64-bit arithmetic.
    Values wrap(12);
    for (std::size_t i = 0; i < 12; ++i) {
        wrap[i] = 1000 + 17 * i;
    }
    wrap[4] = std::uint64_t{1} << 63;
    wrap[5] = (std::uint64_t{1} << 63) + 5;
    check_answer(wrap, 5, solve);

    // Pair at the widest allowed distance, then one step too far.
    Values wide(30);
    for (std::size_t i = 0; i < 30; ++i) {
        wide[i] = 1000 + i;
    }
    wide[0] = 3;
    wide[2] = 4;
    check_answer(wide, 7, solve);  // distance 2 < 3
    wide[2] = 1002;
    wide[3] = 4;
    check_answer(wide, 7, solve);  // distance 3 < 3 is false

    check_answer({}, 5, solve);
    check_answer({1}, 2, solve);
}

template <typename Fn>
void random_cross_check(Fn solve, bool polynomial_values) {
    Rng rng;
    for (int round = 0; round < 3000; ++round) {
        INFO("round ", round);
        const auto n = static_cast<std::size_t>(rng.uniform(0, 80));
        std::set<std::uint64_t> chosen;
        const std::uint64_t spread = polynomial_values ? static_cast<std::uint64_t>(n) * n + 1
                                                       : std::uint64_t{1} << 40;
        while (chosen.size() < n) {
            chosen.insert(static_cast<std::uint64_t>(rng.uniform(1, static_cast<std::int64_t>(spread) + 200)));
        }
        Values b(chosen.begin(), chosen.end());
        rng.shuffle(b);
        std::uint64_t r;
        if (n >= 2 && rng.coin(0.7)) {
            const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
            const auto j = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n) - 1));
            r = b[i] + b[j];
        } else {
            r = static_cast<std::uint64_t>(rng.uniform(1, static_cast<std::int64_t>(spread)));
        }
        if (polynomial_values && r >= (n * n)) {
            continue;  // stage 2 only promises r < n^2
        }
        check_answer(b, r, solve);
    }
}

}  // namespace

TEST(1, "edge cases around distance, duplicates of r/2 and overflow") {
    common_edge_cases(close_pair_expected);
}

TEST(1, "finds a planted pair in the middle") {
    Values b{10, 20, 30, 40, 50, 60, 70, 80, 90, 1000, 2000, 3000, 4000, 5000, 6000};
    const auto got = close_pair_expected(std::span<const std::uint64_t>(b), 150);
    REQUIRE(got.has_value());
    CHECK_EQ(b[got->first] + b[got->second], std::uint64_t{150});
}

TEST(1, "no close pair even though a far pair exists") {
    Values b;
    for (std::uint64_t i = 0; i < 100; ++i) {
        b.push_back(1000 + i);
    }
    b.front() = 1;
    b.back() = 2;
    CHECK(!close_pair_expected(std::span<const std::uint64_t>(b), 3).has_value());
}

TEST(1, "random inputs match brute force") { random_cross_check(close_pair_expected, false); }

PERF(1, "three million values", 3000) {
    Rng rng;
    const std::size_t n = 3'000'000;
    Values b(n);
    for (std::size_t i = 0; i < n; ++i) {
        b[i] = (static_cast<std::uint64_t>(i) + 1) << 8;  // distinct, none pair up by accident near r
    }
    rng.shuffle(b);
    b[1'000'000] = 777;
    b[1'000'003] = 123;
    const auto got = close_pair_expected(std::span<const std::uint64_t>(b), 900);
    REQUIRE(got.has_value());
    CHECK_EQ(b[got->first] + b[got->second], std::uint64_t{900});
}

TEST(2, "edge cases around distance, duplicates of r/2 and overflow") {
    common_edge_cases(close_pair_worst_case);
}

TEST(2, "values far above n squared are simply ignored") {
    Values b{1, 2, 3, std::uint64_t{1} << 60, std::uint64_t{1} << 61, 40, 50, 60, 70, 80, 90, 100};
    const auto got = close_pair_worst_case(std::span<const std::uint64_t>(b), 3);
    REQUIRE(got.has_value());
    CHECK_EQ(b[got->first] + b[got->second], std::uint64_t{3});
}

TEST(2, "two pairs share a target but only one is close") {
    std::vector<std::uint64_t> b(40);
    for (std::size_t i = 0; i < 40; ++i) {
        b[i] = 1000 + 3 * i;
    }
    b[0] = 100;
    b[39] = 200;  // far pair summing to 300
    b[10] = 140;
    b[11] = 160;  // close pair summing to 300
    check_answer(b, 300, close_pair_worst_case);
}

TEST(2, "random inputs with bounded target match brute force") {
    random_cross_check(close_pair_worst_case, true);
}

// Every value is below r, so the sort really has to order all n of them; the planted pair is
// the only solution because the rest are multiples of 7 and r is not.
PERF(2, "ten million values with r below n squared", 3500) {
    Rng rng;
    const std::size_t n = 10'000'000;
    Values b(n);
    for (std::size_t i = 0; i < n; ++i) {
        b[i] = (static_cast<std::uint64_t>(i) + 1) * 7;
    }
    rng.shuffle(b);
    b[n / 2] = 1'000'000'003;
    b[n / 2 + 1] = 1'000'000'005;
    const std::uint64_t r = 2'000'000'008;
    const auto got = close_pair_worst_case(std::span<const std::uint64_t>(b), r);
    REQUIRE(got.has_value());
    CHECK_EQ(b[got->first] + b[got->second], r);
}

TEST(3, "matches and misses against the window boundary") {
    WindowPairMatcher matcher(3, 10);
    CHECK_EQ(matcher.offer(4), std::optional<std::size_t>());       // 0
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 1
    CHECK_EQ(matcher.offer(6), std::optional<std::size_t>(0));      // 2: partner at distance 2
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 3
    CHECK_EQ(matcher.offer(4), std::optional<std::size_t>(2));      // 4
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 5
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 6
    CHECK_EQ(matcher.offer(6), std::optional<std::size_t>(4));      // 7: distance 3 is still in range
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 8
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 9
    CHECK_EQ(matcher.offer(100), std::optional<std::size_t>());     // 10
    CHECK_EQ(matcher.offer(4), std::optional<std::size_t>());       // 11: 7 is now out of range
}

TEST(3, "duplicates resolve to the latest occurrence") {
    WindowPairMatcher matcher(10, 10);
    matcher.offer(3);                                  // 0
    matcher.offer(3);                                  // 1
    matcher.offer(3);                                  // 2
    CHECK_EQ(matcher.offer(7), std::optional<std::size_t>(2));
}

TEST(3, "zero values and a value equal to target") {
    WindowPairMatcher matcher(5, 9);
    CHECK_EQ(matcher.offer(0), std::optional<std::size_t>());      // 0
    CHECK_EQ(matcher.offer(9), std::optional<std::size_t>(0));     // 1: 9 + 0
}

TEST(3, "window must be positive") {
    CHECK_THROWS_AS(WindowPairMatcher(0, 5), std::invalid_argument);
}

TEST(3, "tracked values stay bounded by the window") {
    WindowPairMatcher matcher(50, 1'000'000);
    for (std::uint64_t i = 0; i < 20'000; ++i) {
        matcher.offer(i);
        REQUIRE(matcher.tracked() <= 50);
    }
}

TEST(3, "random stream matches brute force") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        INFO("round ", round);
        const auto window = static_cast<std::size_t>(rng.uniform(1, 30));
        const auto target = static_cast<std::uint64_t>(rng.uniform(0, 40));
        WindowPairMatcher matcher(window, target);
        std::vector<std::uint64_t> stream;
        for (int step = 0; step < 300; ++step) {
            INFO("step ", step);
            const auto value = static_cast<std::uint64_t>(rng.uniform(0, 40));
            std::optional<std::size_t> expected;
            for (std::size_t j = stream.size(); j-- > 0;) {
                if (stream.size() - j <= window && stream[j] + value == target) {
                    expected = j;
                    break;
                }
            }
            REQUIRE_EQ(matcher.offer(value), expected);
            stream.push_back(value);
        }
    }
}

PERF(3, "four million offers with a large window", 3500) {
    WindowPairMatcher matcher(100'000, 2'000'000);
    Rng rng(123);
    std::uint64_t matches = 0;
    for (int i = 0; i < 4'000'000; ++i) {
        matches += matcher.offer(static_cast<std::uint64_t>(rng.uniform(0, 4'000'000))).has_value() ? 1 : 0;
    }
    CHECK_GT(matches, 0);
}
