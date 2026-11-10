#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <optional>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Widths = std::vector<int>;
constexpr std::int64_t kImpossible = std::numeric_limits<std::int64_t>::max();

// Cost of lines [starts[k], starts[k + 1]), or kImpossible if a line overflows.
std::int64_t layout_cost(const Widths& widths, const std::vector<std::size_t>& starts,
                         int line_width) {
    std::int64_t total = 0;
    for (std::size_t k = 0; k < starts.size(); ++k) {
        const std::size_t end = k + 1 < starts.size() ? starts[k + 1] : widths.size();
        std::int64_t used = -1;
        for (std::size_t w = starts[k]; w < end; ++w) {
            used += widths[w] + 1;
        }
        if (used > line_width) {
            return kImpossible;
        }
        if (k + 1 < starts.size()) {
            const std::int64_t slack = line_width - used;
            total += slack * slack * slack;
        }
    }
    return total;
}

// Tries every set of line breaks.
std::int64_t brute_force(const Widths& widths, int line_width) {
    if (widths.empty()) {
        return 0;
    }
    std::int64_t best = kImpossible;
    const std::size_t n = widths.size();
    for (std::size_t mask = 0; mask < (std::size_t{1} << (n - 1)); ++mask) {
        std::vector<std::size_t> starts{0};
        for (std::size_t i = 1; i < n; ++i) {
            if ((mask >> (i - 1)) & 1) {
                starts.push_back(i);
            }
        }
        best = std::min(best, layout_cost(widths, starts, line_width));
    }
    return best;
}

}  // namespace

TEST(1, "no words cost nothing") {
    CHECK_EQ(min_badness({}, 10), std::optional<std::int64_t>(0));
}

TEST(1, "a word wider than the line makes the layout impossible") {
    const Widths widths{3, 11, 2};
    CHECK_EQ(min_badness(widths, 10), std::optional<std::int64_t>());
}

TEST(1, "a word exactly as wide as the line fits") {
    const Widths widths{10};
    CHECK_EQ(min_badness(widths, 10), std::optional<std::int64_t>(0));
}

TEST(1, "the last line is free even when it is short") {
    const Widths widths{1};
    CHECK_EQ(min_badness(widths, 100), std::optional<std::int64_t>(0));
}

TEST(1, "spaces between words count toward the width") {
    const Widths widths{5, 5};
    // Needs two lines because 5 + 1 + 5 = 11 > 10; the first has slack 5.
    CHECK_EQ(min_badness(widths, 10), std::optional<std::int64_t>(125));
}

TEST(1, "cubic badness can favor a looser first line over a greedy packing") {
    // Greedy packs {4, 3} and is left with a very short middle line.
    const Widths widths{4, 3, 2, 7};
    CHECK_EQ(min_badness(widths, 9), std::optional<std::int64_t>(152));
}

TEST(1, "random inputs match trying every set of breaks") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        const auto widths = rng.ints(static_cast<std::size_t>(rng.uniform(1, 12)), 1, 8);
        const int line_width = static_cast<int>(rng.uniform(1, 20));
        INFO("widths ", widths, " line_width ", line_width);
        const std::int64_t expected = brute_force(widths, line_width);
        const auto actual = min_badness(widths, line_width);
        if (expected == kImpossible) {
            REQUIRE(!actual.has_value());
        } else {
            REQUIRE_EQ(actual, std::optional<std::int64_t>(expected));
        }
    }
}

PERF(1, "twenty thousand words with thousands per line", 1500) {
    const Widths widths(20'000, 1);
    CHECK(min_badness(widths, 9'999).has_value());
}

TEST(2, "no words means no lines") {
    CHECK_EQ(line_starts({}, 10), std::optional<std::vector<std::size_t>>(std::vector<std::size_t>{}));
}

TEST(2, "the unique optimal layout is found") {
    const Widths widths{4, 3, 2, 7};
    CHECK_EQ(line_starts(widths, 9),
             std::optional<std::vector<std::size_t>>(std::vector<std::size_t>{0, 1, 3}));
}

TEST(2, "impossible layouts stay impossible") {
    const Widths widths{20};
    CHECK(!line_starts(widths, 10).has_value());
}

TEST(2, "random layouts are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        const auto widths = rng.ints(static_cast<std::size_t>(rng.uniform(1, 30)), 1, 8);
        const int line_width = static_cast<int>(rng.uniform(8, 25));
        INFO("widths ", widths, " line_width ", line_width);
        const auto starts = line_starts(widths, line_width);
        REQUIRE(starts.has_value());
        REQUIRE(!starts->empty());
        REQUIRE_EQ((*starts)[0], 0);
        for (std::size_t k = 1; k < starts->size(); ++k) {
            REQUIRE((*starts)[k - 1] < (*starts)[k]);
            REQUIRE((*starts)[k] < widths.size());
        }
        REQUIRE_EQ(layout_cost(widths, *starts, line_width), *min_badness(widths, line_width));
    }
}

TEST(3, "whitespace runs are collapsed") {
    CHECK_EQ(wrap_text("  the\tquick\n\n  fox  ", 100),
             std::optional<std::vector<std::string>>(std::vector<std::string>{"the quick fox"}));
}

TEST(3, "text with no words gives no lines") {
    CHECK_EQ(wrap_text("   \n\t  ", 10),
             std::optional<std::vector<std::string>>(std::vector<std::string>{}));
    CHECK_EQ(wrap_text("", 10), std::optional<std::vector<std::string>>(std::vector<std::string>{}));
}

TEST(3, "a word too long for the line cannot be wrapped") {
    CHECK(!wrap_text("hi incomprehensibilities", 8).has_value());
}

TEST(3, "lines come out in order and within the width") {
    const std::string text =
        "dynamic programming is careful brute force with a memo table and a topological order";
    const auto lines = wrap_text(text, 20);
    REQUIRE(lines.has_value());
    std::string joined;
    for (const std::string& line : *lines) {
        CHECK_LE(line.size(), 20);
        if (!joined.empty()) {
            joined += ' ';
        }
        joined += line;
    }
    CHECK_EQ(joined, text);
}
