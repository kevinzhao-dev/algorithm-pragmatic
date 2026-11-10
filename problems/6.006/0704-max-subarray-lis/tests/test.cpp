#include <algorithm>
#include <bit>
#include <cstdint>
#include <map>
#include <numeric>
#include <queue>
#include <string>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Counted;
using practice::Rng;

namespace {

std::int64_t brute_max_subarray(std::span<const std::int64_t> values) {
    std::int64_t best = values[0];
    for (std::size_t first = 0; first < values.size(); ++first) {
        std::int64_t sum = 0;
        for (std::size_t last = first; last < values.size(); ++last) {
            sum += values[last];
            best = std::max(best, sum);
        }
    }
    return best;
}

std::size_t brute_lis(const std::vector<int>& values) {
    std::size_t best = 0;
    for (std::size_t mask = 0; mask < (std::size_t{1} << values.size()); ++mask) {
        std::size_t length = 0;
        bool increasing = true;
        int last = 0;
        for (std::size_t i = 0; i < values.size() && increasing; ++i) {
            if ((mask >> i) & 1) {
                increasing = length == 0 || last < values[i];
                last = values[i];
                ++length;
            }
        }
        if (increasing) {
            best = std::max(best, length);
        }
    }
    return best;
}

template <typename R>
void check_indices(const R& values, const std::vector<std::size_t>& indices) {
    for (std::size_t k = 0; k < indices.size(); ++k) {
        REQUIRE(indices[k] < std::ranges::size(values));
        if (k > 0) {
            REQUIRE(indices[k - 1] < indices[k]);
            REQUIRE(values[indices[k - 1]] < values[indices[k]]);
        }
    }
}

std::vector<Counted<int>> counted(const std::vector<int>& values) {
    std::vector<Counted<int>> out;
    out.reserve(values.size());
    for (const int value : values) {
        out.emplace_back(value);
    }
    return out;
}

std::uint64_t comparison_budget(std::size_t n) {
    return n * (static_cast<std::uint64_t>(std::bit_width(n)) + 2);
}

// Minimum number of pick-up-and-reinsert moves, found by breadth-first search over orderings.
int min_moves(const std::vector<int>& before, const std::vector<int>& after) {
    std::map<std::vector<int>, int> distance{{before, 0}};
    std::queue<std::vector<int>> queue;
    queue.push(before);
    while (!queue.empty()) {
        const std::vector<int> order = queue.front();
        queue.pop();
        const int steps = distance[order];
        if (order == after) {
            return steps;
        }
        for (std::size_t from = 0; from < order.size(); ++from) {
            for (std::size_t to = 0; to < order.size(); ++to) {
                std::vector<int> next = order;
                const int value = next[from];
                next.erase(next.begin() + static_cast<std::ptrdiff_t>(from));
                next.insert(next.begin() + static_cast<std::ptrdiff_t>(to), value);
                if (distance.emplace(next, steps + 1).second) {
                    queue.push(std::move(next));
                }
            }
        }
    }
    return -1;
}

}  // namespace

TEST(1, "an empty array is rejected") {
    CHECK_THROWS_AS(max_subarray({}), std::invalid_argument);
}

TEST(1, "a single element is its own best subarray") {
    const std::vector<std::int64_t> values{-7};
    const Subarray best = max_subarray(values);
    CHECK_EQ(best.sum, -7);
    CHECK_EQ(best.first, 0);
    CHECK_EQ(best.last, 0);
}

TEST(1, "all negative values pick the largest element") {
    const std::vector<std::int64_t> values{-9, -2, -7, -5};
    const Subarray best = max_subarray(values);
    CHECK_EQ(best.sum, -2);
    CHECK_EQ(best.first, 1);
    CHECK_EQ(best.last, 1);
}

TEST(1, "all positive values take everything") {
    const std::vector<std::int64_t> values{3, 1, 2};
    const Subarray best = max_subarray(values);
    CHECK_EQ(best.sum, 6);
    CHECK_EQ(best.first, 0);
    CHECK_EQ(best.last, 2);
}

TEST(1, "a dip in the middle is worth crossing when both sides pay for it") {
    const std::vector<std::int64_t> values{5, -2, 6};
    CHECK_EQ(max_subarray(values).sum, 9);
}

TEST(1, "large values do not overflow") {
    const std::vector<std::int64_t> values(100, std::int64_t{1'000'000'000'000});
    CHECK_EQ(max_subarray(values).sum, std::int64_t{100'000'000'000'000});
}

TEST(1, "random arrays match brute force and the reported range adds up") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        const auto raw = rng.ints(static_cast<std::size_t>(rng.uniform(1, 30)), -9, 9);
        const std::vector<std::int64_t> values(raw.begin(), raw.end());
        INFO("values ", values);
        const Subarray best = max_subarray(values);
        REQUIRE(best.first <= best.last);
        REQUIRE(best.last < values.size());
        std::int64_t sum = 0;
        for (std::size_t i = best.first; i <= best.last; ++i) {
            sum += values[i];
        }
        REQUIRE_EQ(best.sum, sum);
        REQUIRE_EQ(best.sum, brute_max_subarray(values));
    }
}

PERF(1, "five million values in linear time", 500) {
    Rng rng;
    const auto raw = rng.ints(5'000'000, -1000, 1000);
    const std::vector<std::int64_t> values(raw.begin(), raw.end());
    CHECK_GT(max_subarray(values).sum, 0);
}

TEST(2, "empty and single inputs") {
    CHECK_EQ(lis_length(std::vector<int>{}), 0);
    CHECK_EQ(lis_length(std::vector<int>{42}), 1);
}

TEST(2, "sorted input is one long run") {
    CHECK_EQ(lis_length(std::vector<int>{1, 2, 3, 4, 5}), 5);
}

TEST(2, "descending input has length one") {
    CHECK_EQ(lis_length(std::vector<int>{5, 4, 3, 2, 1}), 1);
}

TEST(2, "equal values do not count as increasing") {
    CHECK_EQ(lis_length(std::vector<int>{7, 7, 7, 7}), 1);
    CHECK_EQ(lis_length(std::vector<int>{1, 2, 2, 3}), 3);
}

TEST(2, "characters and strings work too") {
    CHECK_EQ(lis_length(std::string{"cabde"}), 4);
    CHECK_EQ(lis_length(std::vector<std::string>{"b", "a", "c"}), 2);
}

TEST(2, "random arrays match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        const auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 12)), 0, 8);
        INFO("values ", values);
        REQUIRE_EQ(lis_length(values), brute_lis(values));
    }
}

TEST(3, "indices of a run that skips the middle") {
    const std::vector<int> values{1, 9, 2, 8, 3};
    const auto indices = lis_indices(values);
    check_indices(values, indices);
    CHECK_EQ(indices.size(), 3);
}

TEST(3, "empty input gives no indices") {
    CHECK(lis_indices(std::vector<int>{}).empty());
}

TEST(3, "duplicates are never both used") {
    const std::vector<int> values{4, 4, 4};
    const auto indices = lis_indices(values);
    check_indices(values, indices);
    CHECK_EQ(indices.size(), 1);
}

TEST(3, "random indices are valid and as long as the LIS") {
    Rng rng;
    for (int trial = 0; trial < 2000; ++trial) {
        const auto values = rng.ints(static_cast<std::size_t>(rng.uniform(0, 40)), -5, 5);
        INFO("values ", values);
        const auto indices = lis_indices(values);
        check_indices(values, indices);
        REQUIRE_EQ(indices.size(), lis_length(values));
    }
}

TEST(4, "lis_length uses about n log n comparisons on sorted input") {
    std::vector<int> ascending(20'000);
    std::iota(ascending.begin(), ascending.end(), 0);
    const auto values = counted(ascending);
    Counted<int>::comparisons() = 0;
    CHECK_EQ(lis_length(values), values.size());
    CHECK_LE(Counted<int>::comparisons(), comparison_budget(values.size()));
}

TEST(4, "lis_indices uses about n log n comparisons on sorted input") {
    std::vector<int> ascending(20'000);
    std::iota(ascending.begin(), ascending.end(), 0);
    const auto values = counted(ascending);
    Counted<int>::comparisons() = 0;
    const auto indices = lis_indices(values);
    CHECK_LE(Counted<int>::comparisons(), comparison_budget(values.size()));
    CHECK_EQ(indices.size(), values.size());
}

TEST(4, "random inputs keep the comparison budget and stay optimal") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const auto raw = rng.ints(static_cast<std::size_t>(rng.uniform(0, 300)), 0, 60);
        const auto values = counted(raw);
        Counted<int>::comparisons() = 0;
        const auto indices = lis_indices(values);
        CHECK_LE(Counted<int>::comparisons(), comparison_budget(values.size()));
        INFO("trial ", trial);
        check_indices(raw, indices);
        // Cross-check against the textbook quadratic recurrence.
        std::vector<std::size_t> best(raw.size(), 1);
        std::size_t longest = raw.empty() ? 0 : 1;
        for (std::size_t i = raw.size(); i-- > 0;) {
            for (std::size_t j = i + 1; j < raw.size(); ++j) {
                if (raw[i] < raw[j]) {
                    best[i] = std::max(best[i], best[j] + 1);
                }
            }
            longest = std::max(longest, best[i]);
        }
        REQUIRE_EQ(indices.size(), longest);
    }
}

PERF(4, "three million values in n log n", 1800) {
    Rng rng;
    const auto values = rng.ints(3'000'000, 0, 1'000'000'000);
    const auto indices = lis_indices(values);
    check_indices(values, indices);
    CHECK_GT(indices.size(), 1000);
}

TEST(5, "an unchanged order keeps everything") {
    const std::vector<int> keys{3, 1, 2};
    CHECK_EQ(keep_in_place(keys, keys), std::vector<std::size_t>{0, 1, 2});
}

TEST(5, "moving one element keeps the rest") {
    const std::vector<int> before{1, 2, 3, 4};
    const std::vector<int> after{4, 1, 2, 3};
    CHECK_EQ(keep_in_place(before, after), std::vector<std::size_t>{1, 2, 3});
}

TEST(5, "an empty list keeps nothing") {
    CHECK(keep_in_place(std::vector<int>{}, std::vector<int>{}).empty());
}

TEST(5, "keys can be strings") {
    const std::vector<std::string> before{"a", "b", "c"};
    const std::vector<std::string> after{"c", "a", "b"};
    CHECK_EQ(keep_in_place(before, after), std::vector<std::size_t>{1, 2});
}

TEST(5, "the kept items match a breadth-first search over all reorderings") {
    Rng rng;
    for (int trial = 0; trial < 150; ++trial) {
        const auto size = static_cast<std::size_t>(rng.uniform(1, 6));
        const std::vector<int> before = rng.permutation(size);
        const std::vector<int> after = rng.permutation(size);
        INFO("before ", before, " after ", after);
        const auto kept = keep_in_place(before, after);
        // Kept items must be in the same relative order in both lists.
        for (std::size_t k = 0; k + 1 < kept.size(); ++k) {
            REQUIRE(kept[k] < kept[k + 1]);
            const auto position_of = [&](int key) {
                return std::ranges::find(before, key) - before.begin();
            };
            REQUIRE(position_of(after[kept[k]]) < position_of(after[kept[k + 1]]));
        }
        REQUIRE_EQ(static_cast<int>(size - kept.size()), min_moves(before, after));
    }
}
