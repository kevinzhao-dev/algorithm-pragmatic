#include <cmath>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

std::vector<Counted<int>> to_counted(const std::vector<int>& values) {
    std::vector<Counted<int>> out;
    out.reserve(values.size());
    for (int value : values) {
        out.emplace_back(value);
    }
    return out;
}

std::vector<int> sorted_copy(std::vector<int> values) {
    std::ranges::sort(values);
    return values;
}

// The inputs that defeat a fixed pivot rule or a two-way partition.
std::vector<int> shaped(const std::string& shape, std::size_t n, Rng& rng) {
    std::vector<int> values(n);
    for (std::size_t i = 0; i < n; ++i) {
        const int position = static_cast<int>(i);
        if (shape == "sorted") {
            values[i] = position;
        } else if (shape == "reversed") {
            values[i] = static_cast<int>(n) - position;
        } else if (shape == "equal") {
            values[i] = 9;
        } else if (shape == "few distinct") {
            values[i] = position % 4;
        } else if (shape == "organ pipe") {
            values[i] = std::min(position, static_cast<int>(n) - position);
        } else {
            values[i] = static_cast<int>(rng.uniform(0, 1'000'000'000));
        }
    }
    return values;
}

const std::vector<std::string> kShapes{"sorted", "reversed", "equal", "few distinct", "organ pipe", "random"};

double mean_sort_comparisons(const std::vector<int>& input, std::mt19937_64& rng, int trials) {
    std::uint64_t total = 0;
    for (int trial = 0; trial < trials; ++trial) {
        auto values = to_counted(input);
        Counted<int>::comparisons() = 0;
        randomized_quicksort<Counted<int>>(values, rng);
        total += Counted<int>::comparisons();
    }
    return static_cast<double>(total) / trials;
}

}  // namespace

TEST(1, "short inputs sort correctly") {
    Rng rng;
    std::vector<int> empty;
    randomized_quicksort<int>(empty, rng.engine());
    CHECK(empty.empty());
    std::vector<int> one{4};
    randomized_quicksort<int>(one, rng.engine());
    CHECK_EQ(one, std::vector<int>{4});
    std::vector<int> two{2, 1};
    randomized_quicksort<int>(two, rng.engine());
    CHECK_EQ(two, (std::vector<int>{1, 2}));
}

TEST(1, "strings sort without losing data") {
    Rng rng;
    std::vector<std::string> words;
    for (int i = 0; i < 2000; ++i) {
        words.push_back(rng.text(static_cast<std::size_t>(rng.uniform(0, 40)), "abc"));
    }
    auto expected = words;
    std::ranges::sort(expected);
    randomized_quicksort<std::string>(words, rng.engine());
    CHECK_EQ(words, expected);
}

TEST(1, "every input shape sorts correctly") {
    Rng rng;
    for (const std::string& shape : kShapes) {
        INFO("shape ", shape);
        auto values = shaped(shape, 20'000, rng);
        const auto expected = sorted_copy(values);
        randomized_quicksort<int>(values, rng.engine());
        CHECK_EQ(values, expected);
    }
}

TEST(1, "random arrays match std::sort") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 100)), 0, rng.coin() ? 3 : 1000);
        const auto expected = sorted_copy(values);
        randomized_quicksort<int>(values, rng.engine());
        CHECK_EQ(values, expected);
    }
}

TEST(1, "expected comparisons stay O(n log n) on every shape") {
    Rng rng;
    const std::size_t n = 1 << 12;
    for (const std::string& shape : kShapes) {
        INFO("shape ", shape);
        const auto input = shaped(shape, n, rng);
        const double mean = mean_sort_comparisons(input, rng.engine(), 10);
        CHECK_LE(mean, 4.0 * static_cast<double>(n) * std::log2(static_cast<double>(n)));
    }
}

PERF(1, "sorting five million integers", 3000) {
    Rng rng;
    auto values = rng.ints(5'000'000, 0, 1'000'000'000);
    randomized_quicksort<int>(values, rng.engine());
    CHECK(std::ranges::is_sorted(values));
}

TEST(2, "short inputs report depth zero or one") {
    Rng rng;
    std::vector<int> one{5};
    CHECK_EQ(paranoid_quicksort<int>(one, rng.engine()), 0);
    std::vector<int> two{2, 1};
    CHECK_EQ(paranoid_quicksort<int>(two, rng.engine()), 1);
    CHECK_EQ(two, (std::vector<int>{1, 2}));
}

TEST(2, "all-equal input finishes in a single level") {
    Rng rng;
    std::vector<int> values(1000, 7);
    CHECK_EQ(paranoid_quicksort<int>(values, rng.engine()), 1);
}

TEST(2, "every shape sorts correctly with a bounded recursion depth") {
    Rng rng;
    const std::size_t n = 20'000;
    const int bound = static_cast<int>(std::ceil(std::log(static_cast<double>(n)) / std::log(4.0 / 3.0))) + 1;
    for (const std::string& shape : kShapes) {
        INFO("shape ", shape);
        for (int trial = 0; trial < 5; ++trial) {
            auto values = shaped(shape, n, rng);
            const auto expected = sorted_copy(values);
            CHECK_LE(paranoid_quicksort<int>(values, rng.engine()), bound);
            CHECK_EQ(values, expected);
        }
    }
}

TEST(2, "expected comparisons remain O(n log n)") {
    Rng rng;
    const std::size_t n = 1 << 12;
    for (const std::string& shape : kShapes) {
        INFO("shape ", shape);
        const auto input = shaped(shape, n, rng);
        std::uint64_t total = 0;
        for (int trial = 0; trial < 10; ++trial) {
            auto values = to_counted(input);
            Counted<int>::comparisons() = 0;
            paranoid_quicksort<Counted<int>>(values, rng.engine());
            total += Counted<int>::comparisons();
        }
        CHECK_LE(static_cast<double>(total) / 10.0,
                 12.0 * static_cast<double>(n) * std::log2(static_cast<double>(n)));
    }
}

TEST(2, "random arrays match std::sort") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 80)), 0, rng.coin() ? 2 : 500);
        const auto expected = sorted_copy(values);
        paranoid_quicksort<int>(values, rng.engine());
        CHECK_EQ(values, expected);
    }
}

TEST(3, "a rank out of range throws") {
    Rng rng;
    std::vector<int> values{1, 2, 3};
    CHECK_THROWS_AS(randomized_select<int>(values, 3, rng.engine()), std::out_of_range);
    std::vector<int> empty;
    CHECK_THROWS_AS(randomized_select<int>(empty, 0, rng.engine()), std::out_of_range);
}

TEST(3, "every rank of a small array is found") {
    Rng rng;
    const std::vector<int> original{50, 20, 90, 10, 40, 30, 80, 60, 70, 0};
    for (std::size_t k = 0; k < original.size(); ++k) {
        auto values = original;
        CHECK_EQ(randomized_select<int>(values, k, rng.engine()), static_cast<int>(k) * 10);
    }
}

TEST(3, "the array is left partitioned around the rank") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(1, 80)), 0, rng.coin() ? 4 : 300);
        const auto expected = sorted_copy(values);
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(values.size()) - 1));
        const int answer = randomized_select<int>(values, k, rng.engine());
        REQUIRE_EQ(answer, expected[k]);
        for (std::size_t i = 0; i < k; ++i) {
            CHECK_LE(values[i], answer);
        }
        for (std::size_t i = k + 1; i < values.size(); ++i) {
            CHECK_GE(values[i], answer);
        }
        CHECK_EQ(sorted_copy(values), expected);
    }
}

TEST(3, "expected comparisons stay linear on every shape") {
    Rng rng;
    const std::size_t n = 1 << 16;
    for (const std::string& shape : kShapes) {
        INFO("shape ", shape);
        const auto input = shaped(shape, n, rng);
        std::uint64_t total = 0;
        for (int trial = 0; trial < 12; ++trial) {
            auto values = to_counted(input);
            Counted<int>::comparisons() = 0;
            randomized_select<Counted<int>>(values, n / 2, rng.engine());
            total += Counted<int>::comparisons();
        }
        CHECK_LE(static_cast<double>(total) / 12.0, 8.0 * static_cast<double>(n));
    }
}

PERF(3, "selecting from five million integers", 1200) {
    Rng rng;
    auto values = rng.ints(5'000'000, 0, 1'000'000'000);
    const std::size_t k = values.size() / 3;
    const int answer = randomized_select<int>(values, k, rng.engine());
    CHECK_LE(*std::ranges::max_element(std::span(values).first(k)), answer);
}

TEST(4, "asking for too many elements throws") {
    Rng rng;
    std::vector<int> values{1, 2};
    CHECK_THROWS_AS(top_k<int>(values, 3, rng.engine()), std::out_of_range);
}

TEST(4, "k zero returns nothing") {
    Rng rng;
    std::vector<int> values{1, 2};
    CHECK(top_k<int>(values, 0, rng.engine()).empty());
}

TEST(4, "the largest elements are returned in descending order") {
    Rng rng;
    std::vector<int> values{5, 1, 9, 3, 7};
    CHECK_EQ(top_k<int>(values, 3, rng.engine()), (std::vector<int>{9, 7, 5}));
}

TEST(4, "duplicates are kept up to k") {
    Rng rng;
    std::vector<int> values{4, 4, 4, 2};
    CHECK_EQ(top_k<int>(values, 2, rng.engine()), (std::vector<int>{4, 4}));
}

TEST(4, "k equal to the size returns everything sorted") {
    Rng rng;
    std::vector<int> values{3, 1, 2};
    CHECK_EQ(top_k<int>(values, 3, rng.engine()), (std::vector<int>{3, 2, 1}));
}

TEST(4, "random inputs match sorting") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        INFO("trial ", trial);
        auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 80)), 0, rng.coin() ? 5 : 1000);
        auto expected = sorted_copy(values);
        std::ranges::reverse(expected);
        const auto k = static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(values.size())));
        expected.resize(k);
        CHECK_EQ(top_k<int>(values, k, rng.engine()), expected);
    }
}

TEST(4, "comparisons are linear plus k log k") {
    Rng rng;
    const std::size_t n = 1 << 16;
    const std::size_t k = 64;
    std::uint64_t total = 0;
    for (int trial = 0; trial < 12; ++trial) {
        auto values = to_counted(rng.ints(n, 0, 1'000'000));
        Counted<int>::comparisons() = 0;
        top_k<Counted<int>>(values, k, rng.engine());
        total += Counted<int>::comparisons();
    }
    CHECK_LE(static_cast<double>(total) / 12.0, 8.0 * static_cast<double>(n) + 40.0 * static_cast<double>(k));
}

PERF(4, "top ten of five million integers", 1200) {
    Rng rng;
    auto values = rng.ints(5'000'000, 0, 1'000'000'000);
    const auto top = top_k<int>(values, 10, rng.engine());
    CHECK(std::ranges::is_sorted(top, std::ranges::greater{}));
}
