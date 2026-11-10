#include <algorithm>
#include <numeric>
#include <set>
#include <vector>

#include "practice.hpp"
#include "solution.hpp"

using practice::Rng;

namespace {

ComputationDag random_dag(Rng& rng, std::size_t n, double density) {
    ComputationDag dag(n);
    for (std::size_t u = 0; u < n; ++u) {
        for (std::size_t v = u + 1; v < n; ++v) {
            if (rng.coin(density)) {
                dag.add_edge(u, v);
            }
        }
    }
    return dag;
}

std::size_t brute_span(const ComputationDag& dag) {
    std::size_t best = 0;
    std::vector<std::size_t> longest(dag.size(), 1);
    // Forward edges only in random_dag, so increasing index order is topological.
    for (std::size_t v = 0; v < dag.size(); ++v) {
        best = std::max(best, longest[v]);
        for (const std::size_t w : dag.successors(v)) {
            longest[w] = std::max(longest[w], longest[v] + 1);
        }
    }
    return best;
}

std::vector<std::size_t> identity_priority(std::size_t n) {
    std::vector<std::size_t> p(n);
    std::iota(p.begin(), p.end(), std::size_t{0});
    return p;
}

// Rejects anything but a valid greedy schedule honoring the priority rule.
void check_greedy(const ComputationDag& dag, const Schedule& schedule, std::size_t processors,
                  const std::vector<std::size_t>& priority) {
    const std::size_t n = dag.size();
    REQUIRE_EQ(schedule.step_of.size(), n);
    std::vector<std::vector<std::size_t>> at_step(schedule.length);
    for (std::size_t v = 0; v < n; ++v) {
        REQUIRE(schedule.step_of[v] < schedule.length);
        at_step[schedule.step_of[v]].push_back(v);
        for (const std::size_t w : dag.successors(v)) {
            REQUIRE(schedule.step_of[v] < schedule.step_of[w]);
        }
    }
    std::vector<std::size_t> predecessors_left(n, 0);
    for (std::size_t v = 0; v < n; ++v) {
        for (const std::size_t w : dag.successors(v)) {
            ++predecessors_left[w];
        }
    }
    std::vector<char> done(n, 0);
    for (std::size_t t = 0; t < schedule.length; ++t) {
        REQUIRE(at_step[t].size() <= processors);
        REQUIRE(!at_step[t].empty());
        std::vector<std::size_t> ready;
        for (std::size_t v = 0; v < n; ++v) {
            if (done[v] == 0 && predecessors_left[v] == 0) {
                ready.push_back(v);
            }
        }
        // Greedy: run min(ready, p) strands, the ones with the smallest (priority, index).
        const std::size_t runs = std::min(ready.size(), processors);
        REQUIRE_EQ(at_step[t].size(), runs);
        std::sort(ready.begin(), ready.end(), [&](std::size_t a, std::size_t b) {
            return std::pair(priority[a], a) < std::pair(priority[b], b);
        });
        std::vector<std::size_t> chosen(ready.begin(), ready.begin() + static_cast<std::ptrdiff_t>(runs));
        std::vector<std::size_t> ran = at_step[t];
        std::sort(chosen.begin(), chosen.end());
        std::sort(ran.begin(), ran.end());
        REQUIRE_EQ(ran, chosen);
        for (const std::size_t v : ran) {
            done[v] = 1;
            for (const std::size_t w : dag.successors(v)) {
                --predecessors_left[w];
            }
        }
    }
}

}  // namespace

TEST(1, "an empty graph has no work and no span") {
    const ComputationDag dag(0);
    CHECK_EQ(dag.size(), 0);
    CHECK_EQ(dag.work(), 0);
    CHECK_EQ(dag.span(), 0);
    CHECK_NEAR(dag.parallelism(), 1.0, 1e-9);
}

TEST(1, "a single strand and a chain") {
    const ComputationDag one(1);
    CHECK_EQ(one.span(), 1);
    ComputationDag chain(5);
    for (std::size_t v = 0; v + 1 < 5; ++v) {
        chain.add_edge(v, v + 1);
    }
    CHECK_EQ(chain.work(), 5);
    CHECK_EQ(chain.span(), 5);
    CHECK_NEAR(chain.parallelism(), 1.0, 1e-9);
}

TEST(1, "independent strands have span one") {
    const ComputationDag dag(6);
    CHECK_EQ(dag.span(), 1);
    CHECK_NEAR(dag.parallelism(), 6.0, 1e-9);
}

TEST(1, "span is the longest path over all branches") {
    ComputationDag dag(6);
    dag.add_edge(0, 1);
    dag.add_edge(1, 5);
    dag.add_edge(0, 2);
    dag.add_edge(2, 3);
    dag.add_edge(3, 4);
    dag.add_edge(4, 5);
    CHECK_EQ(dag.span(), 5);
}

TEST(1, "invalid edges are rejected") {
    ComputationDag dag(3);
    CHECK_THROWS_AS(dag.add_edge(0, 3), std::out_of_range);
    CHECK_THROWS_AS(dag.add_edge(3, 0), std::out_of_range);
    CHECK_THROWS_AS(dag.add_edge(1, 1), std::invalid_argument);
}

TEST(1, "a cycle is reported") {
    ComputationDag dag(3);
    dag.add_edge(0, 1);
    dag.add_edge(1, 2);
    dag.add_edge(2, 0);
    CHECK_THROWS_AS(dag.span(), std::logic_error);
    CHECK_THROWS_AS(dag.parallelism(), std::logic_error);
}

TEST(1, "span matches a direct computation on random DAGs") {
    Rng rng;
    for (int round = 0; round < 200; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 200));
        const ComputationDag dag = random_dag(rng, n, rng.real() * 0.2);
        INFO("round ", round, " n ", n);
        CHECK_EQ(dag.span(), brute_span(dag));
    }
}

TEST(1, "a very long chain does not overflow the stack") {
    const std::size_t n = 1'000'000;
    ComputationDag chain(n);
    for (std::size_t v = 0; v + 1 < n; ++v) {
        chain.add_edge(v, v + 1);
    }
    CHECK_EQ(chain.span(), n);
}

PERF(1, "span stays linear on large graphs", 1500) {
    Rng rng;
    const std::size_t n = 1'000'000;
    ComputationDag dag(n);
    for (std::size_t v = 1; v < n; ++v) {
        dag.add_edge(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(v) - 1)), v);
    }
    CHECK_GE(dag.span(), 2);
}

TEST(2, "zero processors is an error") {
    const ComputationDag dag(1);
    CHECK_THROWS_AS(greedy_schedule(dag, 0, identity_priority(1)), std::invalid_argument);
}

TEST(2, "an empty graph needs no steps") {
    const ComputationDag dag(0);
    const Schedule schedule = greedy_schedule(dag, 4, {});
    CHECK_EQ(schedule.length, 0);
    CHECK(schedule.step_of.empty());
}

TEST(2, "independent strands fill every processor") {
    const ComputationDag dag(10);
    const Schedule s = greedy_schedule(dag, 3, identity_priority(10));
    CHECK_EQ(s.length, 4);
    check_greedy(dag, s, 3, identity_priority(10));
}

TEST(2, "a chain runs one strand per step on any machine") {
    ComputationDag chain(7);
    for (std::size_t v = 0; v + 1 < 7; ++v) {
        chain.add_edge(v, v + 1);
    }
    for (const std::size_t p : {1, 2, 64}) {
        const Schedule s = greedy_schedule(chain, p, identity_priority(7));
        CHECK_EQ(s.length, 7);
    }
}

TEST(2, "strands that become ready wait for the next step") {
    ComputationDag dag(4);
    dag.add_edge(0, 1);  // 1 becomes ready only after 0 has run
    const Schedule s = greedy_schedule(dag, 8, identity_priority(4));
    CHECK_EQ(s.length, 2);
    CHECK_EQ(s.step_of[0], 0);
    CHECK_EQ(s.step_of[1], 1);
    CHECK_EQ(s.step_of[2], 0);
}

TEST(2, "priority decides which strands run first") {
    const ComputationDag dag(4);
    std::vector<std::size_t> priority = {3, 2, 1, 0};
    const Schedule s = greedy_schedule(dag, 2, priority);
    CHECK_EQ(s.step_of[2], 0);
    CHECK_EQ(s.step_of[3], 0);
    CHECK_EQ(s.step_of[0], 1);
    CHECK_EQ(s.step_of[1], 1);
    check_greedy(dag, s, 2, priority);
}

TEST(2, "ties in priority are broken by the lower strand number") {
    const ComputationDag dag(4);
    const std::vector<std::size_t> priority(4, 5);
    const Schedule s = greedy_schedule(dag, 2, priority);
    CHECK_EQ(s.step_of[0], 0);
    CHECK_EQ(s.step_of[1], 0);
    CHECK_EQ(s.step_of[2], 1);
    CHECK_EQ(s.step_of[3], 1);
}

TEST(2, "a cycle is reported") {
    ComputationDag dag(2);
    dag.add_edge(0, 1);
    dag.add_edge(1, 0);
    CHECK_THROWS_AS(greedy_schedule(dag, 2, identity_priority(2)), std::logic_error);
}

TEST(2, "schedules are valid and greedy on random DAGs") {
    Rng rng;
    for (int round = 0; round < 150; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 150));
        const ComputationDag dag = random_dag(rng, n, rng.real() * 0.15);
        const auto p = static_cast<std::size_t>(rng.uniform(1, 8));
        std::vector<std::size_t> priority(n);
        for (auto& value : priority) {
            value = static_cast<std::size_t>(rng.uniform(0, 5));
        }
        const Schedule s = greedy_schedule(dag, p, priority);
        INFO("round ", round, " n ", n, " p ", p);
        check_greedy(dag, s, p, priority);
    }
}

TEST(2, "the work law the span law and the greedy bound always hold") {
    Rng rng;
    for (int round = 0; round < 400; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 200));
        const ComputationDag dag = random_dag(rng, n, rng.real() * 0.2);
        const auto t1 = static_cast<double>(dag.work());
        const auto tinf = static_cast<double>(dag.span());
        std::vector<std::vector<std::size_t>> policies;
        policies.push_back(identity_priority(n));
        std::vector<std::size_t> reversed(n);
        for (std::size_t i = 0; i < n; ++i) {
            reversed[i] = n - i;
        }
        policies.push_back(reversed);
        policies.emplace_back(n, 0);
        for (const auto& priority : policies) {
            for (const std::size_t p : {1, 2, 3, 5, 8, 16, 1000}) {
                const auto tp = static_cast<double>(greedy_schedule(dag, p, priority).length);
                const auto processors = static_cast<double>(p);
                INFO("round ", round, " n ", n, " p ", p, " tp ", tp, " t1 ", t1, " tinf ", tinf);
                REQUIRE(tp >= t1 / processors);
                REQUIRE(tp >= tinf);
                REQUIRE(tp <= (t1 - tinf) / processors + tinf + 1e-9);
            }
        }
    }
}

PERF(2, "greedy_schedule handles large graphs", 3000) {
    Rng rng;
    const std::size_t n = 1'000'000;
    ComputationDag dag(n);
    for (std::size_t v = 1; v < n; ++v) {
        dag.add_edge(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(v) - 1)), v);
        if (v >= 2 && rng.coin()) {
            dag.add_edge(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(v) - 2)), v);
        }
    }
    const Schedule s = greedy_schedule(dag, 64, identity_priority(n));
    CHECK_GE(s.length, dag.span());
}

TEST(3, "the good order finishes in the chain length") {
    for (const std::size_t p : {2, 3, 8}) {
        const std::size_t k = 200;
        const NearTwiceInstance instance = near_twice_instance(p, k);
        CHECK_EQ(instance.dag.size(), k + (p - 1) * k);
        const Schedule good = greedy_schedule(instance.dag, p, instance.good_priority);
        INFO("p ", p);
        CHECK_EQ(good.length, k);
    }
}

TEST(3, "the bad order is nearly twice as slow") {
    for (const std::size_t p : {2, 4, 16, 32}) {
        const std::size_t k = 2'000;
        const NearTwiceInstance instance = near_twice_instance(p, k);
        const Schedule good = greedy_schedule(instance.dag, p, instance.good_priority);
        const Schedule bad = greedy_schedule(instance.dag, p, instance.bad_priority);
        const double ratio = static_cast<double>(bad.length) / static_cast<double>(good.length);
        const double target = 2.0 - 1.0 / static_cast<double>(p);
        INFO("p ", p, " ratio ", ratio, " target ", target);
        CHECK_GE(ratio, target - 0.01);
        check_greedy(instance.dag, bad, p, instance.bad_priority);
    }
}

TEST(3, "the two priorities are genuine permutations of the choices") {
    const NearTwiceInstance instance = near_twice_instance(4, 10);
    CHECK_EQ(instance.good_priority.size(), instance.dag.size());
    CHECK_EQ(instance.bad_priority.size(), instance.dag.size());
    CHECK(instance.good_priority != instance.bad_priority);
}

TEST(4, "no tasks means nothing to do") {
    const BuildPlan plan = plan_build(std::span<const BuildTask>(), 4);
    CHECK_EQ(plan.total_work, 0);
    CHECK_EQ(plan.critical_path, 0);
    CHECK_EQ(plan.makespan, 0);
}

TEST(4, "a chain of tasks runs in sequence") {
    const std::vector<BuildTask> tasks = {{3, {}}, {2, {0}}, {4, {1}}};
    const BuildPlan plan = plan_build(tasks, 8);
    CHECK_EQ(plan.total_work, 9);
    CHECK_EQ(plan.critical_path, 9);
    CHECK_EQ(plan.makespan, 9);
}

TEST(4, "workers run independent tasks in index order") {
    // With one worker the ready order is by index; 0 runs first even though 1 is shorter.
    const std::vector<BuildTask> tasks = {{5, {}}, {1, {}}, {3, {0}}};
    CHECK_EQ(plan_build(tasks, 1).makespan, 9);
    CHECK_EQ(plan_build(tasks, 2).makespan, 8);
    const BuildPlan plan = plan_build(tasks, 3);
    CHECK_EQ(plan.critical_path, 8);
    CHECK_EQ(plan.makespan, 8);
}

TEST(4, "a task starts as soon as its last dependency finishes") {
    const std::vector<BuildTask> tasks = {{2, {}}, {7, {}}, {1, {0, 1}}};
    const BuildPlan plan = plan_build(tasks, 2);
    CHECK_EQ(plan.critical_path, 8);
    CHECK_EQ(plan.makespan, 8);
}

TEST(4, "a dependency that finishes frees a worker for a queued task") {
    // Two workers. Task 2 needs 0; tasks 0 and 1 start together; when 0 finishes at t=1 the
    // freed worker must pick 2 before 3.
    const std::vector<BuildTask> tasks = {{1, {}}, {5, {}}, {1, {0}}, {1, {}}};
    const BuildPlan plan = plan_build(tasks, 2);
    CHECK_EQ(plan.makespan, 5);
}

TEST(4, "unit durations agree with the discrete greedy simulator") {
    Rng rng;
    for (int round = 0; round < 100; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 120));
        ComputationDag dag(n);
        std::vector<BuildTask> tasks(n, BuildTask{1, {}});
        for (std::size_t v = 1; v < n; ++v) {
            for (std::size_t u = 0; u < v; ++u) {
                if (rng.coin(0.08)) {
                    dag.add_edge(u, v);
                    tasks[v].deps.push_back(u);
                }
            }
        }
        for (const std::size_t p : {1, 2, 3, 7, 64}) {
            INFO("round ", round, " n ", n, " p ", p);
            const auto schedule = greedy_schedule(dag, p, identity_priority(n));
            const BuildPlan plan = plan_build(tasks, p);
            CHECK_EQ(plan.makespan, static_cast<long long>(schedule.length));
            CHECK_EQ(plan.total_work, static_cast<long long>(n));
            CHECK_EQ(plan.critical_path, static_cast<long long>(dag.span()));
        }
    }
}

TEST(4, "Graham's list-scheduling bound holds with arbitrary durations") {
    Rng rng;
    for (int round = 0; round < 300; ++round) {
        const auto n = static_cast<std::size_t>(rng.uniform(1, 150));
        std::vector<BuildTask> tasks(n);
        for (std::size_t v = 0; v < n; ++v) {
            tasks[v].duration = rng.uniform(1, 50);
            for (std::size_t u = 0; u < v; ++u) {
                if (rng.coin(0.05)) {
                    tasks[v].deps.push_back(u);
                }
            }
        }
        for (const std::size_t p : {1, 2, 3, 5, 16}) {
            const BuildPlan plan = plan_build(tasks, p);
            const double t1 = static_cast<double>(plan.total_work);
            const double tinf = static_cast<double>(plan.critical_path);
            INFO("round ", round, " p ", p, " makespan ", plan.makespan, " t1 ", t1, " tinf ", tinf);
            REQUIRE(static_cast<double>(plan.makespan) >= t1 / static_cast<double>(p) - 1e-9);
            REQUIRE(plan.makespan >= plan.critical_path);
            REQUIRE(static_cast<double>(plan.makespan) <=
                    (t1 - tinf) / static_cast<double>(p) + tinf + 1e-9);
        }
    }
}

PERF(4, "plan_build handles large task graphs", 3000) {
    Rng rng;
    const std::size_t n = 400'000;
    std::vector<BuildTask> tasks(n);
    for (std::size_t v = 0; v < n; ++v) {
        tasks[v].duration = rng.uniform(1, 1000);
        if (v > 0) {
            tasks[v].deps.push_back(static_cast<std::size_t>(rng.uniform(0, static_cast<std::int64_t>(v) - 1)));
        }
    }
    const BuildPlan plan = plan_build(tasks, 256);
    CHECK_GE(plan.makespan, plan.critical_path);
}
