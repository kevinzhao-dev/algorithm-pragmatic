#include <algorithm>
#include <cstdint>
#include <span>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

// Tries every subset of samples and keeps the longest strictly decreasing gap-free one.
std::size_t exhaustive(std::span<const int> samples, std::size_t per_bucket) {
    std::size_t best = 0;
    for (std::size_t mask = 0; mask < (std::size_t{1} << samples.size()); ++mask) {
        std::size_t length = 0;
        std::size_t last = 0;
        bool valid = true;
        for (std::size_t i = 0; i < samples.size() && valid; ++i) {
            if (!((mask >> i) & 1)) {
                continue;
            }
            if (length > 0) {
                valid = samples[last] > samples[i] && i / per_bucket - last / per_bucket <= 1;
            }
            last = i;
            ++length;
        }
        if (valid) {
            best = std::max(best, length);
        }
    }
    return best;
}

void check_trend(std::span<const int> samples, std::size_t per_bucket,
                 const std::vector<std::size_t>& indices) {
    for (std::size_t k = 0; k < indices.size(); ++k) {
        REQUIRE(indices[k] < samples.size());
        if (k > 0) {
            REQUIRE(indices[k - 1] < indices[k]);
            REQUIRE(samples[indices[k - 1]] > samples[indices[k]]);
            REQUIRE(indices[k] / per_bucket - indices[k - 1] / per_bucket <= 1);
        }
    }
}

std::vector<int> random_series(Rng& rng, std::size_t buckets, std::size_t per_bucket) {
    return rng.ints(buckets * per_bucket, 1, 15);
}

}  // namespace

TEST(1, "no samples means no trend") {
    CHECK_EQ(gapless_decline_length({}, 3), 0);
}

TEST(1, "a zero bucket size is rejected") {
    const std::vector<int> samples{1, 2};
    CHECK_THROWS_AS(gapless_decline_length(samples, 0), std::invalid_argument);
}

TEST(1, "a partial bucket is rejected") {
    const std::vector<int> samples{1, 2, 3};
    CHECK_THROWS_AS(gapless_decline_length(samples, 2), std::invalid_argument);
}

TEST(1, "a single sample is a trend of length one") {
    const std::vector<int> samples{5};
    CHECK_EQ(gapless_decline_length(samples, 1), 1);
}

TEST(1, "equal samples do not count as decreasing") {
    const std::vector<int> samples{4, 4, 4, 4};
    CHECK_EQ(gapless_decline_length(samples, 2), 1);
}

TEST(1, "a falling run across buckets") {
    const std::vector<int> samples{9, 8, 7, 6, 5, 4};
    CHECK_EQ(gapless_decline_length(samples, 2), 6);
}

TEST(1, "a dip within a bucket is usable") {
    const std::vector<int> samples{5, 1, 4, 3};
    // Bucket 0 = {5, 1}, bucket 1 = {4, 3}: 5, 4, 3 is valid; the 1 does not help.
    CHECK_EQ(gapless_decline_length(samples, 2), 3);
}

TEST(1, "a trend may not skip a bucket") {
    // Bucket 0 = {9}, bucket 1 = {100}, bucket 2 = {1}: 9 then 1 would skip bucket 1.
    const std::vector<int> samples{9, 100, 1};
    CHECK_EQ(gapless_decline_length(samples, 1), 2);  // 100, 1
}

TEST(1, "skipping is rejected even when the numbers would fit") {
    // {5, 9}, {2, 8}, {1, 7}: any trend through bucket 2 must also use bucket 1.
    const std::vector<int> samples{5, 9, 2, 8, 1, 7};
    CHECK_EQ(gapless_decline_length(samples, 2), 3);  // e.g. 5, 2, 1 or 9, 8, 7
}

TEST(1, "random samples match exhaustive search") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const auto buckets = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto per_bucket = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto samples = random_series(rng, buckets, per_bucket);
        INFO("samples ", samples, " per_bucket ", per_bucket);
        REQUIRE_EQ(gapless_decline_length(samples, per_bucket), exhaustive(samples, per_bucket));
    }
}

TEST(2, "no samples means no indices") {
    CHECK(gapless_decline({}, 2).empty());
}

TEST(2, "indices are ascending and strictly decreasing in value") {
    const std::vector<int> samples{5, 9, 2, 8, 1, 7};
    const auto indices = gapless_decline(samples, 2);
    check_trend(samples, 2, indices);
    CHECK_EQ(indices.size(), 3);
}

TEST(2, "random trends are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const auto buckets = static_cast<std::size_t>(rng.uniform(1, 4));
        const auto per_bucket = static_cast<std::size_t>(rng.uniform(1, 3));
        const auto samples = random_series(rng, buckets, per_bucket);
        INFO("samples ", samples, " per_bucket ", per_bucket);
        const auto indices = gapless_decline(samples, per_bucket);
        check_trend(samples, per_bucket, indices);
        REQUIRE_EQ(indices.size(), exhaustive(samples, per_bucket));
    }
}

TEST(3, "no series at all is rejected") {
    const std::vector<std::vector<int>> none;
    CHECK_THROWS_AS(worst_series(none, 2), std::invalid_argument);
}

TEST(3, "the longest trend wins across series") {
    const std::vector<std::vector<int>> series{{1, 2, 3, 4}, {4, 3, 2, 1}, {2, 1, 2, 1}};
    const auto result = worst_series(series, 2);
    CHECK_EQ(result.series, 1);
    CHECK_EQ(result.indices, std::vector<std::size_t>{0, 1, 2, 3});
}

TEST(3, "a malformed series is rejected") {
    const std::vector<std::vector<int>> series{{1, 2}, {1, 2, 3}};
    CHECK_THROWS_AS(worst_series(series, 2), std::invalid_argument);
}

TEST(3, "empty series have no trend") {
    const std::vector<std::vector<int>> series{{}, {}};
    const auto result = worst_series(series, 3);
    CHECK_LT(result.series, 2);
    CHECK(result.indices.empty());
}

TEST(3, "random collections pick a series with the longest trend") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const auto count = static_cast<std::size_t>(rng.uniform(1, 5));
        const auto per_bucket = static_cast<std::size_t>(rng.uniform(1, 3));
        std::vector<std::vector<int>> series(count);
        std::size_t best = 0;
        for (auto& samples : series) {
            samples = random_series(rng, static_cast<std::size_t>(rng.uniform(1, 3)), per_bucket);
            best = std::max(best, exhaustive(samples, per_bucket));
        }
        INFO("trial ", trial);
        const auto result = worst_series(series, per_bucket);
        REQUIRE(result.series < count);
        check_trend(series[result.series], per_bucket, result.indices);
        REQUIRE_EQ(result.indices.size(), best);
    }
}

TEST(4, "wide buckets match the quadratic recurrence") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const auto buckets = static_cast<std::size_t>(rng.uniform(1, 12));
        const auto per_bucket = static_cast<std::size_t>(rng.uniform(1, 30));
        const auto samples = rng.ints(buckets * per_bucket, 1, 100);
        const std::size_t total = samples.size();
        std::vector<std::size_t> x(total, 1);
        std::size_t longest = 0;
        for (std::size_t j = total; j-- > 0;) {
            const std::size_t last_allowed =
                std::min(total, (j / per_bucket + 2) * per_bucket);
            for (std::size_t d = j + 1; d < last_allowed; ++d) {
                if (samples[j] > samples[d]) {
                    x[j] = std::max(x[j], x[d] + 1);
                }
            }
            longest = std::max(longest, x[j]);
        }
        INFO("trial ", trial, " per_bucket ", per_bucket);
        const auto indices = gapless_decline(samples, per_bucket);
        check_trend(samples, per_bucket, indices);
        REQUIRE_EQ(indices.size(), longest);
    }
}

PERF(4, "buckets of a hundred thousand samples", 4000) {
    Rng rng;
    const std::size_t per_bucket = 100'000;
    const auto samples = rng.ints(20 * per_bucket, 1, 1'000'000'000);
    const auto indices = gapless_decline(samples, per_bucket);
    check_trend(samples, per_bucket, indices);
    CHECK_GT(indices.size(), 100);
}

PERF(4, "worst_series scales linearly in the number of series", 2500) {
    Rng rng;
    const std::size_t per_bucket = 50'000;
    std::vector<std::vector<int>> series(8);
    for (auto& samples : series) {
        samples = rng.ints(5 * per_bucket, 1, 1'000'000'000);
    }
    CHECK_LT(worst_series(series, per_bucket).series, 8);
}
