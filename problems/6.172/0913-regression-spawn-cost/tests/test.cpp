#include <cmath>
#include <cstdio>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

namespace {

struct Counts {
    std::uint64_t instructions = 0;
    std::uint64_t spawns = 0;
};

void expected_counts(int n, int base, Counts& out) {
    ++out.instructions;
    if (n < 2) {
        return;
    }
    if (n >= base) {
        ++out.spawns;
    }
    expected_counts(n - 1, base, out);
    expected_counts(n - 2, base, out);
}

Counts expected(int n, int base) {
    Counts counts;
    expected_counts(n, base, counts);
    return counts;
}

}  // namespace

TEST(1, "spawns and instructions match an independent count") {
    for (const Input input : std::vector<Input>{{10, 4}, {12, 8}, {14, 3}, {9, 30}, {6, 2}}) {
        INFO("n=", input.first, " base=", input.second);
        const RunCounts got = count_run(input.first, input.second);
        const Counts want = expected(input.first, input.second);
        CHECK_EQ(got.instructions, want.instructions);
        CHECK_EQ(got.spawns, want.spawns);
    }
}

TEST(1, "below the cut-off nothing is spawned") {
    const RunCounts counts = count_run(10, 11);
    CHECK_EQ(counts.spawns, 0);
    CHECK_GT(counts.instructions, 0);
}

TEST(1, "each run reports only its own counts") {
    const RunCounts first = count_run(11, 5);
    const RunCounts second = count_run(11, 5);
    CHECK_EQ(first.spawns, second.spawns);
    CHECK_EQ(first.instructions, second.instructions);
}

TEST(2, "the virtual spawn cost is recovered exactly") {
    std::vector<Sample> samples;
    for (const Input input : std::vector<Input>{{16, 16}, {16, 12}, {16, 8}, {16, 4}, {14, 10}, {14, 6}, {12, 3}}) {
        samples.push_back(sample_virtual(input.first, input.second));
    }
    const CostModel model = fit_spawn_cost(samples);
    CHECK_NEAR(model.per_instruction, workload::kUnitCost, 1e-6);
    CHECK_NEAR(model.per_spawn, workload::kSpawnCost, 1e-6);
}

TEST(2, "a sample's time is the virtual clock's increment") {
    const Sample sample = sample_virtual(15, 9);
    CHECK_NEAR(sample.time,
               sample.instructions * workload::kUnitCost + sample.spawns * workload::kSpawnCost, 1e-9);
    CHECK_GT(sample.spawns, 0.0);
}

TEST(2, "exactly collinear regressors are refused") {
    std::vector<Sample> samples;
    for (double i = 1; i <= 4; ++i) {
        samples.push_back({2.0 * i, 3.0 * i, i});
    }
    CHECK_THROWS_AS(fit_spawn_cost(samples), std::domain_error);
}

TEST(3, "your design has enough inputs within the limits") {
    const auto inputs = design_inputs();
    REQUIRE(inputs.size() >= 6);
    const std::set<Input> distinct(inputs.begin(), inputs.end());
    CHECK_EQ(distinct.size(), inputs.size());
    for (const auto& [n, base] : inputs) {
        INFO("n=", n, " base=", base);
        REQUIRE(n >= 1 && n <= 24);
        REQUIRE(base >= 2);
    }
}

TEST(3, "your design varies spawns independently of the work") {
    std::vector<double> instructions, spawns;
    for (const auto& [n, base] : design_inputs()) {
        const Sample sample = sample_virtual(n, base);
        instructions.push_back(sample.instructions);
        spawns.push_back(sample.spawns);
    }
    INFO("correlation ", practice::ols::correlation(instructions, spawns));
    CHECK_LE(std::fabs(practice::ols::correlation(instructions, spawns)), 0.95);
}

PERF(3, "a spawn costs at least twice a function call", 60000) {
    const double fib_time = time_fib_spawn_ns(18, 2, 3);
    CHECK_GT(fib_time, 0.0);
    const double spawn = measure_spawn_cost(design_inputs(), 5);
    const double call = call_cost_ns();
    std::printf("measured: %.3f ns per spawn, %.3f ns per call, ratio %.1f\n", spawn, call,
                spawn / call);
    CHECK_GT(spawn, 2.0 * call);
}
