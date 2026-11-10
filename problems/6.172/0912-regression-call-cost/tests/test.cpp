#include <cmath>
#include <cstdio>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

struct Counts {
    std::uint64_t instructions = 0;
    std::uint64_t calls = 0;
};

void expected_counts(int n, int base, Counts& out) {
    ++out.calls;
    if (n < base) {
        out.instructions += static_cast<std::uint64_t>(n);
        return;
    }
    expected_counts(n - 1, base, out);
    expected_counts(n - 2, base, out);
}

Counts expected(int n, int base) {
    Counts counts;
    expected_counts(n, base, counts);
    return counts;
}

std::vector<Input> varied_design() { return {{20, 2}, {20, 6}, {20, 11}, {16, 3}, {16, 9}, {18, 5}, {19, 12}, {14, 8}}; }

}  // namespace

TEST(1, "count_run matches an independent count") {
    for (const Input input : std::vector<Input>{{10, 2}, {15, 5}, {7, 3}, {1, 2}, {0, 2}, {20, 12}}) {
        INFO("n=", input.first, " base=", input.second);
        const RunCounts got = count_run(input.first, input.second);
        const Counts want = expected(input.first, input.second);
        CHECK_EQ(got.calls, want.calls);
        CHECK_EQ(got.instructions, want.instructions);
    }
}

TEST(1, "two consecutive runs each report only their own counts") {
    const RunCounts first = count_run(12, 4);
    const RunCounts second = count_run(12, 4);
    CHECK_EQ(first.calls, second.calls);
    CHECK_EQ(first.instructions, second.instructions);
    CHECK_GT(first.calls, 0);
}

TEST(1, "a large base means one call and only a loop") {
    const RunCounts counts = count_run(9, 50);
    CHECK_EQ(counts.calls, 1);
    CHECK_EQ(counts.instructions, 9);
}

TEST(2, "the virtual cost model is recovered exactly") {
    std::vector<Sample> samples;
    for (const Input& input : varied_design()) {
        samples.push_back(sample_virtual(input.first, input.second));
    }
    const CostModel model = fit_cost_model(samples);
    CHECK_NEAR(model.per_instruction, workload::kUnitCost, 1e-6);
    CHECK_NEAR(model.per_call, workload::kCallCost, 1e-6);
}

TEST(2, "time in a sample is the virtual clock's increment") {
    const Sample sample = sample_virtual(13, 6);
    const double predicted =
        sample.instructions * workload::kUnitCost + sample.calls * workload::kCallCost;
    CHECK_NEAR(sample.time, predicted, 1e-9);
    CHECK_GT(sample.calls, 0.0);
}

TEST(2, "exactly collinear counts are refused") {
    std::vector<Sample> samples;
    for (double i = 1; i <= 5; ++i) {
        samples.push_back({2.0 * i, 3.0 * i, 5.0 * i});
    }
    CHECK_THROWS_AS(fit_cost_model(samples), std::domain_error);
}

TEST(2, "counts_correlation reports how related I and F are") {
    std::vector<Sample> perfectly_related;
    for (double i = 1; i <= 5; ++i) {
        perfectly_related.push_back({2.0 * i, 3.0 * i, 1.0});
    }
    CHECK_NEAR(counts_correlation(perfectly_related), 1.0, 1e-9);
    std::vector<Sample> independent{{1, 1, 0}, {1, 3, 0}, {3, 1, 0}, {3, 3, 0}};
    CHECK_NEAR(counts_correlation(independent), 0.0, 1e-9);
}

TEST(2, "a fixed base leaves the counts nearly collinear") {
    std::vector<Sample> samples;
    for (int n = 8; n <= 20; n += 2) {
        samples.push_back(sample_virtual(n, 3));
    }
    CHECK_GT(counts_correlation(samples), 0.999);
}

TEST(3, "your design meets the size and range limits") {
    const auto inputs = design_inputs();
    REQUIRE(inputs.size() >= 6);
    const std::set<Input> distinct(inputs.begin(), inputs.end());
    CHECK_EQ(distinct.size(), inputs.size());
    for (const auto& [n, base] : inputs) {
        INFO("n=", n, " base=", base);
        REQUIRE(n >= 1 && n <= 30);
        REQUIRE(base >= 2);
    }
}

TEST(3, "your design decorrelates instructions from calls") {
    std::vector<Sample> samples;
    for (const auto& [n, base] : design_inputs()) {
        samples.push_back(sample_virtual(n, base));
    }
    INFO("correlation ", counts_correlation(samples));
    CHECK_LE(std::fabs(counts_correlation(samples)), 0.9);
}

TEST(3, "under noise your design estimates costs far better than a fixed base") {
    // Same noise for both designs, driven by a fixed seed.
    const auto estimate_error = [](const std::vector<Input>& inputs) {
        Rng rng(987654321);
        double total_error = 0;
        for (int trial = 0; trial < 40; ++trial) {
            std::vector<Sample> samples;
            for (const auto& [n, base] : inputs) {
                Sample sample = sample_virtual(n, base);
                sample.time *= 1.0 + 0.02 * (rng.real() - 0.5);  // ±1% multiplicative noise
                samples.push_back(sample);
            }
            const CostModel model = fit_cost_model(samples);
            total_error += std::fabs(model.per_instruction - workload::kUnitCost) +
                           std::fabs(model.per_call - workload::kCallCost);
        }
        return total_error / 40.0;
    };

    std::vector<Input> poor;
    for (int n = 8; n <= 20; ++n) {
        poor.emplace_back(n, 3);
    }
    const double poor_error = estimate_error(poor);
    const double your_error = estimate_error(design_inputs());
    INFO("fixed-base error ", poor_error, ", your error ", your_error);
    CHECK_LT(your_error, poor_error * 0.5);
}

PERF(4, "the measured costs on this machine are plausible", 40000) {
    const auto inputs = design_inputs();
    const double fast = time_fib_ns(inputs.front().first, inputs.front().second, 5);
    CHECK_GT(fast, 0.0);
    const CostModel model = measure_real_cost(inputs, 5);
    std::printf("measured: %.3f ns per loop iteration, %.3f ns per call\n", model.per_instruction,
                model.per_call);
    CHECK_GT(model.per_instruction, 0.0);
    CHECK_GT(model.per_call, 0.0);
}
