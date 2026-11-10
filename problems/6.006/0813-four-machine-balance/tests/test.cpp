#include <algorithm>
#include <cstdint>
#include <limits>
#include <numeric>
#include <vector>

#include "07_heap_meter.hpp"
#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

using Jobs = std::vector<int>;

// Recomputes the loads implied by a machine assignment and returns the makespan.
std::int64_t makespan_of(const Jobs& jobs, const std::vector<int>& machine, int machines) {
    REQUIRE_EQ(machine.size(), jobs.size());
    std::vector<std::int64_t> loads(static_cast<std::size_t>(machines), 0);
    for (std::size_t i = 0; i < jobs.size(); ++i) {
        REQUIRE(machine[i] >= 0);
        REQUIRE(machine[i] < machines);
        loads[static_cast<std::size_t>(machine[i])] += jobs[i];
    }
    return *std::ranges::max_element(loads);
}

// Minimum makespan over every assignment.
std::int64_t exhaustive(const Jobs& jobs, std::size_t machines) {
    std::vector<int> machine(jobs.size(), 0);
    std::int64_t best = std::numeric_limits<std::int64_t>::max();
    while (true) {
        std::vector<std::int64_t> loads(machines, 0);
        for (std::size_t i = 0; i < jobs.size(); ++i) {
            loads[static_cast<std::size_t>(machine[i])] += jobs[i];
        }
        best = std::min(best, *std::ranges::max_element(loads));
        std::size_t i = 0;
        while (i < jobs.size() && ++machine[i] == static_cast<int>(machines)) {
            machine[i] = 0;
            ++i;
        }
        if (i == jobs.size()) {
            return best;
        }
    }
}

Jobs random_jobs(Rng& rng, std::size_t max_count, int max_job) {
    return rng.ints(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(max_count))), 1,
                    max_job);
}

std::int64_t sum_of(const Jobs& jobs) {
    return std::accumulate(jobs.begin(), jobs.end(), std::int64_t{0});
}

}  // namespace

TEST(1, "no jobs have makespan zero") {
    const Assignment result = balance_two({});
    CHECK_EQ(result.makespan, 0);
    CHECK(result.machine.empty());
}

TEST(1, "a single job goes somewhere") {
    const Jobs jobs{7};
    const Assignment result = balance_two(jobs);
    CHECK_EQ(result.makespan, 7);
    CHECK_EQ(makespan_of(jobs, result.machine, 2), 7);
}

TEST(1, "an even split is found") {
    const Jobs jobs{3, 3, 2, 2, 2};
    CHECK_EQ(balance_two(jobs).makespan, 6);
}

TEST(1, "random jobs match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 1000; ++trial) {
        const Jobs jobs = random_jobs(rng, 11, 30);
        INFO("jobs ", jobs);
        const Assignment result = balance_two(jobs);
        REQUIRE_EQ(makespan_of(jobs, result.machine, 2), result.makespan);
        REQUIRE_EQ(result.makespan, exhaustive(jobs, 2));
    }
}

PERF(1, "many jobs on two machines in pseudo-polynomial time", 1500) {
    Rng rng;
    const Jobs jobs = rng.ints(1000, 1, 1000);
    CHECK_LE(balance_two(jobs).makespan, sum_of(jobs) / 2 + 1);
}

TEST(2, "no jobs and a single job") {
    CHECK_EQ(min_makespan_four({}), 0);
    const Jobs one{5};
    CHECK_EQ(min_makespan_four(one), 5);
}

TEST(2, "four equal jobs spread out") {
    const Jobs jobs{5, 5, 5, 5};
    CHECK_EQ(min_makespan_four(jobs), 5);
}

TEST(2, "a dominant job sets the makespan") {
    const Jobs jobs{100, 1, 1, 1};
    CHECK_EQ(min_makespan_four(jobs), 100);
}

TEST(2, "four machines beat two on the same jobs") {
    const Jobs jobs{8, 7, 6, 5, 4, 3};
    CHECK_EQ(min_makespan_four(jobs), 9);
}

TEST(2, "random jobs match enumeration") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const Jobs jobs = random_jobs(rng, 6, 12);
        INFO("jobs ", jobs);
        REQUIRE_EQ(min_makespan_four(jobs), exhaustive(jobs, 4));
    }
}

PERF(2, "n times m cubed stays feasible", 3000) {
    Rng rng;
    const Jobs jobs = rng.ints(25, 1, 8);
    CHECK_GT(min_makespan_four(jobs), 0);
}

TEST(3, "the assignment is real and achieves the optimum") {
    const Jobs jobs{8, 7, 6, 5, 4, 3};
    const Assignment result = balance_four(jobs);
    CHECK_EQ(result.makespan, 9);
    CHECK_EQ(makespan_of(jobs, result.machine, 4), 9);
}

TEST(3, "no jobs give an empty assignment") {
    CHECK(balance_four({}).machine.empty());
}

TEST(3, "random assignments are valid and optimal") {
    Rng rng;
    for (int trial = 0; trial < 500; ++trial) {
        const Jobs jobs = random_jobs(rng, 7, 10);
        INFO("jobs ", jobs);
        const Assignment result = balance_four(jobs);
        REQUIRE_EQ(makespan_of(jobs, result.machine, 4), result.makespan);
        REQUIRE_EQ(result.makespan, exhaustive(jobs, 4));
    }
}

TEST(3, "duplicate job sizes are handled") {
    const Jobs jobs{6, 6, 6, 6, 6};
    const Assignment result = balance_four(jobs);
    CHECK_EQ(result.makespan, 12);
    CHECK_EQ(makespan_of(jobs, result.machine, 4), 12);
}

TEST(3, "memory does not grow with a layer per job") {
    // Sum 200 means roughly 8 million states for one layer of int32; storing one per job would
    // be 25 times that. Allow a generous constant factor over a single table.
    const Jobs jobs(25, 8);
    heap_meter::Scope scope;
    const Assignment result = balance_four(jobs);
    const std::size_t side = 201;
    CHECK_LE(scope.peak_bytes(), 8 * side * side * side);
    CHECK_EQ(result.makespan, 56);
}

PERF(3, "assignment within the cubic time budget", 3000) {
    Rng rng;
    const Jobs jobs = rng.ints(25, 1, 8);
    const Assignment result = balance_four(jobs);
    CHECK_EQ(makespan_of(jobs, result.machine, 4), result.makespan);
}

TEST(4, "zero machines is rejected") {
    const Jobs jobs{1};
    CHECK_THROWS_AS(plan_jobs(jobs, 0, 100), std::invalid_argument);
}

TEST(4, "one machine is trivially exact") {
    const Jobs jobs{4, 9};
    const Plan plan = plan_jobs(jobs, 1, 0);
    CHECK(plan.exact);
    CHECK_EQ(plan.makespan, 13);
    CHECK_EQ(makespan_of(jobs, plan.machine, 1), 13);
}

TEST(4, "no jobs make an exact empty plan on one machine") {
    const Plan plan = plan_jobs({}, 1, 0);
    CHECK_EQ(plan.makespan, 0);
    CHECK(plan.machine.empty());
}

TEST(4, "exact plans are used when the work fits the budget") {
    const Jobs jobs{8, 7, 6, 5, 4, 3};
    const std::uint64_t m = 33;
    const std::uint64_t work = jobs.size() * (m + 1);
    const Plan two = plan_jobs(jobs, 2, work);
    CHECK(two.exact);
    CHECK_EQ(two.makespan, exhaustive(jobs, 2));

    const Plan four = plan_jobs(jobs, 4, jobs.size() * (m + 1) * (m + 1) * (m + 1));
    CHECK(four.exact);
    CHECK_EQ(four.makespan, exhaustive(jobs, 4));
}

TEST(4, "the plan falls back to LPT when the work exceeds the budget") {
    const Jobs jobs{8, 7, 6, 5, 4, 3};
    const Plan plan = plan_jobs(jobs, 4, jobs.size() * 2);
    CHECK(!plan.exact);
    CHECK_EQ(makespan_of(jobs, plan.machine, 4), plan.makespan);
}

TEST(4, "machine counts without an exact solver always use LPT") {
    const Jobs jobs{9, 4, 4, 3};
    const Plan plan = plan_jobs(jobs, 3, std::numeric_limits<std::uint64_t>::max());
    CHECK(!plan.exact);
    CHECK_EQ(makespan_of(jobs, plan.machine, 3), plan.makespan);
    CHECK_EQ(plan.makespan, 9);
}

TEST(4, "LPT is the greedy choice, not the optimum") {
    const Jobs jobs{5, 5, 4, 4, 3, 3, 3};
    // Greedy ends with loads 11, 8, 8; the optimum 9 pairs 5+4, 5+4 and 3+3+3.
    const Plan plan = plan_jobs(jobs, 3, 0);
    CHECK_EQ(plan.makespan, 11);
    CHECK_EQ(makespan_of(jobs, plan.machine, 3), 11);
    CHECK_EQ(exhaustive(jobs, 3), 9);
}

TEST(4, "LPT stays within the 4/3 approximation bound") {
    Rng rng;
    for (int trial = 0; trial < 300; ++trial) {
        const Jobs jobs = random_jobs(rng, 8, 30);
        const auto machines = static_cast<std::size_t>(rng.uniform(1, 4));
        INFO("jobs ", jobs, " machines ", machines);
        const Plan plan = plan_jobs(jobs, machines, 0);
        REQUIRE_EQ(makespan_of(jobs, plan.machine, static_cast<int>(machines)), plan.makespan);
        if (machines > 1) {
            const std::int64_t k = static_cast<std::int64_t>(machines);
            const std::int64_t optimum = exhaustive(jobs, machines);
            // makespan <= (4/3 - 1/(3k)) * optimum, written without fractions.
            REQUIRE(3 * k * plan.makespan <= (4 * k - 1) * optimum);
        }
    }
}

TEST(4, "exact solvers beat the LPT bound on the same inputs") {
    Rng rng;
    for (int trial = 0; trial < 200; ++trial) {
        const Jobs jobs = random_jobs(rng, 7, 20);
        INFO("jobs ", jobs);
        const Plan exact = plan_jobs(jobs, 4, std::numeric_limits<std::uint64_t>::max());
        const Plan greedy = plan_jobs(jobs, 4, 0);
        REQUIRE(exact.exact);
        REQUIRE_EQ(greedy.exact, jobs.empty());  // no jobs is no work, within any budget
        REQUIRE(exact.makespan <= greedy.makespan);
        REQUIRE_EQ(exact.makespan, exhaustive(jobs, 4));
    }
}

PERF(4, "LPT handles a million jobs quickly", 1500) {
    Rng rng;
    const Jobs jobs = rng.ints(1'000'000, 1, 1'000'000);
    const Plan plan = plan_jobs(jobs, 64, 0);
    CHECK(!plan.exact);
    CHECK_LE(plan.makespan, sum_of(jobs));
}
