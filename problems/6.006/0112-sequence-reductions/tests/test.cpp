#include <algorithm>
#include <cstddef>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <vector>

#include "counting_seq.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

CountingSeq<int> make_seq(const std::vector<int>& values) { return CountingSeq<int>(values); }

std::vector<int> iota_vector(std::size_t n) {
    std::vector<int> values(n);
    std::iota(values.begin(), values.end(), 0);
    return values;
}

std::vector<int> expected_move(std::vector<int> items, std::size_t i, std::size_t k, std::size_t j) {
    std::vector<int> block(items.begin() + static_cast<std::ptrdiff_t>(i),
                           items.begin() + static_cast<std::ptrdiff_t>(i + k));
    items.erase(items.begin() + static_cast<std::ptrdiff_t>(i),
                items.begin() + static_cast<std::ptrdiff_t>(i + k));
    const std::size_t target = j >= i + k ? j - k : j;
    items.insert(items.begin() + static_cast<std::ptrdiff_t>(target), block.begin(), block.end());
    return items;
}

}  // namespace

TEST(1, "reverses a middle section") {
    auto seq = make_seq({1, 2, 3, 4, 5});
    reverse_range(seq, 1, 3);
    CHECK_EQ(seq.items(), std::vector<int>{1, 4, 3, 2, 5});
}

TEST(1, "reverses the whole sequence") {
    auto seq = make_seq({1, 2, 3, 4});
    reverse_range(seq, 0, 4);
    CHECK_EQ(seq.items(), std::vector<int>{4, 3, 2, 1});
}

TEST(1, "reverses a section touching the end") {
    auto seq = make_seq({1, 2, 3, 4});
    reverse_range(seq, 2, 2);
    CHECK_EQ(seq.items(), std::vector<int>{1, 2, 4, 3});
}

TEST(1, "reversing zero or one item performs no operations") {
    auto seq = make_seq({1, 2, 3});
    seq.reset_operations();
    reverse_range(seq, 1, 0);
    reverse_range(seq, 1, 1);
    reverse_range(seq, 3, 0);
    CHECK_EQ(seq.operations(), 0);
    CHECK_EQ(seq.items(), std::vector<int>{1, 2, 3});
}

TEST(1, "an out-of-bounds range throws and changes nothing") {
    auto seq = make_seq({1, 2, 3});
    seq.reset_operations();
    CHECK_THROWS_AS(reverse_range(seq, 2, 2), std::invalid_argument);
    CHECK_THROWS_AS(reverse_range(seq, 4, 0), std::invalid_argument);
    CHECK_EQ(seq.operations(), 0);
    CHECK_EQ(seq.items(), std::vector<int>{1, 2, 3});
}

TEST(1, "every small reversal is correct and within budget") {
    for (std::size_t n = 0; n <= 10; ++n) {
        for (std::size_t i = 0; i <= n; ++i) {
            for (std::size_t k = 0; i + k <= n; ++k) {
                auto seq = make_seq(iota_vector(n));
                seq.reset_operations();
                reverse_range(seq, i, k);
                auto expected = iota_vector(n);
                std::reverse(expected.begin() + static_cast<std::ptrdiff_t>(i),
                             expected.begin() + static_cast<std::ptrdiff_t>(i + k));
                INFO("n=", n, " i=", i, " k=", k);
                REQUIRE_EQ(seq.items(), expected);
                REQUIRE(seq.operations() <= 2 * k);
            }
        }
    }
}

TEST(1, "cost depends on the section length, not on the sequence length") {
    auto seq = make_seq(iota_vector(200'000));
    seq.reset_operations();
    reverse_range(seq, 100'000, 500);
    CHECK_LE(seq.operations(), 1'000);
}

TEST(1, "move-only elements are moved, never copied") {
    std::vector<std::unique_ptr<int>> items;
    for (int i = 0; i < 6; ++i) {
        items.push_back(std::make_unique<int>(i));
    }
    CountingSeq<std::unique_ptr<int>> seq(std::move(items));
    reverse_range(seq, 1, 4);
    std::vector<int> values;
    for (const auto& item : seq.items()) {
        values.push_back(*item);
    }
    CHECK_EQ(values, std::vector<int>{0, 4, 3, 2, 1, 5});
}

TEST(2, "moving a section to the right") {
    auto seq = make_seq({0, 1, 2, 3, 4, 5});
    move_range(seq, 1, 2, 5);
    CHECK_EQ(seq.items(), std::vector<int>{0, 3, 4, 1, 2, 5});
}

TEST(2, "moving a section to the left") {
    auto seq = make_seq({0, 1, 2, 3, 4, 5});
    move_range(seq, 3, 2, 1);
    CHECK_EQ(seq.items(), std::vector<int>{0, 3, 4, 1, 2, 5});
}

TEST(2, "moving to the very end") {
    auto seq = make_seq({0, 1, 2, 3});
    move_range(seq, 0, 2, 4);
    CHECK_EQ(seq.items(), std::vector<int>{2, 3, 0, 1});
}

TEST(2, "moving to the very front") {
    auto seq = make_seq({0, 1, 2, 3});
    move_range(seq, 2, 2, 0);
    CHECK_EQ(seq.items(), std::vector<int>{2, 3, 0, 1});
}

TEST(2, "moving just in front of the following item is a no-op") {
    auto seq = make_seq({0, 1, 2, 3, 4});
    move_range(seq, 1, 2, 3);
    CHECK_EQ(seq.items(), std::vector<int>{0, 1, 2, 3, 4});
}

TEST(2, "invalid arguments throw and change nothing") {
    auto seq = make_seq({0, 1, 2, 3});
    seq.reset_operations();
    CHECK_THROWS_AS(move_range(seq, 1, 2, 2), std::invalid_argument);  // target inside the block
    CHECK_THROWS_AS(move_range(seq, 1, 2, 1), std::invalid_argument);
    CHECK_THROWS_AS(move_range(seq, 2, 3, 0), std::invalid_argument);  // block past the end
    CHECK_THROWS_AS(move_range(seq, 0, 1, 5), std::invalid_argument);  // target past the end
    CHECK_EQ(seq.operations(), 0);
    CHECK_EQ(seq.items(), std::vector<int>{0, 1, 2, 3});
}

TEST(2, "moving zero items does nothing") {
    auto seq = make_seq({0, 1, 2});
    seq.reset_operations();
    move_range(seq, 1, 0, 3);
    CHECK_EQ(seq.operations(), 0);
    CHECK_EQ(seq.items(), std::vector<int>{0, 1, 2});
}

TEST(2, "every small move is correct and within budget") {
    for (std::size_t n = 0; n <= 9; ++n) {
        for (std::size_t i = 0; i <= n; ++i) {
            for (std::size_t k = 0; i + k <= n; ++k) {
                for (std::size_t j = 0; j <= n; ++j) {
                    if (i <= j && j < i + k) {
                        continue;
                    }
                    auto seq = make_seq(iota_vector(n));
                    seq.reset_operations();
                    move_range(seq, i, k, j);
                    INFO("n=", n, " i=", i, " k=", k, " j=", j);
                    REQUIRE_EQ(seq.items(), expected_move(iota_vector(n), i, k, j));
                    REQUIRE(seq.operations() <= 2 * k);
                }
            }
        }
    }
}

TEST(2, "a long move costs proportional to the block, not the distance") {
    auto seq = make_seq(iota_vector(100'000));
    seq.reset_operations();
    move_range(seq, 10, 300, 90'000);
    CHECK_LE(seq.operations(), 600);
    CHECK_EQ(seq.items()[10], 310);
    CHECK_EQ(seq.items()[90'000 - 300], 10);
}

TEST(2, "random moves match a vector model") {
    Rng rng;
    for (int round = 0; round < 1'000; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 40));
        const auto i = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n)));
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n - i)));
        std::size_t j;
        do {
            j = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(n)));
        } while (i <= j && j < i + k);
        auto seq = make_seq(iota_vector(n));
        move_range(seq, i, k, j);
        INFO("n=", n, " i=", i, " k=", k, " j=", j);
        REQUIRE_EQ(seq.items(), expected_move(iota_vector(n), i, k, j));
    }
}

TEST(2, "move-only elements are moved") {
    std::vector<std::unique_ptr<int>> items;
    for (int i = 0; i < 5; ++i) {
        items.push_back(std::make_unique<int>(i));
    }
    CountingSeq<std::unique_ptr<int>> seq(std::move(items));
    move_range(seq, 0, 2, 5);
    std::vector<int> values;
    for (const auto& item : seq.items()) {
        values.push_back(*item);
    }
    CHECK_EQ(values, std::vector<int>{2, 3, 4, 0, 1});
}

TEST(3, "rotating a section") {
    auto seq = make_seq({0, 1, 2, 3, 4, 5});
    rotate_left(seq, 1, 4, 1);
    CHECK_EQ(seq.items(), std::vector<int>{0, 2, 3, 4, 1, 5});
}

TEST(3, "rotation by zero or by k changes nothing and costs nothing") {
    auto seq = make_seq({0, 1, 2, 3});
    seq.reset_operations();
    rotate_left(seq, 0, 4, 0);
    rotate_left(seq, 0, 4, 4);
    CHECK_EQ(seq.operations(), 0);
    CHECK_EQ(seq.items(), std::vector<int>{0, 1, 2, 3});
}

TEST(3, "invalid rotations throw") {
    auto seq = make_seq({0, 1, 2});
    CHECK_THROWS_AS(rotate_left(seq, 0, 3, 4), std::invalid_argument);
    CHECK_THROWS_AS(rotate_left(seq, 2, 3, 1), std::invalid_argument);
}

TEST(3, "every small rotation is correct and picks the cheaper direction") {
    for (std::size_t n = 0; n <= 10; ++n) {
        for (std::size_t i = 0; i <= n; ++i) {
            for (std::size_t k = 0; i + k <= n; ++k) {
                for (std::size_t m = 0; m <= k; ++m) {
                    auto seq = make_seq(iota_vector(n));
                    seq.reset_operations();
                    rotate_left(seq, i, k, m);
                    auto expected = iota_vector(n);
                    std::rotate(expected.begin() + static_cast<std::ptrdiff_t>(i),
                                expected.begin() + static_cast<std::ptrdiff_t>(i + m),
                                expected.begin() + static_cast<std::ptrdiff_t>(i + k));
                    INFO("n=", n, " i=", i, " k=", k, " m=", m);
                    REQUIRE_EQ(seq.items(), expected);
                    REQUIRE(seq.operations() <= 2 * std::min(m, k - m));
                }
            }
        }
    }
}

TEST(3, "almost-full rotations are cheap in either direction") {
    auto seq = make_seq(iota_vector(50'000));
    seq.reset_operations();
    rotate_left(seq, 0, 50'000, 49'990);
    CHECK_LE(seq.operations(), 20);
    CHECK_EQ(seq.items()[0], 49'990);
    seq.reset_operations();
    rotate_left(seq, 0, 50'000, 10);
    CHECK_LE(seq.operations(), 20);
    CHECK_EQ(seq.items()[0], 0);
}
